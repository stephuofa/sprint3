
# SPRINT-3 Data Acquisition Software 


This repository contains the software for the
Space Particle Radiation Identification with Novel Timepix-3 technology (SPRINT-3) Mission.

It builds on the
[Katherine Control Library](https://github.com/petrmanek/libkatherine/tree/master/c)
with slight modifications allowing for static linking and timeout on absence of hits.

<br>
## Documentation
To build and read the docs, from the top level directory (sprint3), run:<br>
`doxygen Doxyfile`

Then open `docs/html/index.html` with your browser of choice.

<br>
## Building
To build and run the main program:

On Windows:<br>
`./scripts/build.ps1 [Flags...]`<br>
`.build/bin/<BuildMode>/sprint.exe <acq_time_seconds> [-v (for verbose)]`

On Linux:<br>
`./scripts/build.sh [Flags...]`<br>
`./build/bin/sprint <acq_time_seconds> [-v (for verbose)]`

Available Flags:
- -clean (cleans before rebuilding)
- -release (builds in release mode, debug is default)
- -test (builds test)

<br>
## Testing
To build and run tests:<br>

On Windows:<br>
`./scripts/build.ps1 -test`<br>
`./scripts/test.ps1`

On Linux:<br>
`./scripts/build.sh -test`<br>
`./scripts/test.sh`

<br>
## Known Issues and Workarounds

<br>
### Failure to Retrieve Chip ID (Chip ID is zero)
Due to a known firmware issue with the Katherine Readout Device, sometimes the HardPix
fails to respond with the proper chip ID, but instead responds with an all 0 chip ID.

This issue can be solved by powercycling the HardPix.
Our software automatically powercycles the HardPix in case of this issue.

<br>
### Failure to Retrieve Acquision Data
Rarely, the HardPix will fail to send *any* acquisition data, despite successfully
responding with its chip ID.

This issue can be solved by powercycling the HardPix.
Our software automatically powercycles the HardPix and restarts the acquision if more
than 60 seconds passes without receiving any data from the HardPix.

<br>
### UDP Packet Drop Bug
Sporatically, a small number of UDP data packets from the HardPix to the PC are not recieved by the application.

Monitoring the connection with Wireshark indicates that the UDP packets are being received by the PC.

Example output from the minimal executable (minimalEx.cpp):<br>
![alt text](./img/udp_drop.png)

To build and run the minimal executable:
1. Modify minimalEx.cpp to match the configurations required for your HardPix, and add your chip config to the core folder
2. Build the executable <br>
On unix systems: `./scripts/build.sh -min` <br>
On windows systems `./scripts/build.ps1 -min`
3. Run the executable <br>
On unix systems: `.\build\bin\minEx` <br>
On windows systems: `.\build\bin\Debug\minEx.exe`

<br>
This bug has yet to be solved, but has minimal impact of application results.