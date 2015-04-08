/*
 * Serial pass-through adapter.
 *
 * This bridges the Teensy's USB serial port to the Fadecandy board's serial
 * interface at 115200 baud.
 */

HardwareSerial Uart = HardwareSerial();

#define BAUD 115200

void setup()
{
    Serial.begin(BAUD);
    Uart.begin(BAUD);
}

void loop()
{
    if (Serial.available()) {
        Uart.write(Serial.read());
    }
    if (Uart.available()) {
        Serial.write(Uart.read());
    }
}
