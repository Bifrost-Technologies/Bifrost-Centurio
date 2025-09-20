# Bifrost Centurio

Centurio is a mission software stack built on NASA's Core Flight System (cFS) that adds navigation, guidance, and simulation-in-the-loop capabilities. It integrates with Unreal Engine 5 (UE5) for mixed-reality simulation and HoloLens 2, and provides a modern Ground System (C#) while retaining the upstream Python Ground System for reference and debugging.

Status: active development; APIs and IDs may evolve. See CONTRIBUTING.md and SECURITY.md for collaboration and reporting guidance.


## What we're building

- Real-time navigation and control app (`centurio_nav`) with mission-specific telemetry and commands
- A lightweight UE5 bridge (`ue5_bridge`) that mirrors cFS navigation telemetry to UE5 as JSON over UDP and accepts JSON control back into cFS
- A C# Ground System (primary) and the upstream Python Ground System (reference) for command/telemetry, table tools, and ops workflows
- A cohesive flight software mission harness using cFS standard services and lab apps (`ci_lab`, `to_lab`, `sch_lab`)


## Repository layout

- `apps/`
	- `centurio_nav/` — mission-specific navigation/control app (telemetry + commands)
	- `ue5_bridge/` — JSON/UDP bridge between cFS and UE5 (in-the-loop simulation)
	- `ci_lab/`, `to_lab/`, `sch_lab/`, `sample_app/` — cFS lab/sample apps used for comms, telemetry output, scheduling, and examples
- `cfe/`, `osal/`, `psp/` — cFS framework, OS abstraction, and platform support
- `libs/` — libraries (e.g., `sample_lib/`)
- `tools/`
	- `bifrost-cFS-GroundSystem/` — C# Ground System (primary)
	- `cFS-GroundSystem/` — Python Ground System (reference; retains upstream pages and utilities)
	- `clients/centurio_json_client.py` — simple UDP JSON client for the UE5 bridge
	- `elf2cfetbl/`, `tblCRCTool/` — table utilities


## Architecture overview

- Core: NASA cFS provides the executive services, messaging (SB), eventing (EVS), scheduling (SCH), and platform/OS abstraction (OSAL/PSP).
- Mission apps:
	- `centurio_nav` publishes housekeeping telemetry (position, attitude, velocities, throttle, status) and handles simple guidance/actuation commands.
	- `ue5_bridge` subscribes to `centurio_nav` telemetry and emits JSON to UE5 over UDP; it receives JSON commands from UE5 (or tools) and forwards them to `centurio_nav` as cFS commands.
- Ground systems:
	- C# Ground System (primary) is the operational UI and tooling going forward.
	- Python Ground System (reference) remains present to cross-check message definitions, command pages, and for quick Windows debugging.


## centurio_nav (navigation/control)

Features
- Housekeeping telemetry: navigation state and status
- Commands: NOOP, reset counters, set mode, set target position/velocity/attitude, set throttle

Key messages (default/topic-based mapping; IDs may vary by mission config)
- Command MID: maps from `CFE_MISSION_CENTURIO_NAV_CMD_TOPICID` (e.g., 0x1892)
- Send HK MID: maps from `CFE_MISSION_CENTURIO_NAV_SEND_HK_TOPICID` (e.g., 0x1893)
- Housekeeping TLM MID/APID: maps from `CFE_MISSION_CENTURIO_NAV_HK_TLM_TOPICID` (e.g., APID 0x0894)

Telemetry payload fields (partial)
- `LatitudeDeg`, `LongitudeDeg`, `AltitudeM`
- `VelNorthMS`, `VelEastMS`, `VelDownMS`
- `YawDeg`, `PitchDeg`, `RollDeg`
- `ThrottlePercent`
- `SystemStatus` (0=INIT,1=SAFE,2=GUIDANCE,3=MANUAL), `NavFixType` (0=NONE,2=2D,3=3D)

Commands and function codes
- `CENTURIO_NAV_NOOP_CC`
- `CENTURIO_NAV_RESET_COUNTERS_CC`
- `CENTURIO_NAV_SET_MODE_CC`      — `Payload.Mode : uint8`
- `CENTURIO_NAV_SET_TARGET_POS_CC` — `Payload.LatDeg : double`, `LonDeg : double`, `AltM : float`
- `CENTURIO_NAV_SET_TARGET_VEL_CC` — `Payload.VN_MS : float`, `VE_MS : float`, `VD_MS : float`
- `CENTURIO_NAV_SET_TARGET_ATT_CC` — `Payload.YawDeg : float`, `PitchDeg : float`, `RollDeg : float`
- `CENTURIO_NAV_SET_THROTTLE_CC`   — `Payload.Percent : float (0–100)`

