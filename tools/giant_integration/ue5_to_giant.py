#!/usr/bin/env python3
"""
UE5 -> GIANT OpNav adapter for the Bifrost-Centurio ground system.

Turns UE5 game-camera screenshots (.png / .jpg) plus JSON metadata sidecars
into GIANT ``OpNavImage`` / ``Camera`` objects so the satellite's position can
be estimated from imagery.

Data flow
---------
1. centurio_nav publishes HK telemetry; ue5_bridge mirrors it to UE5 as a
   ``{"type":"nav", ...}`` JSON datagram (lat/lon/alt, vn/ve/vd, yaw/pitch/roll).
2. UE5 captures a SceneCapture2D screenshot and writes a sidecar JSON next to
   it (same basename, ``.json`` extension) using the most recent nav datagram
   plus its camera settings.  See ``write_sidecar`` and the README for the
   schema.
3. On the ground, this module ingests the image+sidecar pairs and produces a
   ``UE5Camera`` ready for GIANT's stellar/relative OpNav pipelines.

Conventions
-----------
* The "inertial" frame used for GIANT metadata is ECEF (WGS84), in meters.
* Vehicle attitude is yaw/pitch/roll (degrees, ZYX order) of the body frame
  relative to local NED, matching the centurio_nav telemetry.
* The GIANT camera frame is +Z boresight, +X right, +Y down.  By default the
  camera boresight is along body +X (nose camera); pass a different
  ``camera_from_body`` matrix for other mounts.
"""

import argparse
import json
import math
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, Union

import numpy as np

from giant.camera import Camera
from giant.camera_models import PinholeModel
from giant.image import OpNavImage
from giant.rotations import Rotation

IMAGE_EXTENSIONS = ('.png', '.jpg', '.jpeg')

# WGS84 ellipsoid
_WGS84_A = 6378137.0
_WGS84_E2 = 6.69437999014e-3

# Camera frame from body frame for a nose-mounted camera:
# camera +Z (boresight) = body +X, camera +X = body +Y, camera +Y = body +Z
NOSE_CAMERA_FROM_BODY = np.array([[0.0, 1.0, 0.0],
                                  [0.0, 0.0, 1.0],
                                  [1.0, 0.0, 0.0]])

# Camera frame from body frame for a nadir (down-looking) camera:
# camera +Z = body +Z (down), camera +X = body +Y, camera +Y = -body +X
NADIR_CAMERA_FROM_BODY = np.array([[0.0, 1.0, 0.0],
                                   [-1.0, 0.0, 0.0],
                                   [0.0, 0.0, 1.0]])


# ---------------------------------------------------------------------------
# frame conversions
# ---------------------------------------------------------------------------

def geodetic_to_ecef(lat_deg: float, lon_deg: float, alt_m: float) -> np.ndarray:
    """Convert WGS84 geodetic coordinates to an ECEF position vector in meters."""
    lat = math.radians(lat_deg)
    lon = math.radians(lon_deg)
    sin_lat = math.sin(lat)
    n = _WGS84_A / math.sqrt(1.0 - _WGS84_E2 * sin_lat * sin_lat)
    x = (n + alt_m) * math.cos(lat) * math.cos(lon)
    y = (n + alt_m) * math.cos(lat) * math.sin(lon)
    z = (n * (1.0 - _WGS84_E2) + alt_m) * sin_lat
    return np.array([x, y, z])


def ecef_from_ned_rotation(lat_deg: float, lon_deg: float) -> np.ndarray:
    """Rotation matrix taking local NED vectors to the ECEF frame."""
    lat = math.radians(lat_deg)
    lon = math.radians(lon_deg)
    sl, cl = math.sin(lat), math.cos(lat)
    so, co = math.sin(lon), math.cos(lon)
    # columns are the NED unit vectors expressed in ECEF
    return np.array([[-sl * co, -so, -cl * co],
                     [-sl * so, co, -cl * so],
                     [cl, 0.0, -sl]])


def ned_from_body_rotation(yaw_deg: float, pitch_deg: float, roll_deg: float) -> np.ndarray:
    """Rotation matrix taking body vectors to local NED (ZYX yaw-pitch-roll)."""
    y = math.radians(yaw_deg)
    p = math.radians(pitch_deg)
    r = math.radians(roll_deg)
    cy, sy = math.cos(y), math.sin(y)
    cp, sp = math.cos(p), math.sin(p)
    cr, sr = math.cos(r), math.sin(r)
    return np.array([
        [cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr],
        [sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr],
        [-sp, cp * sr, cp * cr],
    ])


