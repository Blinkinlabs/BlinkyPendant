/*
 * Simple ARM debug interface for Arduino, using the SWD (Serial Wire Debug) port.
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
#include <stdarg.h>
#include "arm_debug.h"


ARMDebug::ARMDebug(unsigned clockPin, unsigned dataPin, LogLevel logLevel)
    : clockPin(clockPin), dataPin(dataPin), logLevel(logLevel),
      fastPins(dataPin == ARMDEBUG_FAST_DATA_PIN && clockPin == ARMDEBUG_FAST_CLOCK_PIN)
{}

bool ARMDebug::begin()
{
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT_PULLUP);

    // Invalidate cache
    cache.select = 0xFFFFFFFF;

    // Put the bus in a known state, and trigger a JTAG-to-SWD transition.
    wireWriteTurnaround();
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0xE79E, 16);
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0xFFFFFFFF, 32);
    wireWrite(0, 32);
    wireWrite(0, 32);

    uint32_t idcode;
    if (!getIDCODE(idcode))
        return false;
    log(LOG_NORMAL, "Found ARM processor debug port (IDCODE: %08x)", idcode);

    if (!debugPortPowerup())
        return false;

    if (!debugPortReset())
        return false;

    if (!initMemPort())
        return false;

    return true;
}

bool ARMDebug::getIDCODE(uint32_t &idcode)
{
    // Retrieve IDCODE
    if (!dpRead(IDCODE, false, idcode)) {
        log(LOG_ERROR, "No ARM processor detected. Check power and cables?");
        return false;
    }
    
    // Verify debug port part number only. This isn't allowed to change, and it's a good early sanity check.
    if ((idcode & 0x0FF00001) != 0x0ba00001) {
        // For reference, the K20's IDCODE is 0x4ba00477 over JTAG vs. 0x2ba01477 over SWD.
        log(LOG_ERROR, "ARM Debug Port has an incorrect part number (IDCODE: %08x)", idcode);
        return false;
    }

    return true;
}

bool ARMDebug::debugPortPowerup()
{
    // Initialize CTRL/STAT, request system and debugger power-up
    if (!dpWrite(CTRLSTAT, false, CSYSPWRUPREQ | CDBGPWRUPREQ))
        return false;

    // Wait for power-up acknowledgment
    uint32_t ctrlstat;
    if (!dpReadPoll(CTRLSTAT, ctrlstat, CDBGPWRUPACK | CSYSPWRUPACK, -1)) {
        log(LOG_ERROR, "ARMDebug: Debug port failed to power on (CTRLSTAT: %08x)", ctrlstat);
        return false;
    }

    return true;
}

bool ARMDebug::debugPortReset()
{
    // Reset the debug access port
    if (!dpWrite(CTRLSTAT, false, CSYSPWRUPREQ | CDBGPWRUPREQ | CDBGRSTREQ))
        return false;

    // Wait for reset acknowledgment
    uint32_t ctrlstat;
    if (!dpReadPoll(CTRLSTAT, ctrlstat, CDBGPWRUPACK | CSYSPWRUPACK | CDBGRSTACK, -1)) {
        log(LOG_ERROR, "ARMDebug: Debug port failed to reset (CTRLSTAT: %08x)", ctrlstat);
        return false;
    }

    // Clear reset request bit (leave power requests on)
    if (!dpWrite(CTRLSTAT, false, CSYSPWRUPREQ | CDBGPWRUPREQ))
        return false;

    return true;
}

bool ARMDebug::initMemPort()
{
    // Make sure the default debug access port is an AHB (memory bus) port, like we expect
    uint32_t idr;
    if (!apRead(MEM_IDR, idr))
        return false;
    if ((idr & 0xF) != 1) {
        log(LOG_ERROR, "ARMDebug: Default access port is not an AHB-AP as expected! (IDR: %08x)", idr);
        return false;
    }

    // Invalidate CSW cache
    cache.csw = -1;

    return true;
}

bool ARMDebug::memWriteCSW(uint32_t data)
{
    // Write to the MEM-AP CSW. Duplicate writes are ignored, and the cache is updated.

    if (data == cache.csw)
        return true;

    if (!apWrite(MEM_CSW, data | CSW_DEFAULTS))
        return false;

    cache.csw = data;
    return true;
}

bool ARMDebug::memWait()
{
    // Wait for a transaction to complete & the port to be enabled.

    uint32_t csw;
    if (!apReadPoll(MEM_CSW, csw, CSW_TRIN_PROG | CSW_DEVICE_EN, CSW_DEVICE_EN)) {
        log(LOG_ERROR, "ARMDebug: Error while waiting for memory port");
        return false;
    }

    return true;
}

bool ARMDebug::memStore(uint32_t addr, uint32_t data)
{
    return memStore(addr, &data, 1);
}

bool ARMDebug::memLoad(uint32_t addr, uint32_t &data)
{
    return memLoad(addr, &data, 1);
}

bool ARMDebug::memStoreAndVerify(uint32_t addr, uint32_t data)
{
    return memStoreAndVerify(addr, &data, 1);
}

bool ARMDebug::memStoreAndVerify(uint32_t addr, const uint32_t *data, unsigned count)
{
    if (!memStore(addr, data, count))
        return false;

    if (!memWait())
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    while (count) {
        uint32_t readback;

        if (!memWait())
            return false;
        if (!apRead(MEM_DRW, readback))
            return false;

        log(readback == *data ? LOG_TRACE_MEM : LOG_ERROR,
            "MEM Verif [%08x] %08x (expected %08x)", addr, readback, *data);

        if (readback != *data)
            return false;

        data++;
        addr++;
        count--;
    }

    return true;    
}

bool ARMDebug::memStore(uint32_t addr, const uint32_t *data, unsigned count)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_32BIT | CSW_ADDRINC_SINGLE))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    while (count) {
        log(LOG_TRACE_MEM, "MEM Store [%08x] %08x", addr, *data);

        if (!memWait())
            return false;
        if (!apWrite(MEM_DRW, *data))
            return false;

        data++;
        addr++;
        count--;
    }

    return true;
}

bool ARMDebug::memLoad(uint32_t addr, uint32_t *data, unsigned count)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_32BIT | CSW_ADDRINC_SINGLE))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    while (count) {
        if (!memWait())
            return false;
        if (!apRead(MEM_DRW, *data))
            return false;

        log(LOG_TRACE_MEM, "MEM Load  [%08x] %08x", addr, *data);

        data++;
        addr++;
        count--;
    }

    return true;
}

bool ARMDebug::memStoreByte(uint32_t addr, uint8_t data)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_8BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    log(LOG_TRACE_MEM, "MEM Store [%08x] %02x", addr, data);

    // Replicate across lanes
    uint32_t word = data | (data << 8) | (data << 16) | (data << 24);

    return apWrite(MEM_DRW, word);
}

bool ARMDebug::memLoadByte(uint32_t addr, uint8_t &data)
{
    uint32_t word;
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_8BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;
    if (!apRead(MEM_DRW, word))
        return false;

    // Select the proper lane
    data = word >> ((addr & 3) << 3);

    log(LOG_TRACE_MEM, "MEM Load  [%08x] %02x", addr, data);
    return true;
}

bool ARMDebug::memStoreHalf(uint32_t addr, uint16_t data)
{
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_16BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;

    log(LOG_TRACE_MEM, "MEM Store [%08x] %04x", addr, data);

    // Replicate across lanes
    uint32_t word = data | (data << 16);

    return apWrite(MEM_DRW, word);
}

bool ARMDebug::memLoadHalf(uint32_t addr, uint16_t &data)
{
    uint32_t word;
    if (!memWait())
        return false;
    if (!memWriteCSW(CSW_16BIT))
        return false;
    if (!apWrite(MEM_TAR, addr))
        return false;
    if (!apRead(MEM_DRW, word))
        return false;

    // Select the proper lane
    data = word >> ((addr & 2) << 3);

    log(LOG_TRACE_MEM, "MEM Load  [%08x] %04x", addr, data);
    return true;
}

bool ARMDebug::apWrite(unsigned addr, uint32_t data)
{
    log(LOG_TRACE_AP, "AP  Write [%x] %08x", addr, data);
    return dpSelect(addr) && dpWrite(addr, true, data);
}

bool ARMDebug::apRead(unsigned addr, uint32_t &data)
{
    /*
     * This is really hard to find in the docs, but AP reads are delayed by one transaction.
     * So, the very first AP read will return undefined data, the next AP read returns the
     * data for the previous address, and so on.
     *
     * See:
     *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0314h/ch02s04s03.html
     *
     * To make this easier to deal with, we issue a real AP read followed by a dummy read.
     * The first read will give us the previous dummy result, the second read to a dummy
     * address will give us the data we care about.
     *
     * Performance for this will be kinda sucky, but so far I don't care.
     */

    unsigned dummyAddr = MEM_IDR;  // Something with no side effects
    uint32_t dummyData;

    bool result = dpSelect(addr) && dpRead(addr, true, dummyData) &&
                  dpSelect(dummyAddr) && dpRead(dummyAddr, true, data);

    if (result) {
        log(LOG_TRACE_AP, "AP  Read  [%x] %08x", addr, data);
    }

    return result;
}

