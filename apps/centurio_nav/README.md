centurio_nav
============

Mission-specific navigation/control app for the Centurio UE5-in-the-loop scenario.

Provides: housekeeping telemetry with nav/status, and simple commands to set mode,
target position/velocity/attitude, and throttle. Telemetry is mirrored to UE5 by
the ue5_bridge app as JSON over UDP.

Sending commands from GroundSystem (Windows)
-------------------------------------------

This repo includes Command System pages for Centurio Nav:

- "Centurio Nav (CMD)" for command MID 0x1892 (No-Op, Reset, Set Mode/Targets/Throttle)
- "Centurio Nav (SendHK)" for housekeeping request MID 0x1893 (Send HK)

To use on Windows without launching the full GroundSystem (ZeroMQ ipc is not supported by default on Windows):

1) Create the header offset file required by the GUIs:
	- Run: tools/cFS-GroundSystem/init_offsets_win.py
2) Start the Command System directly:
	- Run: tools/cFS-GroundSystem/Subsystems/cmdGui/CommandSystem.py
3) In the UI, select either "Centurio Nav (CMD)" or "Centurio Nav (SendHK)" and press "Display Page".
4) Use the Send buttons to transmit commands to 127.0.0.1:1234 (CI_LAB default) with LE endianness.

Notes
-----
- Telemetry page "Centurio Nav HK" (APID 0x894) is available under the Telemetry System.
- If your mission platform remaps MIDs/APIDs, update the IDs in tools/cFS-GroundSystem/Subsystems/cmdGui/command-pages.txt.
- Commands with floating-point parameters (lat/lon/vel/att/throttle) are supported by the updated MiniCmdUtil.
