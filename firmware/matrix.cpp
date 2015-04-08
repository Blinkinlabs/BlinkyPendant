#include <matrix.h>

// Output positions for the signals in each group
// These are out of order to make the board routing easier
// RGB RGB RGB RGB RGB
uint8_t OUTPUT_ORDER[] = {
   8, // G0
  13, // B0
   7, // R0
  12, // G1
   6, // B1
  11, // R1
  10, // G2
   4, // B2
   9, // R2
  14, // G3
   0, // B3
   5, // R3
   1, // G4
   2, // B4
   3, // R4
};


// Offsets in the port c register (data)
#define DMA_DAT_SHIFT   0       // Location of the data pin in Port C register
#define DMA_CLK_SHIFT   1       // Location of the clock pin in Port C register

// Offsets in the port D register (address)
// Note: The select lines are contiguous starting with 0, so they don't need to be shifted
#define DMA_STB_SHIFT  6        // Location of the strobe pin in Port D register


// Display buffer (write into this!)
pixel Pixels[LED_COLS * LED_ROWS];

// Address output buffer
// Note: Increasing the count BYTE_WRITES_PER_ADDRESS will increase the time delay between
// LED_OE deassertion and LED_STB strobe.
#define ADDRESS_REPEAT_COUNT 10
uint8_t Addresses[BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT*ADDRESS_REPEAT_COUNT];

// Timer output buffers (these will be DMAd to the FTM1_MOD and FTM1_C0V registers)
uint32_t FTM1_MODStates[BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT];
uint32_t FTM1_C0VStates[BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT];

// Big 'ol waveform that should be sent out over DMA in chunks.
// There are LED_ROWS/ROWS_PER_OUTPUT separate loops, where the LED matrix address lines
// to be set before they are activated.
// For each of these rows, there are then BIT_DEPTH separate inner loops
// And each inner loop has LED_COLS * 2 bytes states (the data is LED_COLS long, plus the clock signal is baked in)
//#define ROW_BIT_SIZE (LED_COLS*2)                                  // Number of bytes required to store a single row of 1-bit color data output

#define ROW_BIT_SIZE (LED_COLS*3*2)                                // Number of bytes required to store a single row of 1-bit color data output
#define ROW_DEPTH_SIZE (ROW_BIT_SIZE*BIT_DEPTH)                    // Number of bytes required to store a single row of full-color data output
#define PANEL_DEPTH_SIZE (ROW_DEPTH_SIZE*LED_ROWS/ROWS_PER_OUTPUT) // Number of bytes required to store an entire panel's worth of data output.

// 2x DMA buffer
// Note: Extra ROW_BIT_SIZE at end to account for extra DMA transfer
// TODO: Trigger int from last address and skip the extra data transfer?
uint8_t DmaBuffer[2][PANEL_DEPTH_SIZE];

void pixelsToDmaBuffer(struct pixel* pixelInput, uint8_t bufferOutput[]);

void setupTCD0(uint32_t* source, int minorLoopSize, int majorLoops);
void setupTCD1(uint32_t* source, int minorLoopSize, int majorLoops);
void setupTCD2(uint8_t* source, int minorLoopSize, int majorLoops);
void setupTCD3(uint8_t* source, int minorLoopSize, int majorLoops);
void dma_ch2_isr(void);
void setupTCDs();
void setupFTM1();


