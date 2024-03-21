<h1 align = "center"> Dash_THMI</h1>

Civic EF dashboard display project.

It displays:
- Fuel level (from stock sensor)
- Coolant temperatue. External sensor. Could be installed in stock place in case of D-series engine.
- Oil temperature. External additional sensor required.
- Oil pressure. External additional sensor required.
- Fuel pressure. External additional sensor required.
- Onboard voltage.
All these data are divided to 2 screens. Screens could be switched with button.

I used Lilygo T-HMI ESP32S3 dev board as it fits perfectly into EF's dashboard housing. 
It stands there upside down.
Some minor modifications of this board are required though:
1. Solder out "left" grove connector (when looking at the display upside down)
2. Solder out all buttons
These components slightly protrude beyond the board and prevent the correct installation of the display into the housing

PCB for peripherals and power supply is included in PCB folder (SprintLayout 6 format). 
There is also mirrored PDF version for PCB etching using Laser Printer

Two external 16bit ADCs are used to make measurements more precise - ADS1115.

There is separate power supply for display and resistive sensors pullup.


It utilize standatrd dashboard housing holes and contacs for mounting, power supply and fuel level sensor input.
Other sensors are connected by wires, which come out

There is also a display spacer model included for 3dPrinting.

Project is for VSCode with PlatformIO.