def ecef_to_camera_rotation(lat_deg: float, lon_deg: float,
                            yaw_deg: float, pitch_deg: float, roll_deg: float,
                            camera_from_body: np.ndarray = NOSE_CAMERA_FROM_BODY) -> Rotation:
    """Build the GIANT ``rotation_inertial_to_camera`` (ECEF -> camera frame)."""
    body_from_ned = ned_from_body_rotation(yaw_deg, pitch_deg, roll_deg).T
    ned_from_ecef = ecef_from_ned_rotation(lat_deg, lon_deg).T
    return Rotation(camera_from_body @ body_from_ned @ ned_from_ecef)


def ned_velocity_to_ecef(lat_deg: float, lon_deg: float,
                         vn: float, ve: float, vd: float) -> np.ndarray:
    """Convert a NED velocity (m/s) to the ECEF frame."""
    return ecef_from_ned_rotation(lat_deg, lon_deg) @ np.array([vn, ve, vd])


# ---------------------------------------------------------------------------
# sidecar handling
# ---------------------------------------------------------------------------

def sidecar_path(image_path: Union[str, Path]) -> Path:
    """Return the expected metadata sidecar path for an image file."""
    return Path(image_path).with_suffix('.json')


def write_sidecar(image_path: Union[str, Path], nav: dict, time_utc: Optional[str] = None,
                  fov_deg: float = 90.0, exposure_s: float = 0.01,
                  target: Optional[str] = None) -> Path:
    """
    Write a metadata sidecar for a captured image.

    ``nav`` is a dict using the ue5_bridge nav JSON keys (lat, lon, alt, vn,
    ve, vd, yaw, pitch, roll).  This is the same helper the UE5 capture code
    (or a test harness) should replicate.
    """
    meta = {
        'type': 'opnav_image',
        'time_utc': time_utc or datetime.now(timezone.utc).isoformat(),
        'exposure_s': exposure_s,
        'fov_deg': fov_deg,
        'lat': nav['lat'],
        'lon': nav['lon'],
        'alt_m': nav.get('alt', nav.get('alt_m', 0.0)),
        'vn': nav.get('vn', 0.0),
        've': nav.get('ve', 0.0),
        'vd': nav.get('vd', 0.0),
        'yaw': nav.get('yaw', 0.0),
        'pitch': nav.get('pitch', 0.0),
        'roll': nav.get('roll', 0.0),
    }
    if target:
        meta['target'] = target
    out = sidecar_path(image_path)
    out.write_text(json.dumps(meta, indent=2))
    return out


def _parse_time(value: str) -> datetime:
    """Parse an ISO-8601 timestamp, tolerating a trailing 'Z'."""
    dt = datetime.fromisoformat(value.replace('Z', '+00:00'))
    if dt.tzinfo is not None:
        dt = dt.astimezone(timezone.utc).replace(tzinfo=None)
    return dt


# ---------------------------------------------------------------------------
# GIANT classes
# ---------------------------------------------------------------------------

class UE5Image(OpNavImage):
    """An ``OpNavImage`` whose metadata is parsed from a UE5 JSON sidecar."""

    # camera mount used when parsing sidecars; override on the class if needed
    camera_from_body = NOSE_CAMERA_FROM_BODY

    # exposures longer than this (seconds) are treated as "long" exposures
    long_exposure_threshold = 0.5

    def parse_data(self, *args):
        assert self.file is not None, "we need a file to parse the data from"
        meta_file = sidecar_path(self.file)
        if not meta_file.exists():
            raise ValueError(
                "no metadata sidecar found for this image; expected {}".format(meta_file))

        meta = json.loads(meta_file.read_text())

        self.observation_date = _parse_time(meta['time_utc'])

        self.exposure = float(meta.get('exposure_s', 0.01))
        self.exposure_type = 'long' if self.exposure > self.long_exposure_threshold else 'short'

        lat = float(meta['lat'])
        lon = float(meta['lon'])
        alt = float(meta.get('alt_m', 0.0))

        # position/velocity of the camera in the "inertial" (ECEF) frame, meters
        self.position = geodetic_to_ecef(lat, lon, alt)
        self.velocity = ned_velocity_to_ecef(
            lat, lon,
            float(meta.get('vn', 0.0)), float(meta.get('ve', 0.0)), float(meta.get('vd', 0.0)))

        self.rotation_inertial_to_camera = ecef_to_camera_rotation(
            lat, lon,
            float(meta.get('yaw', 0.0)), float(meta.get('pitch', 0.0)),
            float(meta.get('roll', 0.0)),
            camera_from_body=self.camera_from_body)

        self.temperature = float(meta.get('temperature_c', 20.0))
        self.target = meta.get('target')
        self.instrument = meta.get('instrument', 'UE5_CAM')
        self.spacecraft = meta.get('spacecraft', 'CENTURIO')

        # saturation from the image bit depth (8-bit jpeg/png -> 255)
        if np.issubdtype(self.dtype, np.integer):
            self.saturation = float(np.iinfo(self.dtype).max)
        else:
            self.saturation = 1.0