void matrixSetup() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(S4, OUTPUT);
  pinMode(S5, OUTPUT);

  pinMode(LED_DAT, OUTPUT);
  pinMode(LED_CLK, OUTPUT);
  pinMode(LED_STB, OUTPUT);
  pinMode(LED_OE, OUTPUT);

  // Fill the address table
  // To make the DMA engine easier to program, we store a copy of the address table for each output page.
  for(int address = 0; address < LED_ROWS/ROWS_PER_OUTPUT; address++) {
    for(int page = 0; page < BIT_DEPTH; page++) {
      int last_address;
      if(page == 0) {
        last_address = (address + LED_ROWS/ROWS_PER_OUTPUT - 1)%(LED_ROWS/ROWS_PER_OUTPUT);
      }
      else {
        last_address = address;
      }
      
      for(int i = 0; i < ADDRESS_REPEAT_COUNT; i++) {
        // Note: We're actually pumping out the last address here, to avoid changing it too soon after
        // deasserting enable.
        Addresses[(address*BIT_DEPTH + page)*ADDRESS_REPEAT_COUNT + i] = (0x3F & ~(1 << last_address));
      }
      
      // TODO: Inserted to cause extra delay between OE and address change.
      Addresses[(address*BIT_DEPTH + page)*ADDRESS_REPEAT_COUNT + ADDRESS_REPEAT_COUNT - 2] = (0x3F & ~(1 << address)) | (1 << DMA_STB_SHIFT);
      Addresses[(address*BIT_DEPTH + page)*ADDRESS_REPEAT_COUNT + ADDRESS_REPEAT_COUNT - 1] = (0x3F & ~(1 << address));
    }
  }


  // Fill the timer states table
  for(int address = 0; address < LED_ROWS/ROWS_PER_OUTPUT; address++) {

    // Each row update consists of BIT_DEPTH cycles. The length of the 'on' time
    // (when OE is asserted) on each cycle is set by onTime; it begins with
    // ON_TIME_MIN and doubles every cycle after that to create a binary progression.
    // TODO: What does this translate to, in time?
    #define LOW_BIT_ENABLE_TIME     0x2             // Shortest OE on interval; the shorter, the dimmer the lowest bit.
    
    // The interval between OE cycle is set by one of the three cases:
    // 1. For low bits, where onTime is small, the interval is expanded to MIN_CYCLE_TIME
    // 2. For longer bits, where onTime is longer, the cycle time is calculated as onTime + MIN_BLANKING_TIME
    // 3. For the last cycle of the last row, the cycle time is expanded to MIN_LAST_CYCLE_TIME to allow
    //    the display interrupt to update.

    #define MIN_BLANKING_TIME       0x50        // Minimum time between OE assertions
    #define MIN_CYCLE_TIME          0x05F       // 
    #define MIN_LAST_CYCLE_TIME     0x0120      // Mininum number of cycles for the last cycle loop.


    int onTime = LOW_BIT_ENABLE_TIME;               

    for(int page = 0; page < BIT_DEPTH; page++) {
      if((address == LED_ROWS/ROWS_PER_OUTPUT -1)
         && (page == BIT_DEPTH - 2)
         && ((onTime + MIN_BLANKING_TIME) < MIN_LAST_CYCLE_TIME)) {
        // On the second-to-last cycle, we need enough time to flush the DMA engines and handle the
        // interrupt to reset the DMA engines. If the combination of blanking time and
        // on time don't meet this, increase the timer cycle count to an acceptable length.
        FTM1_C0VStates[address*BIT_DEPTH + page] = onTime;
        FTM1_MODStates[address*BIT_DEPTH + page] = MIN_LAST_CYCLE_TIME;        
      }      
      else if((onTime + MIN_BLANKING_TIME) < MIN_CYCLE_TIME) {
        // The DMA engines need enough time to write out the data after every cycle.
        // WHen the on time is really low, the combination of blanking time and
        // on time might not create a long enough delay to meet this, so we need to increase
        // the timer cycle count to meet this requirement.
        FTM1_C0VStates[address*BIT_DEPTH + page] = onTime;
        FTM1_MODStates[address*BIT_DEPTH + page] = MIN_CYCLE_TIME;
      }
      else {
        FTM1_C0VStates[address*BIT_DEPTH + page] = onTime;      
        FTM1_MODStates[address*BIT_DEPTH + page] = onTime + MIN_BLANKING_TIME;
      }

      onTime = onTime*2;
    }
  }

  // DMA
  // Configure DMA
  SIM_SCGC7 |= SIM_SCGC7_DMA;  // Enable DMA clock
  DMA_CR = 0;  // Use default configuration

  // Configure the DMA request input for DMA0
  DMA_SERQ = DMA_SERQ_SERQ(0);

  // Enable interrupt on major completion for DMA channel 2 (address)
  DMA_TCD2_CSR = DMA_TCD_CSR_INTMAJOR;  // Enable interrupt on major complete
  NVIC_ENABLE_IRQ(IRQ_DMA_CH2);         // Enable interrupt request

  // DMAMUX
  // Configure the DMAMUX
  SIM_SCGC6 |= SIM_SCGC6_DMAMUX; // Enable DMAMUX clock

  // Timer DMA channel:
  // Configure DMAMUX to trigger DMA0 from FTM1_CH0
  DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
  DMAMUX0_CHCFG0 = DMAMUX_SOURCE_FTM1_CH0 | DMAMUX_ENABLE;

  // Load this frame of data into the DMA engine
  setupTCDs();

  // FTM
  SIM_SCGC6 |= SIM_SCGC6_FTM1;  // Enable FTM0 clock
  setupFTM1();

  // Clear the display
  pixelsToDmaBuffer(Pixels, DmaBuffer[0]);
}

void updateMatrix() {
    // TODO: Double-buffer the Dma buffer frame
    pixelsToDmaBuffer(Pixels, DmaBuffer[0]);
}



