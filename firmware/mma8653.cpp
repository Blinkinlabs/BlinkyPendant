#include "mma8653.h"
#include "mk20dn64.h"
#include "core_pins.h"

#include <stdint.h>

#define MMA8653_ADDRESS    0x1D

#define STATUS             0x00    // Real time status
#define OUT_X_MSB          0x01    // [7:0] are 8 MSBs of 10-bit real-time sample
#define OUT_X_LSB          0x02    // [7:6] are 2 LSBs of 10-bit real-time sample
#define OUT_Y_MSB          0x03    // [7:0] are 8 MSBs of 10-bit real-time sample
#define OUT_Y_LSB          0x04    // [7:6] are 2 LSBs of 10-bit real-time sample
#define OUT_Z_MSB          0x05    // [7:0] are 8 MSBs of 10-bit real-time sample
#define OUT_Z_LSB          0x06    // [7:6] are 2 LSBs of 10-bit real-time sample

#define SYSMOD             0x0B    // Current System Mode
#define INT_SOURCE         0x0C    // Interrupt status
#define WHO_AM_I           0x0D    // Device ID (0x5A)
#define XYZ_DATA_CFG       0x0E    // Dynamic Range Settings

#define PL_STATUS          0x10    // Landscape/Portrait orientation status
#define PL_CFG             0x11    // Landscape/Portrait configuration.
#define PL_COUNT           0x12    // Landscape/Portrait debounce counter
#define PL_BF_ZCOMP        0x13    // Back/Front, Z-Lock Trip threshold
#define PL_THS_REG         0x14    // Portrait to Landscape Trip angle
#define FF_MT_CFG          0x15    // Freefall/Motion functional block configuration
#define FF_MT_SRC          0x16    // Freefall/Motion event source register
#define FF_MT_THS          0x17    // Freefall/Motion threshold register
#define FF_MT_COUNT        0x18    // Freefall/Motion debounce counter

#define ASLP_COUNT         0x29    // Counter setting for Auto-SLEEP/WAKE
#define CTRL_REG1          0x2A    // Data Rates, ACTIVE Mode.
#define CTRL_REG2          0x2B    // Sleep Enable, OS Modes, RST, ST
#define CTRL_REG3          0x2C    // Wake from Sleep, IPOL, PP_OD
#define CTRL_REG4          0x2D    // Interrupt enable register
#define CTRL_REG5          0x2E    // Interrupt pin (INT1/INT2) map
#define OFF_X              0x2F    // X-axis offset adjust
#define OFF_Y              0x30    // Y-axis offset adjust
#define OFF_Z              0x31    // Z-axis offset adjust


#define XYZ_DATA_CFG_2G    0x00
#define XYZ_DATA_CFG_4G    0x01
#define XYZ_DATA_CFG_8G    0x02

#define CTRL_REG1_ACTIVE   0x01                            // Full-scale selection
#define CTRL_REG1_F_READ   0x02                            // Fast Read Mode
#define CTRL_REG1_DR(n)    (uint8_t)(((n) & 0x07) << 3)    // Data rate selection
#define CTRL_REG1_ASLP_RATE(n) (uint8_t)(((n) & 0x03) << 6)// Auto-WAKE sample frequency

#define CTRL_REG2_ST       0x80                            // Self-Test Enable
#define CTRL_REG2_RST      0x40                            // Software Reset
#define CTRL_REG2_SMODS(n) (uint8_t)(((n) & 0x03) << 3)    // SLEEP mode power scheme selection
#define CTRL_REG2_SLPE     0x02                            // Auto-SLEEP enable
#define CTRL_REG2_MODS(n)  (uint8_t)(((n) & 0x03))         // ACTIVE mode power scheme selection


class WIRE {
private:
    int remaining;
public:
    void begin();
    void beginTransmission(uint8_t address);
    void endTransmission(bool stop);
    void write(uint8_t data);
    void requestFrom(uint8_t address, int length);
    uint8_t receive();
    bool available();
};

