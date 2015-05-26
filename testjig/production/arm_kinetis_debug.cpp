/*
 * Simple ARM debug interface for Arduino, using the SWD (Serial Wire Debug) port.
 * Extensions for Freescale Kinetis chips.
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>
#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"


ARMKinetisDebug::ARMKinetisDebug(unsigned clockPin, unsigned dataPin, LogLevel logLevel)
    : ARMDebug(clockPin, dataPin, logLevel)
{}

bool ARMKinetisDebug::startup()
{
    return detect() && reset() && debugHalt() && peripheralInit();
}

bool ARMKinetisDebug::detect()
{
    // Make sure we're on a compatible chip. The MDM-AP peripheral is Freescale-specific.
    uint32_t idr;
    if (!apRead(REG_MDM_IDR, idr))
        return false;
    if (idr != 0x001C0000) {
        log(LOG_ERROR, "ARMKinetisDebug: Didn't find a supported MDM-AP peripheral");
        return false;
    }

    return true;
}

bool ARMKinetisDebug::reset()
{
    // System resets can be slow, give them more time than the default.
    const unsigned resetRetries = 2000;

    // Put the control register in a known state, and make sure we aren't already in the middle of a reset
    uint32_t status;
    if (!apWrite(REG_MDM_CONTROL, REG_MDM_CONTROL_CORE_HOLD_RESET))
        return false;
    if (!apReadPoll(REG_MDM_STATUS, status, REG_MDM_STATUS_SYS_NRESET, -1, resetRetries))
        return false;

    // System reset
    if (!apWrite(REG_MDM_CONTROL, REG_MDM_CONTROL_SYS_RESET_REQ))
        return false;
    if (!apReadPoll(REG_MDM_STATUS, status, REG_MDM_STATUS_SYS_NRESET, 0))
        return false;
    if (!apWrite(REG_MDM_CONTROL, 0))
        return false;

    // Wait until the flash controller is ready & system is out of reset.
    // Also wait for security bit to be cleared. Early in reset, the chip is determining
    // its security status. When the security bit is set, AHB-AP is disabled.
    if (!apReadPoll(REG_MDM_STATUS, status,
            REG_MDM_STATUS_SYS_NRESET | REG_MDM_STATUS_FLASH_READY | REG_MDM_STATUS_SYS_SECURITY,
            REG_MDM_STATUS_SYS_NRESET | REG_MDM_STATUS_FLASH_READY,
            resetRetries))
        return false;

    return true;
}

bool ARMKinetisDebug::debugHalt()
{

    /*
     * Enable debug, request a halt, and read back status.
     *
     * This part is somewhat timing critical, since we're racing against the watchdog
     * timer. Avoid memWait() by calling the lower-level interface directly.
     *
     * Since this is expected to fail a bunch before succeeding, mute errors temporarily.
     */

    unsigned haltRetries = 10000;
    LogLevel savedLogLevel;
    uint32_t dhcsr;

    // Point at the debug halt control/status register. We disable MEM-AP autoincrement,
    // and leave TAR pointed at DHCSR for the entire loop.
    if (memWriteCSW(CSW_32BIT) && apWrite(MEM_TAR, REG_SCB_DHCSR)) {

        setLogLevel(LOG_NONE, savedLogLevel);

        while (haltRetries) {
            haltRetries--;

            if (!apWrite(MEM_DRW, 0xA05F0003))
                continue;
            if (!apRead(MEM_DRW, dhcsr))
                continue;

            if (dhcsr & (1 << 17)) {
                // Halted!
                break;
            }
        }

        setLogLevel(savedLogLevel);
    }

    if (!haltRetries) {
        log(LOG_ERROR, "ARMKinetisDebug: Failed to put CPU in debug halt state. (DHCSR: %08x)", dhcsr);
        return false;
    }

    return true;
}

