# CenturioBridge — UE5 Plugin

Connects an Unreal Engine 5 solar-system simulation to the Bifrost-Centurio
flight software and captures spacecraft OpNav imagery for the ground system.

```
cFS (centurio_nav) ──> ue5_bridge ──UDP 15002──> UCenturioBridgeSubsystem
                            ▲                          │ nav state
                            │                          ▼
                       UDP 15001 <────────── ACenturioOpNavCamera
                     (commands)                        │
                                                       ▼
                                    <OutputDirectory>/opnav_*.png + .json
                                                       │
                                                       ▼
                                 tools/giant_integration/ue5_to_giant.py
                                        (GIANT position estimation)
```

## Contents

| Class | Purpose |
|---|---|
| `UCenturioBridgeSubsystem` | GameInstance subsystem. Listens for `{"type":"nav",...}` JSON from the cFS `ue5_bridge` app (UDP 15002), exposes the nav state to Blueprints/C++ (`GetNavState`, `OnNavUpdated`), and sends commands back to the bridge (UDP 15001): `SendSetMode`, `SendSetThrottle`, `SendSetTargetPosition/Velocity/Attitude`. |
| `ACenturioOpNavCamera` | Placeable camera actor. Attach to your spacecraft; on a timer (default 1 Hz) it renders a SceneCapture2D, saves a PNG, and writes the GIANT sidecar JSON from the latest nav state. |
| `FCenturioNavState` | Blueprint-visible mirror of centurio_nav HK (lat/lon/alt, NED velocity, yaw/pitch/roll, throttle, status, fix). |

## Installation

1. Copy (or symlink/junction) this folder into your UE5 project:
   `YourProject/Plugins/CenturioBridge/`
2. Regenerate project files and build (C++ project required), or enable the
   plugin in `Edit > Plugins > Simulation > Centurio Bridge`.
3. The subsystem starts automatically with the game instance — no level setup
   needed for the link itself.

## Using in a solar-system level

1. Drag a **Centurio OpNav Camera** actor into the level and attach it to your
   spacecraft actor (or add it as a child actor component), oriented along the
   desired boresight.
2. Configure on the actor's *Centurio|OpNav* category:
   - `Capture Interval Seconds` (default 1.0)
   - `Image Width/Height` (default 1024x1024)
   - `Field Of View Deg` (default 90 — recorded in the sidecar so the ground
     builds the correct GIANT camera model)
   - `Output Directory` — where PNG+JSON pairs land. Default
     `<ProjectSaved>/CenturioOpNav`. Point it at a directory (or network
     share) that the ground system can ingest.
   - `Target Name` — optional, e.g. `EARTH`, recorded for GIANT relative OpNav.
3. Start the cFS side (`core-cpu1` with `ue5_bridge` + `centurio_nav`) and hit
   Play. Captures begin as soon as nav telemetry is flowing
   (`bRequireNavLink` skips frames when the link is down).
4. Ground side:
   ```sh
   python tools/giant_integration/ue5_to_giant.py <OutputDirectory>   # validate
   ```
   then build a GIANT camera with `build_camera()` for OpNav processing.

Endpoints can be changed in `Config/DefaultGame.ini`:

```ini
[/Script/CenturioBridge.CenturioBridgeSubsystem]
NavListenPort=15002
BridgeHost=127.0.0.1
BridgeCommandPort=15001
```

These defaults match `UE5_BRIDGE_JSON_TX_PORT` / `UE5_BRIDGE_JSON_RX_PORT` in
[apps/ue5_bridge](../../../apps/ue5_bridge/config/default_ue5_bridge_platform_cfg.h).

## Notes

* The camera-frame convention expected by the ground adapter is documented in
  [tools/giant_integration/README.md](../../giant_integration/README.md);
  by default the sidecar attitude describes the spacecraft body (nose camera).
* Pixel readback is synchronous — fine at OpNav rates (~1 Hz); avoid
  sub-100 ms intervals at large resolutions.
* Commands sent from UE5 (e.g. `SendSetMode`) go through the same bridge path
  the ground system uses, so they show up in cFS command counters/events.