// Munge the data so it can be written out by the DMA engine
// Note: bufferOutput[][xxx] should have BIT_DEPTH as xxx
void pixelsToDmaBuffer(struct pixel* pixelInput, uint8_t bufferOutput[]) {

  // Fill in the pixel data
  // TODO: Fix this so the color info is read correctly.
  // Right now, it's globbed together...
  
  for(int row = 0; row < LED_ROWS/ROWS_PER_OUTPUT; row++) {
    for(int col = 0; col < LED_COLS; col++) {
      
      // Data is the data to
      int data_R = pixelInput[row*LED_COLS + col].R;
      int data_G = pixelInput[row*LED_COLS + col].G;
      int data_B = pixelInput[row*LED_COLS + col].B;

      for(int depth = 0; depth < BIT_DEPTH; depth++) {
        uint8_t output_r =
            (((data_R >> depth) & 0x01) << DMA_DAT_SHIFT);
        uint8_t output_g =
            (((data_G >> depth) & 0x01) << DMA_DAT_SHIFT);
        uint8_t output_b =
            (((data_B >> depth) & 0x01) << DMA_DAT_SHIFT);



        int offset_g = OUTPUT_ORDER[col*3 + 0];
        int offset_b = OUTPUT_ORDER[col*3 + 1];
        int offset_r = OUTPUT_ORDER[col*3 + 2];

        bufferOutput[row*ROW_DEPTH_SIZE + depth*ROW_BIT_SIZE + offset_r*2 + 0] = output_r;
        bufferOutput[row*ROW_DEPTH_SIZE + depth*ROW_BIT_SIZE + offset_r*2 + 1] = output_r | 1 << DMA_CLK_SHIFT;
        bufferOutput[row*ROW_DEPTH_SIZE + depth*ROW_BIT_SIZE + offset_g*2 + 0] = output_g;
        bufferOutput[row*ROW_DEPTH_SIZE + depth*ROW_BIT_SIZE + offset_g*2 + 1] = output_g | 1 << DMA_CLK_SHIFT;
        bufferOutput[row*ROW_DEPTH_SIZE + depth*ROW_BIT_SIZE + offset_b*2 + 0] = output_b;
        bufferOutput[row*ROW_DEPTH_SIZE + depth*ROW_BIT_SIZE + offset_b*2 + 1] = output_b | 1 << DMA_CLK_SHIFT;
      }
    }
  }
}


// TCD0 updates the timer values for FTM1
void setupTCD0(uint32_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD0_SADDR = source;                                        // Address to read from
  DMA_TCD0_SOFF = 4;                                              // Bytes to increment source register between writes 
  DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);  // 32-bit input and output
  DMA_TCD0_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD0_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  //  DMA_TCD0_DADDR = TimerStatesDump;                               // Address to write to
  DMA_TCD0_DADDR = &FTM1_MOD;                                      // Address to write to
  DMA_TCD0_DOFF = 0;                                              // Bytes to increment destination register between write
  //  DMA_TCD0_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  //  DMA_TCD0_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD0_DLASTSGA = 0;                                          // Address of next TCD (N/A)

  // Workaround for DMA majorelink unreliability: increase the minor loop count by one
  // Note that the final transfer doesn't end up happening, because 
  DMA_TCD0_CITER_ELINKYES = majorLoops + 1;                           // Number of major loops to complete
  DMA_TCD0_BITER_ELINKYES = majorLoops + 1;                           // Reset value for CITER (must be equal to CITER)

  // Trigger DMA1 (timer) after each minor loop
  DMA_TCD0_BITER_ELINKYES |= DMA_TCD_CITER_ELINK;
  DMA_TCD0_BITER_ELINKYES |= (0x01 << 9);  
  DMA_TCD0_CITER_ELINKYES |= DMA_TCD_CITER_ELINK;
  DMA_TCD0_CITER_ELINKYES |= (0x01 << 9);
}

// TCD1 updates the timer values for FTM1
void setupTCD1(uint32_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD1_SADDR = source;                                        // Address to read from
  DMA_TCD1_SOFF = 4;                                              // Bytes to increment source register between writes 
  DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);  // 32-bit input and output
  DMA_TCD1_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD1_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  //  DMA_TCD0_DADDR = TimerStatesDump;                               // Address to write to
  DMA_TCD1_DADDR = &FTM1_C0V;                                      // Address to write to
  DMA_TCD1_DOFF = 0;                                              // Bytes to increment destination register between write
  //  DMA_TCD1_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  //  DMA_TCD1_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD1_DLASTSGA = 0;                                          // Address of next TCD (N/A)

  // Workaround for DMA majorelink unreliability: increase the minor loop count by one
  // Note that the final transfer doesn't end up happening, because 
  DMA_TCD1_CITER_ELINKYES = majorLoops + 1;                           // Number of major loops to complete
  DMA_TCD1_BITER_ELINKYES = majorLoops + 1;                           // Reset value for CITER (must be equal to CITER)

  // Trigger DMA2 (address) after each minor loop
  DMA_TCD1_BITER_ELINKYES |= DMA_TCD_CITER_ELINK;
  DMA_TCD1_BITER_ELINKYES |= (0x02 << 9);  
  DMA_TCD1_CITER_ELINKYES |= DMA_TCD_CITER_ELINK;
  DMA_TCD1_CITER_ELINKYES |= (0x02 << 9);
}