class UE5Camera(Camera):
    """A GIANT ``Camera`` for UE5 SceneCapture imagery."""

    def __init__(self, images=None, model=None, name='UE5_CAM', spacecraft_name='CENTURIO',
                 frame='UE5_CAMERA_FRAME', parse_data=True, psf=None, attitude_function=None,
                 start_date=None, end_date=None, default_image_class=UE5Image):
        super().__init__(images=images, model=model, name=name, spacecraft_name=spacecraft_name,
                         frame=frame, parse_data=parse_data, psf=psf,
                         attitude_function=attitude_function, start_date=start_date,
                         end_date=end_date, default_image_class=default_image_class)

    def preprocessor(self, image):
        # UE5 renders are clean; no flats/darks needed
        return image


def build_camera_model(fov_deg: float, n_cols: int, n_rows: int) -> PinholeModel:
    """
    Build a pinhole model matching a UE5 camera.

    UE5's ``FieldOfView`` is the horizontal FOV in degrees; the focal length in
    pixels follows from that and the render resolution.
    """
    focal_px = (n_cols / 2.0) / math.tan(math.radians(fov_deg) / 2.0)
    return PinholeModel(focal_length=1.0, kx=focal_px, ky=focal_px,
                        px=(n_cols - 1) / 2.0, py=(n_rows - 1) / 2.0,
                        n_rows=n_rows, n_cols=n_cols, field_of_view=fov_deg)


# ---------------------------------------------------------------------------
# ingest
# ---------------------------------------------------------------------------

def find_image_pairs(image_dir: Union[str, Path]) -> list:
    """Return sorted image paths in a directory that have a metadata sidecar."""
    image_dir = Path(image_dir)
    pairs = [p for p in sorted(image_dir.iterdir())
             if p.suffix.lower() in IMAGE_EXTENSIONS and sidecar_path(p).exists()]
    return pairs


def load_images(image_dir: Union[str, Path]) -> list:
    """Load every image+sidecar pair in a directory as ``UE5Image`` objects."""
    return [UE5Image(str(p), parse_data=True) for p in find_image_pairs(image_dir)]


def build_camera(image_dir: Union[str, Path], fov_deg: Optional[float] = None,
                 **camera_kwargs) -> UE5Camera:
    """
    Ingest a directory of UE5 captures and return a GIANT-ready ``UE5Camera``.

    The FOV is read from the first sidecar unless explicitly provided; the
    resolution is taken from the first image.
    """
    pairs = find_image_pairs(image_dir)
    if not pairs:
        raise ValueError("no image/sidecar pairs found in {}".format(image_dir))

    if fov_deg is None:
        first_meta = json.loads(sidecar_path(pairs[0]).read_text())
        fov_deg = float(first_meta.get('fov_deg', 90.0))

    images = [UE5Image(str(p), parse_data=True) for p in pairs]
    n_rows, n_cols = images[0].shape[:2]
    model = build_camera_model(fov_deg, n_cols, n_rows)

    return UE5Camera(images=images, model=model, parse_data=False, **camera_kwargs)


# ---------------------------------------------------------------------------
# CLI: validate a capture directory
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Validate UE5 capture directory for GIANT ingest')
    parser.add_argument('image_dir', help='directory containing .png/.jpg + .json sidecars')
    args = parser.parse_args()

    image_dir = Path(args.image_dir)
    candidates = [p for p in sorted(image_dir.iterdir())
                  if p.suffix.lower() in IMAGE_EXTENSIONS]
    if not candidates:
        print('no images found in', image_dir)
        return 1

    ok = 0
    for p in candidates:
        sc = sidecar_path(p)
        if not sc.exists():
            print('MISSING SIDECAR:', p.name)
            continue
        try:
            img = UE5Image(str(p), parse_data=True)
            print('{:<40s} {}  {}x{}  pos_ecef=[{:.0f} {:.0f} {:.0f}] m'.format(
                p.name, img.observation_date, img.shape[1], img.shape[0],
                *img.position))
            ok += 1
        except Exception as exc:  # noqa: BLE001 - report and continue validating
            print('INVALID:', p.name, '-', exc)

    print('\n{}/{} images ready for GIANT'.format(ok, len(candidates)))
    return 0 if ok == len(candidates) else 1


if __name__ == '__main__':
    raise SystemExit(main())