bool ARMKinetisDebug::peripheralInit()
{
    /*
     * ARM peripheral initialization, based on the peripheral startup code
     * used in Teensyduino. We set up the same peripherals that FC-Boot sets up.
     */

    uint8_t value;
    return
        // Enable peripheral clocks
        memStore(REG_SIM_SCGC5, 0x00043F82) && // clocks active to all GPIO
        memStore(REG_SIM_SCGC6,
            REG_SIM_SCGC6_RTC | REG_SIM_SCGC6_FTM0 | REG_SIM_SCGC6_FTM1 |
            REG_SIM_SCGC6_ADC0 | REG_SIM_SCGC6_FTFL) &&

        // Start in FEI mode
        // Enable capacitors for crystal
        memStoreByte(REG_OSC0_CR, REG_OSC_SC8P | REG_OSC_SC2P) &&

        // Enable osc, 8-32 MHz range, low power mode
        memStoreByte(REG_MCG_C2, REG_MCG_C2_RANGE0(2) | REG_MCG_C2_EREFS) &&

        // Switch to crystal as clock source, FLL input = 16 MHz / 512
        memStoreByte(REG_MCG_C1, REG_MCG_C1_CLKS(2) | REG_MCG_C1_FRDIV(4)) &&

        // Wait for crystal oscillator to begin
        memPollByte(REG_MCG_S, value, REG_MCG_S_OSCINIT0, -1) &&

        // Wait for FLL to use oscillator
        memPollByte(REG_MCG_S, value, REG_MCG_S_IREFST, 0) &&

        // Wait for MCGOUT to use oscillator
        memPollByte(REG_MCG_S, value, REG_MCG_S_CLKST_MASK, REG_MCG_S_CLKST(2)) &&

        // Now we're in FBE mode
        // Config PLL input for 16 MHz Crystal / 4 = 4 MHz
        memStoreByte(REG_MCG_C5, REG_MCG_C5_PRDIV0(3)) &&

        // Config PLL for 96 MHz output
        memStoreByte(REG_MCG_C6, REG_MCG_C6_PLLS | REG_MCG_C6_VDIV0(0)) &&

        // Wait for PLL to start using xtal as its input
        memPollByte(REG_MCG_S, value, REG_MCG_S_PLLST, -1) &&

        // Wait for PLL to lock
        memPollByte(REG_MCG_S, value, REG_MCG_S_LOCK0, -1) &&

        // Now we're in PBE mode
        // Config divisors: 48 MHz core, 48 MHz bus, 24 MHz flash
        memStore(REG_SIM_CLKDIV1, REG_SIM_CLKDIV1_OUTDIV1(1) |
            REG_SIM_CLKDIV1_OUTDIV2(1) | REG_SIM_CLKDIV1_OUTDIV4(3)) &&

        // Switch to PLL as clock source, FLL input = 16 MHz / 512
        memStoreByte(REG_MCG_C1, REG_MCG_C1_CLKS(0) | REG_MCG_C1_FRDIV(4)) &&

        // Wait for PLL clock to be used
        memPollByte(REG_MCG_S, value, REG_MCG_S_CLKST_MASK, REG_MCG_S_CLKST(3)) &&

        // Now we're in PEE mode
        // Configure USB for 48 MHz clock
        // USB = 96 MHz PLL / 2
        memStore(REG_SIM_CLKDIV2, REG_SIM_CLKDIV2_USBDIV(1)) &&

        // USB uses PLL clock, trace is CPU clock, CLKOUT=OSCERCLK0
        memStore(REG_SIM_SOPT2, REG_SIM_SOPT2_USBSRC | REG_SIM_SOPT2_PLLFLLSEL |
            REG_SIM_SOPT2_TRACECLKSEL | REG_SIM_SOPT2_CLKOUTSEL(6)) &&

        // Enable USB clock gate
        memStore(REG_SIM_SCGC4, REG_SIM_SCGC4_USBOTG) &&

        // Reset USB core
        memStoreByte(REG_USB0_USBTRC0, REG_USB_USBTRC_USBRESET) &&
        memPollByte(REG_USB0_USBTRC0, value, REG_USB_USBTRC_USBRESET, 0) &&

        // Enable USB
        memStoreByte(REG_USB0_CTL, REG_USB_CTL_USBENSOFEN) &&
        memStoreByte(REG_USB0_USBCTRL, 0) &&

        // USB pull-up off for now
        usbSetPullup(false) &&

        // Test AHB-AP: Can we successfully write to RAM?
        testMemoryAccess();
}

