Ardulight (lightpack clone) wireless (via Wi-Fi virtual COM port).

Worked with Prismatic in Ardulight mode and esp-8266 (tested with nodeMCU v3 and Wemos).

Solder control to D2 pin via 320R resistor, and LED power to 5V power source (no 3,3v-5V level shifter needed).
Number of leds must be equal to capture zones of Prizmatic! (i.e. in case of 30 led strip zone count must be 30).
I'm using Tibbo Virtual Serial Port driver to emulate serial port via UDP.

This project are using Adafruit Neopixel library
https://github.com/adafruit/Adafruit_NeoPixel
And UDP library
https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/udp-examples.rst