GroundSystem command pages
- The Python Ground System includes pages for `Centurio Nav (CMD)` and `Centurio Nav (SendHK)` with the above commands and Send HK request.
- On Windows, you can run the Python Command System directly (without ZeroMQ):
	1) Generate the header offset file: `tools/cFS-GroundSystem/init_offsets_win.py`
	2) Launch the Command System: `tools/cFS-GroundSystem/Subsystems/cmdGui/CommandSystem.py`
	3) In the UI, open either "Centurio Nav (CMD)" or "Centurio Nav (SendHK)".
	4) Send to `127.0.0.1:1234` (CI_LAB default) with little-endian.


## ue5_bridge (JSON/UDP link to UE5)

Purpose
- Mirror `centurio_nav` housekeeping telemetry as JSON to UE5
- Accept simple JSON control messages from UE5/tools and forward them into cFS

Networking defaults (configurable at build-time)
- RX (JSON control into cFS): UDP `0.0.0.0:15001` (`UE5_BRIDGE_JSON_RX_PORT`)
- TX (JSON telemetry to UE5): UDP `<UE5 host IP>:15002` (`UE5_BRIDGE_JSON_TX_PORT`), default host `127.0.0.1`

Outgoing JSON (telemetry) example
```json
{
	"type": "nav",
	"lat": 37.61912345,
	"lon": -122.37498765,
	"alt": 120.000,
	"vn": 10.0,
	"ve": 0.0,
	"vd": 0.0,
	"yaw": 45.0,
	"pitch": 0.0,
	"roll": 0.0,
	"thr": 65.0,
	"status": 2,
	"fix": 3
}
```

Accepted JSON commands (send via UDP to RX port)
- `{ "type": "noop" }`
- `{ "type": "reset" }`
- `{ "type": "set_mode", "mode": 0..255 }`
- `{ "type": "set_throttle", "percent": 0..100 }`
- `{ "type": "set_target_pos", "lat": <deg>, "lon": <deg>, "alt_m": <m> }`
- `{ "type": "set_target_vel", "vn": <m/s>, "ve": <m/s>, "vd": <m/s> }`
- `{ "type": "set_target_att", "yaw": <deg>, "pitch": <deg>, "roll": <deg> }`

Quick test tooling
- PowerShell script: `apps/ue5_bridge/test_bridge.ps1`
- Python client: `tools/clients/centurio_json_client.py` (sends a single JSON UDP packet per invocation)


## Ground systems (Ops and debugging)

- C# Ground System (primary): `tools/bifrost-cFS-GroundSystem/`
	- Cross-platform .NET app under active development (UI, command/telemetry, and mission workflows)
	- Use this as the main operator console for Centurio

- Python Ground System (reference): `tools/cFS-GroundSystem/`
	- Upstream, widely used cFS Ground System with command/telemetry pages (retained for validation and debugging)
	- Windows usage note: for local UDP CI/TO use, run the Command System directly as described in the `centurio_nav` section above


## Build and run (high-level)

Centurio follows the cFS build system using CMake. Platform specifics (PSP/OSAL, toolchains, mission configuration) determine the exact steps.

Typical flow
1) Configure a cFS mission build (select PSP/OSAL, include `centurio_nav`, `ue5_bridge`, and lab apps)
2) Build the mission with CMake
3) Launch the cFS core (target platform)
4) Observe/command:
	 - Use the C# Ground System (primary) or Python Ground System (reference)
	 - Or, for quick bridge tests, use `tools/clients/centurio_json_client.py` to send control and listen for UE5 telemetry on UDP 15002

References
- See `cfe/README.md` and `cfe/cmake/` for framework build details
- Each app folder contains `CMakeLists.txt`, `mission_build.cmake`, and configs under `config/`


## UE5 integration notes

- UE5 receives JSON telemetry via UDP on port 15002 (by default) and can send control JSON to 15001
- The default UE5 host IP is set in `apps/ue5_bridge/config/default_ue5_bridge_mission_cfg.h`
- Implement a simple UDP listener in UE5 (e.g., via a plugin or a small C++/Blueprint bridge) to parse the `type:"nav"` JSON
- For closed-loop testing, drive JSON control from UE5 to influence `centurio_nav` state (mode/targets/throttle)


## Contributing, security, license

- Contributions: see `CONTRIBUTING.md`
- Security: see `SECURITY.md`
- License: see `LICENSE`
- Citation: see `CITATION.cff`


## Acknowledgements

Built on NASA's Core Flight System (cFS) and community-provided lab apps/tools.