bool ARMKinetisDebug::testMemoryAccess()
{
    // Try word-wide stores to SRAM
    if (!memStoreAndVerify(0x20000000, 0x31415927))
        return false;
    if (!memStoreAndVerify(0x20000000, 0x76543210))
        return false;

    // Test byte-wide memory access
    uint32_t word;
    uint8_t byte;
    if (!memStoreByte(0x20000001, 0x55))
        return false;
    if (!memStoreByte(0x20000002, 0x9F))
        return false;
    if (!memLoad(0x20000000, word))
        return false;
    if (word != 0x769F5510) {
        log(LOG_ERROR, "ARMKinetisDebug: Byte-wide AHB write seems broken! (Test word = %08x)", word);
        return false;
    }
    if (!memLoadByte(0x20000003, byte))
        return false;
    if (byte != 0x76) {
        log(LOG_ERROR, "ARMKinetisDebug: Byte-wide AHB read seems broken! (Test byte = %02x)", byte);
        return false;
    }

    // Test halfword-wide memory access
    uint16_t half;
    if (!memStoreHalf(0x20000000, 0x5abc))
        return false;
    if (!memStoreHalf(0x20000002, 0xdef0))
        return false;
    if (!memLoad(0x20000000, word))
        return false;
    if (word != 0xdef05abc) {
        log(LOG_ERROR, "ARMKinetisDebug: Halfword-wide AHB write seems broken! (Test word = %08x)", word);
        return false;
    }
    if (!memLoadHalf(0x20000002, half))
        return false;
    if (half != 0xdef0) {
        log(LOG_ERROR, "ARMKinetisDebug: Halfword-wide AHB read seems broken! (Test half = %04x)", half);
        return false;
    }

    return true;
}

bool ARMKinetisDebug::flashMassErase()
{
    // Erase all flash, even if some of it is protected.

    uint32_t status;
    if (!apRead(REG_MDM_STATUS, status))
        return false;
    if (!(status & REG_MDM_STATUS_FLASH_READY)) {
        log(LOG_ERROR, "FLASH: Flash controller not ready before mass erase");
        return false;
    }
    if ((status & REG_MDM_STATUS_FLASH_ERASE_ACK)) {
        log(LOG_ERROR, "FLASH: Mass erase already in progress");
        return false;
    }
    if (!(status & REG_MDM_STATUS_MASS_ERASE_ENABLE)) {
        log(LOG_ERROR, "FLASH: Mass erase is disabled!");
        return false;
    }

    log(LOG_NORMAL, "FLASH: Beginning mass erase operation");
    if (!apWrite(REG_MDM_CONTROL, REG_MDM_CONTROL_CORE_HOLD_RESET | REG_MDM_CONTROL_MASS_ERASE))
        return false;

    // Wait for the mass erase to begin (ACK bit set)
    if (!apReadPoll(REG_MDM_STATUS, status, REG_MDM_STATUS_FLASH_ERASE_ACK, -1)) {
        log(LOG_ERROR, "FLASH: Timed out waiting for mass erase to begin");
        return false;
    }

    // Wait for it to complete (CONTROL bit cleared)
    uint32_t control;
    if (!apReadPoll(REG_MDM_CONTROL, control, REG_MDM_CONTROL_MASS_ERASE, 0, 10000)) {
        log(LOG_ERROR, "FLASH: Timed out waiting for mass erase to complete");
        return false;
    }

    // Check status again
    if (!apRead(REG_MDM_STATUS, status))
        return false;
    if (!(status & REG_MDM_STATUS_FLASH_READY)) {
        log(LOG_ERROR, "FLASH: Flash controller not ready after mass erase");
        return false;
    }

    log(LOG_NORMAL, "FLASH: Mass erase complete");
    return true;
}

bool ARMKinetisDebug::flashSectorBufferInit()
{
    // Use FlexRAM as normal RAM, and erase it. Test to make sure it's working.
    return
        ftfl_setFlexRAMFunction(0xFF) &&
        memStoreAndVerify(REG_FLEXRAM_BASE, 0x12345678) &&
        memStoreAndVerify(REG_FLEXRAM_BASE, 0xFFFFFFFF) &&
        memStoreAndVerify(REG_FLEXRAM_BASE + FLASH_SECTOR_SIZE - 4, 0xA5559872) &&
        memStoreAndVerify(REG_FLEXRAM_BASE + FLASH_SECTOR_SIZE - 4, 0xFFFFFFFF);
}

bool ARMKinetisDebug::flashSectorBufferWrite(uint32_t bufferOffset, const uint32_t *data, unsigned count)
{
    if (bufferOffset & 3) {
        log(LOG_ERROR, "ARMKinetisDebug::flashSectorBufferWrite alignment error");
        return false;
    }
    if (bufferOffset + (count * sizeof *data) > FLASH_SECTOR_SIZE) {
        log(LOG_ERROR, "ARMKinetisDebug::flashSectorBufferWrite overrun");
        return false;
    }

    return memStore(REG_FLEXRAM_BASE + bufferOffset, data, count);
}