bool ARMDebug::dpReadPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!dpRead(addr, false, data))
            return false;
        if ((data & mask) == expected)
            return true;
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling DP ([%08x] & %08x == %08x). Current value: %08x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::apReadPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!apRead(addr, data))
            return false;
        if ((data & mask) == expected)
            return true;
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling AP ([%08x] & %08x == %08x). Current value: %08x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::memPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!memLoad(addr, data))
            return false;
        if ((data & mask) == expected)
            return true;
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling MEM ([%08x] & %08x == %08x). Current value: %08x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::memPollByte(unsigned addr, uint8_t &data, uint8_t mask, uint8_t expected, unsigned retries)
{
    expected &= mask;
    do {
        if (!memLoadByte(addr, data))
            return false;
        if ((data & mask) == expected)
            return true;
    } while (retries--);

    log(LOG_ERROR, "ARMDebug: Timed out while polling MEM ([%08x] & %02x == %02x). Current value: %02x",
        addr, mask, expected, data);
    return false;
}

bool ARMDebug::dpSelect(unsigned addr)
{
    /*
     * Select a new access port and/or a bank. Uses only the high byte and
     * second nybble of 'addr'. This is cached, so redundant dpSelect()s have no effect.
     */

    uint32_t select = addr & 0xFF0000F0;
    if (select != cache.select) {
        if (!dpWrite(SELECT, false, select))
            return false;
        cache.select = select;
    }
    return true;
}

