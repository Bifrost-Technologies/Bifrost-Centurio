# GIANT Integration — Image-Based Position Tracking

Adapter between UE5 game-camera captures and [GIANT](../giant/) (Goddard Image
Analysis and Navigation Tool) so the ground system can estimate the satellite's
position from imagery, independent of the onboard nav solution.

## Pipeline

```
centurio_nav (HK) ──SB──> ue5_bridge ──UDP JSON──> UE5
                                                    │  SceneCapture2D
                                                    ▼
                                       captures/NNNN.png + NNNN.json
                                                    │
                                                    ▼
                                  ue5_to_giant.py  (this tool)
                                                    │
                                                    ▼
                              GIANT UE5Camera / OpNavImage objects
                                                    │
                                                    ▼
                        GIANT OpNav (stellar / relative / limb-based)
```

GIANT reads `.png`/`.jpeg` natively (`OpNavImage.load_image`). What it
additionally requires per image is metadata — observation time, camera
attitude quaternion, camera position/velocity in an inertial frame, and
exposure — which this adapter supplies from a JSON **sidecar file** written
next to each screenshot.

## Sidecar contract (what UE5 must write)

For each capture `captures/0001.png`, write `captures/0001.json`:

```json
{
  "type": "opnav_image",
  "time_utc": "2026-07-03T12:00:00.000Z",
  "exposure_s": 0.01,
  "fov_deg": 90.0,
  "lat": 37.6213, "lon": -122.3790, "alt_m": 120.0,
  "vn": 10.0, "ve": 0.0, "vd": 0.0,
  "yaw": 45.0, "pitch": 0.0, "roll": 0.0,
  "target": "EARTH"
}
```

* `lat/lon/alt_m`, `vn/ve/vd`, `yaw/pitch/roll` are copied from the most
  recent `{"type":"nav", ...}` datagram that ue5_bridge already streams to
  UE5 — the UE5 capture code just echoes the last one received.
* `fov_deg` is the UE5 camera's horizontal `FieldOfView`.
* `time_utc` is the capture wall-clock time (ISO-8601).
* Optional: `temperature_c`, `instrument`, `spacecraft`.

`write_sidecar()` in `ue5_to_giant.py` implements this schema and can be used
by Python test harnesses; the UE5 Blueprint/C++ capture code should produce
the same JSON.

## Frame conventions

* GIANT's "inertial" frame here is **ECEF (WGS84), meters**.
* `yaw/pitch/roll` are body-from-NED, ZYX order (same as centurio_nav HK).
* GIANT camera frame: +Z boresight, +X right, +Y down. Default mount is a
  nose camera (boresight = body +X); use `NADIR_CAMERA_FROM_BODY` or a custom
  matrix via `UE5Image.camera_from_body` for other mounts.

## Usage

Validate a capture directory:

```sh
python ue5_to_giant.py /path/to/captures
```

Build a GIANT camera and run OpNav:

```python
from ue5_to_giant import build_camera
from giant.opnav_class import OpNav

camera = build_camera('/path/to/captures')     # FOV read from sidecars
opnav = OpNav(camera)
# ... proceed with stellar_opnav / relative_opnav as in tools/giant/examples
```

Requires the GIANT package importable (`pip install -e ../giant` or add it to
`PYTHONPATH`), plus its dependencies (`numpy`, `opencv`).
