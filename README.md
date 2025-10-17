Flash Programmer
----------------

SST39SF010A flash programmer for Raspberry Pi Pico.
Probably could work with other Arduino-compatible boards,
as long as they have enough I/O pins.

Warning: This runs at 3.3v which is out of spec for the
SST39SF010A. In practice it appears to work, but YMMV.

This can do 32KiB at a time. If you need more than that,
you can program it in multiple passes, manually changing
the connections of A15+ between each pass.

Pin Connections
---------------

```
   Pico       SST39SF010A
+---------+   +-------+
|3v3(OUT) |---| VDD   |
|     GND |---| VSS   |
|     GND |---| /CE   |
|   GP0-7 |---| D0-7  |
|  GP8-22 |---| A0-14 |
|     GND |---| A15+  |
|    GP26 |---| /OE   |
|    GP27 |---| /WE   |
+---------+   +-------+
```