bool ARMDebug::dpWrite(unsigned addr, bool APnDP, uint32_t data)
{
    unsigned retries = 10;
    unsigned ack;
    log(LOG_TRACE_DP, "DP  Write [%x:%x] %08x", addr, APnDP, data);

    do {
        wireWrite(packHeader(addr, APnDP, false), 8);
        wireReadTurnaround();
        ack = wireRead(3);
        wireWriteTurnaround();

        switch (ack) {
            case 1:     // Success
                wireWrite(data, 32);
                wireWrite(evenParity(data), 1);
                wireWriteIdle();
                return true;

            case 2:     // WAIT
                wireWriteIdle();
                log(LOG_TRACE_DP, "DP  WAIT response, %d retries left", retries);
                retries--;
                break;

            case 4:     // FAULT
                wireWriteIdle();
                log(LOG_ERROR, "FAULT response during write (addr=%x APnDP=%d data=%08x)",
                    addr, APnDP, data);
                if (!handleFault())
                    log(LOG_ERROR, "Error during fault recovery!");
                return false;

            default:
                wireWriteIdle();
                log(LOG_ERROR, "PROTOCOL ERROR response during write (ack=%x addr=%x APnDP=%d data=%08x)",
                    ack, addr, APnDP, data);
                return false;
        }
    } while (retries--);

    log(LOG_ERROR, "WAIT timeout during write (addr=%x APnDP=%d data=%08x)",
        addr, APnDP, data);
    return false;
}

