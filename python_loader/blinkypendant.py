"""BlinkyPendant Python communication library.

  This code assumes stock serialLoop() in the firmware.
"""

import serial
import listports
import time

class BlinkyPendant(object):
    def __init__(self, port=None, ledCount=10, buffered=True):
        """Creates a BlinkyPendant object and opens the port.

        Parameters:
          port
            Optional, port name as accepted by PySerial library:
            http://pyserial.sourceforge.net/pyserial_api.html#serial.Serial
            It is the same port name that is used in Arduino IDE.
            Ex.: COM5 (Windows), /dev/ttyACM0 (Linux).
            If no port is specified, the library will attempt to connect
            to the first port that looks like a BlinkyPendant.

        """

        # If a port was not specified, try to find one and connect automatically
        if port == None:
            ports = listports.listPorts()
            if len(ports) == 0:
                raise IOError("BlinkyPendant not found!")
        
            port = listports.listPorts()[0]

        self.port = port                # Path of the serial port to connect to
        self.ledCount = ledCount        # Number of LEDs on the BlinkyTape
        self.buffered = buffered        # If Frue, buffer output data before sending
        self.buf = ""                   # Color data to send
        self.serial = serial.Serial(port, 115200)


    def sendCommand(self, command):
        
        controlEscapeSequence = ""
        for i in range(0,10):
            controlEscapeSequence += chr(255);

        self.serial.write(controlEscapeSequence)
        self.serial.write(command)
        self.serial.flush()

        # give a small pause and wait for data to be returned
        time.sleep(.01)
        ret = self.serial.read(2)

        status = (ret[0] == 'P')
        returnData = ""
        returnData = self.serial.read(ord(ret[1]) + 1)

        if not status:
            print "Error during write, got:",
            for d in returnData:
                print ord(d),
            print

        self.serial.flushInput()
        return status, returnData

    def startWrite(self):
        """Initiate pattern write
        """
        command = chr(0x01)

        status, returnData = self.sendCommand(command)
        return status

    def write(self, data):
        """Write one packet (64 bytes) of pattern data
        """
        if len(data) != 64:
            return False

        command = chr(0x02)
        command += data

        status, returnData = self.sendCommand(command)
        return status

    def stopWrite(self):
        """Stope pattern write
        """
        command = chr(0x03)

        status, returnData = self.sendCommand(command)
        return status



    def close(self):
        """Safely closes the serial port."""
        self.serial.close()


# Example code

if __name__ == "__main__":

    import glob
    import optparse

    parser = optparse.OptionParser()
    parser.add_option("-p", "--port", dest="portname",
                      help="serial port (ex: /dev/ttyUSB0)", default=None)
    (options, args) = parser.parse_args()

    port = options.portname

    LED_COUNT = 20

    bt = BlinkyPendant(port, LED_COUNT, True)

    print "starting write"
    if not bt.startWrite():
        print "Error starting write"
        exit(1)

    # Make a simple animation
    data = ''
    for step in range(0,10):
        for pixel in range(0,10):
            if step == pixel:
                data += chr(100);
                data += chr(0);
                data += chr(100);
            else:
                data += chr(0);
                data += chr(20);
                data += chr(20);
   
    # pad it out to a 1k sector
    while(len(data)%1024 != 0):
        data += chr(255)

    print "writing"
    for i in range(0, len(data)/64):
        if not bt.write(data[i*64:(i+1)*64]):
            print "Error writing"
            exit(1)

    print "stopping write"
    if not bt.stopWrite():
        print "Error stopping write"
        exit(1)
