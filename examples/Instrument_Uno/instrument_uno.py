#!/usr/bin/env python3

"""
A Python interface to the Instrument_Uno over the Ardunio's USB serial connection.
"""

import logging, serial, time, re
from collections import defaultdict, namedtuple
from threading import Lock

# Start of string expected to be returned from device after an "*IDN?" query
_device_identifier = "SCPI Parser,Instrument Uno"

# Wait some time (in seconds) after serial port opened before sending commands
_open_delay = 2

class InstrumentUno():

    def __init__(self, serial_port):
        """Initialise and open serial port for the device."""
        # Lock to only allow single query to serial port at a time (from multiple threads)
        self.sp_lock = Lock()
        self.log = logging.getLogger(__name__)
        self.log.debug("Initialising serial port ({})...".format(serial_port))
        # Open and configure serial port settings
        self.sp = serial.Serial(port=serial_port,
                         baudrate=115200,
                         parity=serial.PARITY_NONE,
                         stopbits=serial.STOPBITS_ONE,
                         bytesize=serial.EIGHTBITS,
                         timeout=1,
                         write_timeout=1)
        self.log.debug("Opened serial port OK.")

        # Wait a bit before sending first command
        time.sleep(_open_delay)

        # Check if device is an Instrument_Uno
        reply = self._send_command("*IDN?", expect_reply=True)
        if not reply.startswith(_device_identifier):
            raise Exception("Could not query device information! (device responded with {})".format(repr(reply)))
        else:
            self.id = reply

        # Get list of supported commands
        self.commands = re.findall("  ([a-zA-Z:?]+)[\r\n]", self._send_command("HELP?", expect_reply=True))

    def __del__(self):
        self.close()

    def _check_property_name(self, name):
        """Check that a class property name will translate to an device command string."""
        # Swap _ for : and strip numeric suffixes from tokens
        cmd = re.sub("_", ":", name)
        cmd = re.sub("[0-9]+(:|\?|$)", "\g<1>", cmd)
        # Check if the produced command matches any valid one
        try:
            # The __getattribute__ is to prevent infinite recursion
            valid = (cmd.upper() in [x.upper() for x in self.__getattribute__("commands")])
            return valid
        except Exception as ex:
            return False

    def __getattr__(self, name):
        """Intercept unresolved attribute calls and check for valid device properties."""
        if self._check_property_name(name + "?"):
            return self._send_command(name.replace("_", ":") + "?", expect_reply=True)
        return self.__getattribute__(name)

    def __setattr__(self, name, value):
        """Intercept __setattr__ calls for valid device properties."""
        if self._check_property_name(name):
            if type(value) in (tuple, list): value = ",".join([str(x) for x in value])
            self._send_command("{} {}".format(name.replace("_", ":"), value))
        else:
            super().__setattr__(name, value)

    def _send_command(self, command_string, expect_reply=False):
        """Write a command out the the serial port.

        If ``expect_reply`` is true, then also wait for response and return the received string.

        A linefeed ("\n") will be appended to the given ``command_string``."""

        if self.sp == None:
            raise Exception("Can't communicate with device as serial port has been closed!")
        command_string = command_string + "\n"
        self.sp_lock.acquire()
        self.log.debug("Writing command string: {}".format(repr(command_string)))
        self.sp.write(bytearray(command_string, "ascii"))
        indata = ""
        if expect_reply:
            while indata[-2:] != "\r\n":
                try:
                    inbytes = self.sp.read(1)
                except serial.SerialException as ex:
                    self.log.warning("Error reading reply string! (requested '{}', received '{}')".format(repr(command_string), repr(indata)))
                    break
                if len(inbytes) > 0:
                    indata += inbytes.decode("ascii")
                else:
                    self.log.warning("Timeout reading reply string! (requested '{}', received '{}')".format(repr(command_string), repr(indata)))
                    break
            self.log.debug("Received reply string:  {}".format(repr(indata)))
        self.sp_lock.release()
        return indata[:-2]

    def get_commands(self):
        """Return a string containing the list of possible commands the device understands."""
        return self._send_command("HELP?", expect_reply=True)

    def close(self):
        if self.sp != None:
            self.sp.close()
            self.sp = None