bool ARMDebug::dpRead(unsigned addr, bool APnDP, uint32_t &data)
{
    unsigned retries = 10;
    unsigned ack;

    do {
        wireWrite(packHeader(addr, APnDP, true), 8);
        wireReadTurnaround();
        ack = wireRead(3);

        switch (ack) {
            case 1:     // Success
                data = wireRead(32);
                if (wireRead(1) != evenParity(data)) {
                    wireWriteTurnaround();
                    wireWriteIdle();
                    log(LOG_ERROR, "PARITY ERROR during read (addr=%x APnDP=%d data=%08x)",
                        addr, APnDP, data);         
                    return false;
                }
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_TRACE_DP, "DP  Read  [%x:%x] %08x", addr, APnDP, data);
                return true;

            case 2:     // WAIT
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_TRACE_DP, "DP  WAIT response, %d retries left", retries);
                retries--;
                break;

            case 4:     // FAULT
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_ERROR, "FAULT response during read (addr=%x APnDP=%d)", addr, APnDP);
                if (!handleFault())
                    log(LOG_ERROR, "Error during fault recovery!");
                return false;

            default:
                wireWriteTurnaround();
                wireWriteIdle();
                log(LOG_ERROR, "PROTOCOL ERROR response during read (ack=%x addr=%x APnDP=%d)", ack, addr, APnDP);
                return false;
        }
    } while (retries--);

    log(LOG_ERROR, "WAIT timeout during read (addr=%x APnDP=%d)", addr, APnDP);
    return false;
}

bool ARMDebug::handleFault()
{
    // Read CTRLSTAT to see what the fault was, and set appropriate ABORT bits

    uint32_t ctrlstat;
    uint32_t abortbits = 0;
    bool dumpRegs = false;

    if (!dpRead(CTRLSTAT, false, ctrlstat))
        return false;

    if (ctrlstat & (1 << 7)) {
        log(LOG_ERROR, "FAULT: Detected WDATAERR");
        abortbits |= 1 << 3;
    }
    if (ctrlstat & (1 << 4)) {
        log(LOG_ERROR, "FAULT: Detected STICKCMP");
        abortbits |= 1 << 1;
    }
    if (ctrlstat & (1 << 1)) {
        log(LOG_ERROR, "FAULT: Detected STICKORUN");
        abortbits |= 1 << 4;
    }
    if (ctrlstat & (1 << 5)) {
        log(LOG_ERROR, "FAULT: Detected STICKYERR");
        abortbits |= 1 << 2;
        dumpRegs = true;
    }

    if (abortbits && !dpWrite(ABORT, false, abortbits))
        return false;

    if (dumpRegs && !dumpMemPortRegisters()) {
        log(LOG_ERROR, "Failed to dump memory port registers for diagnostics.");
        return false;
    }

    return true;
}

bool ARMDebug::dumpMemPortRegisters()
{
    uint32_t reg;

    if (!apRead(MEM_IDR, reg)) return false;
    log(LOG_ERROR, "  IDR = %08x", reg);

    if (!apRead(MEM_CSW, reg)) return false;
    log(LOG_ERROR, "  CSW = %08x", reg);

    if (!apRead(MEM_TAR, reg)) return false;
    log(LOG_ERROR, "  TAR = %08x", reg);

    return true;
}

uint8_t ARMDebug::packHeader(unsigned addr, bool APnDP, bool RnW)
{
    bool a2 = (addr >> 2) & 1;
    bool a3 = (addr >> 3) & 1;
    bool parity = APnDP ^ RnW ^ a2 ^ a3;
    return (1 << 0)              |  // Start
           (APnDP << 1)          |
           (RnW << 2)            |
           ((addr & 0xC) << 1)   |
           (parity << 5)         |
           (1 << 7)              ;  // Park
}

bool ARMDebug::evenParity(uint32_t word)
{
    word = (word & 0xFFFF) ^ (word >> 16);
    word = (word & 0xFF) ^ (word >> 8);
    word = (word & 0xF) ^ (word >> 4);
    word = (word & 0x3) ^ (word >> 2);
    word = (word & 0x1) ^ (word >> 1);
    return word;
}

void ARMDebug::wireWrite(uint32_t data, unsigned nBits)
{
    log(LOG_TRACE_SWD, "SWD Write %08x (%d)", data, nBits);

    if (fastPins) {
        // Fast path

        while (nBits--) {
            digitalWriteFast(ARMDEBUG_FAST_DATA_PIN, data & 1);
            digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, LOW);
            data >>= 1;
            digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, HIGH);
        }

    } else {
        // Slow (generic) path

        while (nBits--) {
            digitalWrite(dataPin, data & 1);
            digitalWrite(clockPin, LOW);
            data >>= 1;
            digitalWrite(clockPin, HIGH);
        }
    }
}