bool ARMKinetisDebug::flashSectorProgram(uint32_t address)
{
    if (address & (FLASH_SECTOR_SIZE-1)) {
        log(LOG_ERROR, "ARMKinetisDebug::flashSectorProgram alignment error");
        return false;
    }

    return ftfl_programSection(address, FLASH_SECTOR_SIZE/4);
}

bool ARMKinetisDebug::ftfl_busyWait()
{
    const unsigned retries = 1000;
    uint32_t fstat;

    if (!memPoll(REG_FTFL_FSTAT, fstat, REG_FTFL_FSTAT_CCIF, -1)) {
        log(LOG_ERROR, "FLASH: Error waiting for flash controller");
        return false;
    }

    return true;
}

bool ARMKinetisDebug::ftfl_launchCommand()
{
    // Begin a flash memory controller command, and clear any previous error status.
    return
        memStoreByte(REG_FTFL_FSTAT, REG_FTFL_FSTAT_ACCERR | REG_FTFL_FSTAT_FPVIOL | REG_FTFL_FSTAT_RDCOLERR) &&
        memStoreByte(REG_FTFL_FSTAT, REG_FTFL_FSTAT_CCIF);
}

bool ARMKinetisDebug::ftfl_setFlexRAMFunction(uint8_t controlCode)
{
    return
        ftfl_busyWait() &&
        memStoreByte(REG_FTFL_FCCOB0, 0x81) &&
        memStoreByte(REG_FTFL_FCCOB1, controlCode) &&
        ftfl_launchCommand() &&
        ftfl_busyWait() &&
        ftfl_handleCommandStatus();
}

bool ARMKinetisDebug::ftfl_programSection(uint32_t address, uint32_t numLWords)
{
    return
        ftfl_busyWait() &&
        memStoreByte(REG_FTFL_FCCOB0, 0x0B) &&
        memStoreByte(REG_FTFL_FCCOB1, address >> 16) &&
        memStoreByte(REG_FTFL_FCCOB2, address >> 8) &&
        memStoreByte(REG_FTFL_FCCOB3, address) &&
        memStoreByte(REG_FTFL_FCCOB4, numLWords >> 8) &&
        memStoreByte(REG_FTFL_FCCOB5, numLWords) &&
        ftfl_launchCommand() &&
        ftfl_busyWait() &&
        ftfl_handleCommandStatus("FLASH: Error verifying sector! (FSTAT: %08x)");
}

bool ARMKinetisDebug::ftfl_handleCommandStatus(const char *cmdSpecificError)
{
    /*
     * Handle common errors from an FSTAT register value.
     * The indicated "errorMessage" is used for reporting a command-specific
     * error from MGSTAT0. Returns true on success, false on error.
     */

    uint32_t fstat;
    if (!memLoad(REG_FTFL_FSTAT, fstat))
        return false;

    if (fstat & FTFL_FSTAT_RDCOLERR) {
        log(LOG_ERROR, "FLASH: Bus collision error (FSTAT: %08x)", fstat);
        return false;
    }

    if (fstat & (FTFL_FSTAT_FPVIOL | FTFL_FSTAT_ACCERR)) {
        log(LOG_ERROR, "FLASH: Address access error (FSTAT: %08x)", fstat);
        return false;
    }

    if (cmdSpecificError && (fstat & FTFL_FSTAT_MGSTAT0)) {
        // Command-specifid error
        log(LOG_ERROR, cmdSpecificError, fstat);
        return false;
    }

    return true;
}

ARMKinetisDebug::FlashProgrammer::FlashProgrammer(
    ARMKinetisDebug &target, const uint32_t *image, unsigned numSectors)
    : target(target), image(image), numSectors(numSectors)
{}

bool ARMKinetisDebug::FlashProgrammer::begin()
{
    nextSector = 0;
    isVerifying = false;

    // Start with a mass-erase
    if (!target.flashMassErase())
        return false;

    // Reset again after mass erase, for new protection bits to take effect
    if (!(target.reset() && target.debugHalt() && target.peripheralInit()))
        return false;

    // Use FlexRAM as normal RAM, for buffering flash sectors
    if (!target.flashSectorBufferInit())
        return false;

    return true;
}