void WIRE::begin() {
    SIM_SCGC4 |= SIM_SCGC4_I2C0;    // Enable the I2C0 clock
    
    I2C0_F = 0x1B;                  // Set transmission speed (100KHz?)
    I2C0_C1 = I2C_C1_IICEN;         // Enable I2C

    // TODO: Set pin muxes!
    PORTB_PCR0 = PORT_PCR_MUX(2);
    PORTB_PCR1 = PORT_PCR_MUX(2);
    
}

void waitForDone() {
    while((I2C0_S & I2C_S_IICIF) == 0) {}

    I2C0_S |= I2C_S_IICIF;
}

void WIRE::beginTransmission(uint8_t address) {
    I2C0_C1 |= I2C_C1_TX;
    I2C0_C1 |= I2C_C1_MST;

    write(address << 1);
}


void WIRE::endTransmission(bool stop = true) {
    if(stop) {
        I2C0_C1 &= ~(I2C_C1_MST);
        I2C0_C1 &= ~(I2C_C1_TX);

        while(I2C0_S & I2C_S_BUSY) {};
    }
    else {
        I2C0_C1 |= I2C_C1_RSTA;
    }
}

void WIRE::write(uint8_t data) {
    I2C0_D = data;

    waitForDone();
}


void WIRE::requestFrom(uint8_t address, int length) {
    write(address << 1 | 0x01);

    I2C0_C1 &= ~(I2C_C1_TX);    // Set for RX mode, and write the device address

    //TODO: this is a hack?
    remaining = length + 1;

    receive();
}

uint8_t WIRE::receive() {
    if(remaining == 0) {
        return 0;
    }

    if(remaining <= 2) {           // On the last byte, don't ACK
        I2C0_C1 |= I2C_C1_TXAK;
    }

    if(remaining == 1) {
        endTransmission();
        I2C0_C1 &= ~(I2C_C1_TXAK);
    }

    uint8_t read = I2C0_D;

    if(remaining >1) {
        waitForDone();
    }

    remaining--;

    return read;
}

bool WIRE::available() {
    return remaining > 0;
}

WIRE Wire;


void MMA8653::setup() {

// Accelerometer setup
  Wire.begin();
  
  // Reset the device, to put it into a known state.
  Wire.beginTransmission(MMA8653_ADDRESS);
  Wire.write(CTRL_REG2);
  Wire.write(CTRL_REG2_RST);
  Wire.endTransmission();

  delay(1); // Allow the device to reset

  // Check that we're talking to the right kind of device
  Wire.beginTransmission(MMA8653_ADDRESS);
  Wire.write(WHO_AM_I);
  Wire.endTransmission(false);

  Wire.requestFrom(MMA8653_ADDRESS, 1);
  while(Wire.available()) {
    Wire.receive();
    // TODO: Test if this is equal to 0x5A?
  }

  // Configure for 8G sensitivity
  Wire.beginTransmission(MMA8653_ADDRESS);
  Wire.write(XYZ_DATA_CFG);
  Wire.write(XYZ_DATA_CFG_4G);
  Wire.endTransmission();

  // Put in fast-read mode, with 1.56Hz output rate, and activate
  Wire.beginTransmission(MMA8653_ADDRESS);
  Wire.write(CTRL_REG1);
  Wire.write(CTRL_REG1_ACTIVE | CTRL_REG1_F_READ | CTRL_REG1_DR(0));
  Wire.endTransmission();

}

bool MMA8653::getXYZ(int& X, int& Y, int& Z) {

    Wire.beginTransmission(MMA8653_ADDRESS);
    Wire.write(STATUS);
    Wire.endTransmission(false);
    
    Wire.requestFrom(MMA8653_ADDRESS, 4);
 
    if(Wire.available()) {
        Wire.receive();
    }
    if(Wire.available()) {
        X = (int8_t)Wire.receive();
    }
    if(Wire.available()) {
        Y = (int8_t)Wire.receive();
    }
    if(Wire.available()) {
        Z = (int8_t)Wire.receive();
    }

    return true;
}