void ARMDebug::wireWriteIdle()
{
    // Minimum 8 clock cycles.
    wireWrite(0, 32);
}

uint32_t ARMDebug::wireRead(unsigned nBits)
{
    uint32_t result = 0;
    uint32_t mask = 1;
    unsigned count = nBits;

    if (fastPins) {
        // Fast path

        while (count--) {
            if (digitalReadFast(ARMDEBUG_FAST_DATA_PIN)) {
                result |= mask;
            }
            digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, LOW);
            mask <<= 1;
            digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, HIGH);
        }

    } else {
        // Slow (generic) path

        while (count--) {
            if (digitalRead(dataPin)) {
                result |= mask;
            }
            digitalWrite(clockPin, LOW);
            mask <<= 1;
            digitalWrite(clockPin, HIGH);
        }
    }

    log(LOG_TRACE_SWD, "SWD Read  %08x (%d)", result, nBits);
    return result;
}

void ARMDebug::wireWriteTurnaround()
{
    log(LOG_TRACE_SWD, "SWD Write trn");

    if (fastPins) {
        // Fast path

        digitalWriteFast(ARMDEBUG_FAST_DATA_PIN, HIGH);
        pinMode(ARMDEBUG_FAST_DATA_PIN, INPUT_PULLUP);
        digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, LOW);
        digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, HIGH);
        pinMode(ARMDEBUG_FAST_DATA_PIN, OUTPUT);

    } else {
        // Slow (generic) path

        digitalWrite(dataPin, HIGH);
        pinMode(dataPin, INPUT_PULLUP);
        digitalWrite(clockPin, LOW);
        digitalWrite(clockPin, HIGH);
        pinMode(dataPin, OUTPUT);
    }
}

void ARMDebug::wireReadTurnaround()
{
    log(LOG_TRACE_SWD, "SWD Read  trn");

    if (fastPins) {
        // Fast path

        digitalWriteFast(ARMDEBUG_FAST_DATA_PIN, HIGH);
        pinMode(ARMDEBUG_FAST_DATA_PIN, INPUT_PULLUP);
        digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, LOW);
        digitalWriteFast(ARMDEBUG_FAST_CLOCK_PIN, HIGH);

    } else {
        // Slow (generic) path

        digitalWrite(dataPin, HIGH);
        pinMode(dataPin, INPUT_PULLUP);
        digitalWrite(clockPin, LOW);
        digitalWrite(clockPin, HIGH);
    }
}

void ARMDebug::log(int level, const char *fmt, ...)
{
    if (level <= logLevel && Serial) {
        va_list ap;
        char buffer[256];

        va_start(ap, fmt);
        int ret = vsnprintf(buffer, sizeof buffer, fmt, ap);
        va_end(ap);

        Serial.println(buffer);
    }
}

void ARMDebug::hexDump(uint32_t addr, unsigned count, int level)
{
    // Hex dump target memory to the log

    if (level <= logLevel && Serial) {
        va_list ap;
        char buffer[32];
        LogLevel oldLogLevel;

        setLogLevel(LOG_NONE, oldLogLevel);

        while (count) {
            snprintf(buffer, sizeof buffer, "%08x:", addr);
            Serial.print(buffer);

            for (unsigned x = 0; count && x < 4; x++) {
                uint32_t word;
                if (memLoad(addr, word)) {
                    snprintf(buffer, sizeof buffer, " %08x", word);
                    Serial.print(buffer);
                } else {
                    Serial.print(" (error )");
                }

                count--;
                addr += 4;
            }

            Serial.println();
        }

        setLogLevel(oldLogLevel);
    }
}

void ARMDebug::setLogLevel(LogLevel newLevel)
{
    logLevel = newLevel;
}

void ARMDebug::setLogLevel(LogLevel newLevel, LogLevel &prevLevel)
{
    prevLevel = logLevel;
    logLevel = newLevel;
}