// TCD2 writes out the address select lines, which are on port D
void setupTCD2(uint8_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD2_SADDR = source;                                        // Address to read from
  DMA_TCD2_SOFF = 1;                                              // Bytes to increment source register between writes 
  DMA_TCD2_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
  DMA_TCD2_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD2_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
//  DMA_TCD2_DADDR = &GPIOB_PDOR;                                   // Address to write to
  DMA_TCD2_DADDR = &GPIOD_PDOR;                                   // Address to write to
  DMA_TCD2_DOFF = 0;                                              // Bytes to increment destination register between write
  DMA_TCD2_CITER_ELINKYES = majorLoops;                           // Number of major loops to complete
  DMA_TCD2_BITER_ELINKYES = majorLoops;                           // Reset value for CITER (must be equal to CITER)
  DMA_TCD2_DLASTSGA = 0;                                          // Address of next TCD (N/A)
  
  // Workaround for DMA majorelink unreliability: increase the minor loop count by one
  // Note that the final transfer doesn't end up happening, because 
  DMA_TCD2_CITER_ELINKYES = majorLoops;                           // Number of major loops to complete
  DMA_TCD2_BITER_ELINKYES = majorLoops;                           // Reset value for CITER (must be equal to CITER)

  // Trigger DMA3 (address) after each minor loop
  DMA_TCD2_BITER_ELINKYES |= DMA_TCD_CITER_ELINK;
  DMA_TCD2_BITER_ELINKYES |= (0x03 << 9);  
  DMA_TCD2_CITER_ELINKYES |= DMA_TCD_CITER_ELINK;
  DMA_TCD2_CITER_ELINKYES |= (0x03 << 9);
}

// TCD3 clocks and strobes the pixel data, which are on port C
void setupTCD3(uint8_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD3_SADDR = source;                                        // Address to read from
  DMA_TCD3_SOFF = 1;                                              // Bytes to increment source register between writes 
  DMA_TCD3_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
  DMA_TCD3_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD3_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  DMA_TCD3_DADDR = &GPIOC_PDOR;                                   // Address to write to
  DMA_TCD3_DOFF = 0;                                              // Bytes to increment destination register between write
  DMA_TCD3_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  DMA_TCD3_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD3_DLASTSGA = 0;                                          // Address of next TCD (N/A)
}


// When the last address write has completed, that means we're at the end of the display refresh cycle
// Set up the next display frame
// TODO: flip pages, etc
void dma_ch2_isr(void) {
  DMA_CINT = DMA_CINT_CINT(2);
  
  setupTCDs();
}

void setupTCDs() {
  setupTCD0(FTM1_MODStates, 4,                        BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT);
  setupTCD1(FTM1_C0VStates, 4,                        BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT);
  setupTCD2(Addresses,      ADDRESS_REPEAT_COUNT, BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT);
  setupTCD3(DmaBuffer[0],   ROW_BIT_SIZE,             BIT_DEPTH*LED_ROWS/ROWS_PER_OUTPUT);

  DMA_SSRT = DMA_SSRT_SSRT(3);
}


// FTM1 drives our whole operation! We need to periodically update the
// FTM1_MOD and FTM1_C1V registers to program the next cycle.
void setupFTM1(){
  FTM1_MODE = FTM_MODE_WPDIS;    // Disable Write Protect

  FTM1_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?
  FTM1_MOD = 0x02FF;             // Period register
  FTM1_SC |= FTM_SC_CLKS(1) | FTM_SC_PS(1);

  FTM1_MODE |= FTM_MODE_INIT;         // Enable FTM0

  FTM1_C0SC = 0x40                    // Enable interrupt
  | 0x20                    // Mode select: Edge-aligned PWM 
  | 0x04                    // Low-true pulses (inverted)
  | 0x01;                   // Enable DMA out
  FTM1_C0V = 0x0200;        // Duty cycle of PWM signal
  FTM1_SYNC |= 0x80;        // set PWM value update


  // Configure LED_OE pinmux (LED_OE is on FTM1_CH0)
  PORTB_PCR0 = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE; 
}