bool ARMKinetisDebug::FlashProgrammer::isComplete()
{
    return isVerifying && nextSector == numSectors;
}

bool ARMKinetisDebug::FlashProgrammer::next()
{
    uint32_t address = nextSector * FLASH_SECTOR_SIZE;
    const uint32_t *ptr = image + (nextSector * FLASH_SECTOR_SIZE/4);

    if (isVerifying) {
        target.log(LOG_NORMAL, "FLASH: Verifying sector at %08x", address);

        uint32_t buffer[FLASH_SECTOR_SIZE/4];
        if (!target.memLoad(address, buffer, FLASH_SECTOR_SIZE/4))
            return false;

        bool okay = true;

        for (unsigned i = 0; i < FLASH_SECTOR_SIZE/4; i++) {
            if (buffer[i] != ptr[i]) {
                target.log(LOG_ERROR, "FLASH: Verify error at %08x. Expected %08x, actual %08x",
                    address + i*4, ptr[i], buffer[i]);
                okay = false;
            }
        }

        if (!okay)
            return false;

        if (++nextSector == numSectors) {
            // Done with verify!
            target.log(LOG_NORMAL, "FLASH: Programming successful!");
        }

    } else {
        target.log(LOG_NORMAL, "FLASH: Programming sector at %08x", address);

        if (!target.flashSectorBufferWrite(0, ptr, FLASH_SECTOR_SIZE/4))
            return false;
        if (!target.flashSectorProgram(address))
            return false;

        if (++nextSector == numSectors) {
            // Done programming. Another reset! Load new protection flags.
            if (!(target.reset() && target.debugHalt() && target.peripheralInit()))
                return false;

            nextSector = 0;
            isVerifying = true;
        }
    }

    return true;
}

static inline uint32_t gpioBitBandAddr(uint32_t addr, unsigned bit)
{
    return (addr - 0x40000000) * 32 + bit * 4 + 0x42000000;
}

static inline uint32_t gpioPortAddr(uint32_t base, unsigned p)
{
    return base + (p >> 12) * (REG_GPIOB_PDOR - REG_GPIOA_PDOR);
}

static inline uint32_t gpioPortBit(unsigned p)
{
    return (p >> 2) & 31;
}

bool ARMKinetisDebug::memStoreBit(uint32_t addr, unsigned bit, uint32_t data)
{
    return memStore(gpioBitBandAddr(addr, bit), data);
}

bool ARMKinetisDebug::memLoadBit(uint32_t addr, unsigned bit, uint32_t &data)
{
    return memLoad(gpioBitBandAddr(addr, bit), data);
}

bool ARMKinetisDebug::pinMode(unsigned p, int mode)
{
    // GPIO, and default drive strength + slew rate
    uint32_t pcrValue = REG_PORT_PCR_MUX(1) | REG_PORT_PCR_DSE | REG_PORT_PCR_SRE;

    // PCR address
    uint32_t pcrAddr = REG_PORTA_PCR0 + p;

    switch (mode) {
        case INPUT_PULLUP:
            // Turn on pullup
            pcrValue |= REG_PORT_PCR_PE | REG_PORT_PCR_PS;
            break;

        case INPUT:
        case OUTPUT:
            // Default PCR value
            break;

        default:
            log(LOG_ERROR, "GPIO: Unsupported pinMode %d", mode);
            return true;
    }

    // Set pin mode
    if (!memStore(pcrAddr, pcrValue))
        return false;

    // Set direction
    return memStoreBit(gpioPortAddr(REG_GPIOA_PDDR, p), gpioPortBit(p), mode == OUTPUT);
}

bool ARMKinetisDebug::digitalWrite(unsigned p, int value)
{
    return memStoreBit(gpioPortAddr(REG_GPIOA_PDOR, p), gpioPortBit(p), value != 0);
}

int ARMKinetisDebug::digitalRead(unsigned p)
{
    uint32_t data;
    if (!memLoadBit(gpioPortAddr(REG_GPIOA_PDIR, p), gpioPortBit(p), data))
        return -1;
    return data;
}

bool ARMKinetisDebug::digitalWritePort(unsigned port, unsigned value)
{
    // Write to all bits on a given port
    return memStore(gpioPortAddr(REG_GPIOA_PDOR, port), value);
}

bool ARMKinetisDebug::usbSetPullup(bool enable)
{
    return memStoreByte(REG_USB0_CONTROL, enable ? USB_CONTROL_DPPULLUPNONOTG : 0);
}
