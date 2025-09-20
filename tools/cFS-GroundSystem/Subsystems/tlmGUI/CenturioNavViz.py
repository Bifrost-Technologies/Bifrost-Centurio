#!/usr/bin/env python3

# Lightweight attitude + map visualization for CENTURIO_NAV HK

import math
import sys
import getopt
import csv
from collections import deque
from pathlib import Path
from struct import unpack

import zmq
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QPointF, QRectF
from PyQt5.QtGui import QPainter, QPen, QBrush, QColor, QFont
from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow

ROOTDIR = Path(sys.argv[0]).resolve().parent

# Optional label maps for readable status/fix values
STATUS_LABELS = {
    0: 'INIT',
    1: 'READY',
    2: 'ACTIVE',
    3: 'FAULT',
}

FIX_LABELS = {
    0: 'NONE',
    1: 'DR',
    2: '2D',
    3: '3D',
    4: 'DGPS',
    5: 'RTK',
}


class TlmReceiver(QThread):
    signal_packet = pyqtSignal(bytes)

    def __init__(self, subscr: str):
        super().__init__()
        self.runs = True
        ctx = zmq.Context()
        self.sub = ctx.socket(zmq.SUB)
        self.sub.connect("ipc:///tmp/GroundSystem")
        # Determine the exact ZeroMQ topic to subscribe to.
        # Cases handled:
        # 1) subscr == "GroundSystem.0x894" -> expand to GroundSystem.Spacecraft1.TelemetryPackets.0x894
        # 2) subscr == "GroundSystem.Spacecraft1.TelemetryPackets.0x894" -> use verbatim
        # 3) subscr already contains "TelemetryPackets." -> use verbatim
        # 4) fallback: use subscr as-is
        topic = subscr
        if 'TelemetryPackets.' in subscr:
            topic = subscr
        elif subscr.startswith('GroundSystem.') and subscr.lower().find('0x') != -1:
            # Keep just the apid suffix after the first dot
            apid = subscr.split('.', 1)[1]
            topic = f"GroundSystem.Spacecraft1.TelemetryPackets.{apid}"
        self.sub.setsockopt_string(zmq.SUBSCRIBE, topic)

    def run(self):
        while self.runs:
            _, datagram = self.sub.recv_multipart()
            self.signal_packet.emit(datagram)


class NavViz(QWidget):
    def __init__(self, title: str, endian: str):
        super().__init__()
        self.setWindowTitle(title)
        self.resize(900, 600)
        # Endian handling: allow explicit L/B or auto-detect (A)
        endian = (endian or '').upper()
        if endian in ('L', 'B'):
            self.py_endian = '<' if endian == 'L' else '>'
            self._endian_locked = True
        else:
            # Start with a reasonable default and auto-detect on first packet
            self.py_endian = '<'  # x86/LE default
            self._endian_locked = False
        # Telemetry header offset (set by GroundSystem main window). Default 0.
        self.hdr_offset = 0
        try:
            # Match GenericTelemetry behavior
            with open("/tmp/OffsetData", "rb") as f:
                data = f.read(1)
                if data:
                    self.hdr_offset = data[0]
        except Exception:
            self.hdr_offset = 0

        # Telemetry state
        self.lat = 0.0
        self.lon = 0.0
        self.alt_m = 0.0
        self.vn = 0.0
        self.ve = 0.0
        self.vd = 0.0
        self.yaw = 0.0
        self.pitch = 0.0
        self.roll = 0.0
        self.throttle = 0.0
        self.fix = 0
        self.status = 0

        # Trail buffer (lat, lon)
        self.trail = deque(maxlen=180)  # ~3 minutes @ 1 Hz
        self.origin_set = False
        self.lat0 = 0.0
        self.lon0 = 0.0
        # Map interaction state
        self.map_scale = 0.005  # pixels per meter
        self.pan_x = 0.0
        self.pan_y = 0.0
        self.dragging = False
        self.last_mouse = None
        self.heading_up = False

        # Load telemetry definition file (same one used by GenericTelemetry)
        self.tlm_defs = []  # list of dicts with keys: desc, start, size, type
        self._load_tlm_defs()

    # -------------------------------
    # Decode helpers
    # -------------------------------
    def _plausibility_score(self, fields: dict) -> int:
        """Score decoded fields for plausibility. Higher is better."""
        score = 0
        lat = fields.get('lat')
        lon = fields.get('lon')
        alt = fields.get('alt')
        vn = fields.get('vn')
        ve = fields.get('ve')
        vd = fields.get('vd')
        yaw = fields.get('yaw')
        pitch = fields.get('pitch')
        roll = fields.get('roll')
        thr = fields.get('thr')
        fix = fields.get('fix')
        status = fields.get('status')

        def between(x, lo, hi):
            return x is not None and lo <= x <= hi

        if lat is not None and -90.5 <= lat <= 90.5:
            score += 3
        if lon is not None and -180.5 <= lon <= 180.5:
            score += 3
        if alt is not None and -500.0 <= alt <= 50000.0:
            score += 2
        # Speeds within a broad range
        if vn is not None and abs(vn) <= 1000:
            score += 1
        if ve is not None and abs(ve) <= 1000:
            score += 1
        if vd is not None and abs(vd) <= 1000:
            score += 1
        if between(yaw, -360, 720):
            score += 1
        if between(pitch, -90.5, 90.5):
            score += 1
        if between(roll, -180.5, 180.5):
            score += 1
        if thr is not None and -1.0 <= thr <= 101.0:
            score += 1
        if fix is not None and 0 <= int(fix) <= 7:
            score += 1
        if status is not None and 0 <= int(status) <= 10:
            score += 1
        return score

    def _decode_fields(self, pkt: bytes, endian: str, offset: int) -> dict:
        """Attempt to decode all fields using a given endian ('<' or '>') and header offset."""
        save_endian = self.py_endian
        save_offset = self.hdr_offset
        self.py_endian = endian
        self.hdr_offset = offset
        try:
            if self.tlm_defs:
                status = self._get_field(pkt, 'System Status', None)
                fix = self._get_field(pkt, 'Nav Fix Type', None)
                lat = self._get_field(pkt, 'Latitude (deg)', None)
                lon = self._get_field(pkt, 'Longitude (deg)', None)
                alt = self._get_field(pkt, 'Altitude (m)', None)
                vn = self._get_field(pkt, 'Vel North (m/s)', None)
                ve = self._get_field(pkt, 'Vel East (m/s)', None)
                vd = self._get_field(pkt, 'Vel Down (m/s)', None)
                yaw = self._get_field(pkt, 'Yaw (deg)', None)
                pitch = self._get_field(pkt, 'Pitch (deg)', None)
                roll = self._get_field(pkt, 'Roll (deg)', None)
                thr = self._get_field(pkt, 'Throttle (%)', None)
            else:
                off = offset
                if len(pkt) < off + 64:
                    return {}
                status = unpack(endian + 'B', pkt[off + 14: off + 15])[0]
                fix = unpack(endian + 'B', pkt[off + 15: off + 16])[0]
                lat = unpack(endian + 'd', pkt[off + 16: off + 24])[0]
                lon = unpack(endian + 'd', pkt[off + 24: off + 32])[0]
                alt = unpack(endian + 'f', pkt[off + 32: off + 36])[0]
                vn = unpack(endian + 'f', pkt[off + 36: off + 40])[0]
                ve = unpack(endian + 'f', pkt[off + 40: off + 44])[0]
                vd = unpack(endian + 'f', pkt[off + 44: off + 48])[0]
                yaw = unpack(endian + 'f', pkt[off + 48: off + 52])[0]
                pitch = unpack(endian + 'f', pkt[off + 52: off + 56])[0]
                roll = unpack(endian + 'f', pkt[off + 56: off + 60])[0]
                thr = unpack(endian + 'f', pkt[off + 60: off + 64])[0]
            return {
                'status': float(status) if status is not None else None,
                'fix': float(fix) if fix is not None else None,
                'lat': float(lat) if lat is not None else None,
                'lon': float(lon) if lon is not None else None,
                'alt': float(alt) if alt is not None else None,
                'vn': float(vn) if vn is not None else None,
                've': float(ve) if ve is not None else None,
                'vd': float(vd) if vd is not None else None,
                'yaw': float(yaw) if yaw is not None else None,
                'pitch': float(pitch) if pitch is not None else None,
                'roll': float(roll) if roll is not None else None,
                'thr': float(thr) if thr is not None else None,
            }
        except Exception:
            return {}
        finally:
            self.py_endian = save_endian
            self.hdr_offset = save_offset

    def _load_tlm_defs(self):
        tlm_file = ROOTDIR / 'cfs-centurio-nav-hk-tlm.txt'
        try:
            with open(tlm_file, 'r') as f:
                reader = csv.reader(f, skipinitialspace=True)
                for row in reader:
                    if not row or row[0].startswith('#'):
                        continue
                    # Columns: desc, offset, size, type, display, ...
                    desc = row[0].strip()
                    start = int(row[1].strip())
                    size = int(row[2].strip())
                    py_type = row[3].strip()  # struct code like B,H,I,f,d
                    self.tlm_defs.append({
                        'desc': desc,
                        'start': start,
                        'size': size,
                        'type': py_type,
                    })
        except Exception:
            # Fall back to empty; decoding will use hardcoded offsets
            self.tlm_defs = []

    def _normalize_name(self, s: str) -> str:
        # Lowercase, strip spaces, drop unit annotations in parentheses
        s = s.strip().lower()
        # Remove parenthetical unit text
        if '(' in s and ')' in s:
            s = s[:s.find('(')].strip()
        return s

    def _get_field(self, pkt: bytes, name: str, default=None):
        # Lookup by description; decode with py_endian and hdr_offset
        target = self._normalize_name(name)
        for d in self.tlm_defs:
            dn = self._normalize_name(d['desc'])
            if dn == target or dn.startswith(target):
                start = self.hdr_offset + d['start']
                end = start + d['size']
                if len(pkt) < end:
                    return default
                fmt = ('' if d['type'] == 's' else self.py_endian) + d['type']
                try:
                    val = unpack(fmt, pkt[start:end])[0]
                    if isinstance(val, bytes):
                        try:
                            return val.decode('utf-8', 'ignore')
                        except Exception:
                            return default
                    return val
                except Exception:
                    return default
        return default

    def update_from_packet(self, pkt: bytes):
        # Auto-detect endianness and header offset on first packet if not locked
        if not self._endian_locked:
            candidates_endian = ['<', '>']
            # Try current offset first, then scan several likely offsets (0..32 step 4)
            offsets = [self.hdr_offset] + [o for o in range(0, 36, 4) if o != self.hdr_offset]
            best = None
            best_score = -1
            for e in candidates_endian:
                for off in offsets:
                    fields = self._decode_fields(pkt, e, off)
                    score = self._plausibility_score(fields)
                    if score > best_score:
                        best_score = score
                        best = (e, off, fields)
            if best is not None:
                self.py_endian, self.hdr_offset, fields = best
                # Apply chosen fields immediately
                self.status = int(fields.get('status') or 0)
                self.fix = int(fields.get('fix') or 0)
                self.lat = float(fields.get('lat') or self.lat)
                self.lon = float(fields.get('lon') or self.lon)
                self.alt_m = float(fields.get('alt') or self.alt_m)
                self.vn = float(fields.get('vn') or self.vn)
                self.ve = float(fields.get('ve') or self.ve)
                self.vd = float(fields.get('vd') or self.vd)
                self.yaw = float(fields.get('yaw') or self.yaw)
                self.pitch = float(fields.get('pitch') or self.pitch)
                self.roll = float(fields.get('roll') or self.roll)
                self.throttle = float(fields.get('thr') or self.throttle)
            self._endian_locked = True

        # Prefer CSV-driven decode for parity with GenericTelemetry
        if self.tlm_defs:
            try:
                self.status = int(self._get_field(pkt, 'System Status', self.status) or 0)
                self.fix = int(self._get_field(pkt, 'Nav Fix Type', self.fix) or 0)
                lat = self._get_field(pkt, 'Latitude (deg)', self.lat)
                lon = self._get_field(pkt, 'Longitude (deg)', self.lon)
                alt = self._get_field(pkt, 'Altitude (m)', self.alt_m)
                vn = self._get_field(pkt, 'Vel North (m/s)', self.vn)
                ve = self._get_field(pkt, 'Vel East (m/s)', self.ve)
                vd = self._get_field(pkt, 'Vel Down (m/s)', self.vd)
                yaw = self._get_field(pkt, 'Yaw (deg)', self.yaw)
                pitch = self._get_field(pkt, 'Pitch (deg)', self.pitch)
                roll = self._get_field(pkt, 'Roll (deg)', self.roll)
                thr = self._get_field(pkt, 'Throttle (%)', self.throttle)

                # Assign if successfully decoded (not None)
                if lat is not None: self.lat = float(lat)
                if lon is not None: self.lon = float(lon)
                if alt is not None: self.alt_m = float(alt)
                if vn is not None: self.vn = float(vn)
                if ve is not None: self.ve = float(ve)
                if vd is not None: self.vd = float(vd)
                if yaw is not None: self.yaw = float(yaw)
                if pitch is not None: self.pitch = float(pitch)
                if roll is not None: self.roll = float(roll)
                if thr is not None: self.throttle = float(thr)
            except Exception:
                # If CSV decode fails unexpectedly, bail out for this packet
                return
        else:
            # Fallback to hardcoded offsets if CSV not loaded
            try:
                off = self.hdr_offset
                if len(pkt) < off + 64:
                    return
                self.status = unpack(self.py_endian + 'B', pkt[off + 14: off + 15])[0]
                self.fix = unpack(self.py_endian + 'B', pkt[off + 15: off + 16])[0]
                self.lat = unpack(self.py_endian + 'd', pkt[off + 16: off + 24])[0]
                self.lon = unpack(self.py_endian + 'd', pkt[off + 24: off + 32])[0]
                self.alt_m = unpack(self.py_endian + 'f', pkt[off + 32: off + 36])[0]
                self.vn = unpack(self.py_endian + 'f', pkt[off + 36: off + 40])[0]
                self.ve = unpack(self.py_endian + 'f', pkt[off + 40: off + 44])[0]
                self.vd = unpack(self.py_endian + 'f', pkt[off + 44: off + 48])[0]
                self.yaw = unpack(self.py_endian + 'f', pkt[off + 48: off + 52])[0]
                self.pitch = unpack(self.py_endian + 'f', pkt[off + 52: off + 56])[0]
                self.roll = unpack(self.py_endian + 'f', pkt[off + 56: off + 60])[0]
                self.throttle = unpack(self.py_endian + 'f', pkt[off + 60: off + 64])[0]
            except Exception:
                return

        # Update origin on first valid coordinate and always record trail
        if not self.origin_set and (self.lat != 0.0 or self.lon != 0.0):
            self.lat0, self.lon0 = self.lat, self.lon
            self.origin_set = True
        if self.origin_set:
            # Append if position changed meaningfully to avoid dup points
            if not self.trail or (abs(self.lat - self.trail[-1][0]) > 1e-9 or abs(self.lon - self.trail[-1][1]) > 1e-9):
                self.trail.append((self.lat, self.lon))

        self.update()

    @staticmethod
    def norm_heading(deg: float) -> float:
        x = deg % 360.0
        if x < 0:
            x += 360.0
        return x

    def latlon_to_xy(self, lat: float, lon: float) -> QPointF:
        # Local tangent plane approximation around origin
        if not self.origin_set:
            return QPointF(0, 0)
        m_per_deg_lat = 111_320.0
        m_per_deg_lon = 111_320.0 * math.cos(math.radians(self.lat0))
        dx = (lon - self.lon0) * m_per_deg_lon
        dy = (lat - self.lat0) * m_per_deg_lat
        # scale to pixels (tuned for view size)
        return QPointF(dx * self.map_scale, -dy * self.map_scale)  # screen y down

    def draw_horizon(self, p: QPainter, rect):
        cx = rect.x() + rect.width() * 0.25
        cy = rect.y() + rect.height() * 0.5
        radius = min(rect.width() * 0.45, rect.height() * 0.45)

        p.save()
        p.translate(cx, cy)
        p.rotate(-self.roll)

        # Sky and ground bands, shifted by pitch
        sky = QColor(70, 130, 180)
        ground = QColor(160, 110, 60)
        p.setPen(Qt.NoPen)

        pitch_px = self.pitch * 3  # pixels per degree
        p.setBrush(sky)
        p.drawRect(QRectF(-radius, -radius * 2 + pitch_px, radius * 2, radius * 2))
        p.setBrush(ground)
        p.drawRect(QRectF(-radius, pitch_px, radius * 2, radius * 2))

        # Horizon line
        p.setPen(QPen(Qt.white, 2))
        p.drawLine(QPointF(-radius, pitch_px), QPointF(radius, pitch_px))

        # Pitch ladder every 10 deg
        for deg in range(-90, 100, 10):
            y = pitch_px - deg * 3
            if -radius * 1.5 <= y <= radius * 1.5:
                w = radius * (0.2 if deg % 20 else 0.35)
                p.drawLine(QPointF(-w, y), QPointF(w, y))
                if deg % 20 == 0 and deg != 0:
                    p.drawText(QPointF(w + 5, y + 4), f"{deg}")
                    p.drawText(QPointF(-w - 25, y + 4), f"{deg}")

        # Bank index (ticks)
        p.setPen(QPen(Qt.white, 2))
        for ang in (-60, -45, -30, -20, -10, 10, 20, 30, 45, 60):
            a = math.radians(ang)
            r1 = radius * 0.9
            r2 = r1 - (10 if abs(ang) % 30 == 0 else 6)
            p.drawLine(QPointF(r1 * math.sin(a), -r1 * math.cos(a)), QPointF(r2 * math.sin(a), -r2 * math.cos(a)))

        # Fixed aircraft symbol
        p.setPen(QPen(Qt.yellow, 3))
        p.drawLine(-20, 0, 20, 0)
        p.drawLine(0, 0, 0, 12)

        p.restore()

        # Readouts
        p.setPen(Qt.white)
        p.setFont(QFont('Arial', 9))
        p.drawText(rect.x() + 10, rect.y() + 20, f"Pitch {round(self.pitch,1):.1f} deg")
        p.drawText(rect.x() + 10, rect.y() + 40, f"Roll  {round(self.roll,1):.1f} deg")

    def draw_compass(self, p: QPainter, rect):
        cx = rect.center().x()
        cy = rect.y() + rect.height() * 0.18
        radius = rect.width() * 0.09  # smaller so it doesn't crowd the map
        hdg = self.norm_heading(self.yaw)

        p.save()
        p.translate(cx, cy)
        p.setPen(QPen(Qt.white, 2))
        p.setBrush(Qt.NoBrush)
        p.drawEllipse(QRectF(-radius, -radius, radius * 2, radius * 2))

        # Rose markings
        for ang in range(0, 360, 30):
            a = math.radians(ang)
            r1 = radius * 0.85
            r2 = radius * 0.95
            p.drawLine(QPointF(r1 * math.cos(a), r1 * math.sin(a)), QPointF(r2 * math.cos(a), r2 * math.sin(a)))
        p.setFont(QFont('Arial', 9))
        p.drawText(QPointF(-22, radius + 14), f"HDG {hdg:06.2f}")

        # Lubber line (upwards)
        p.setPen(QPen(Qt.red, 3))
        p.drawLine(QPointF(0, -radius), QPointF(0, -radius + 12))
        p.restore()

    def draw_map(self, p: QPainter, rect):
        # Background (not rotated)
        p.save()
        p.setPen(QPen(QColor(70, 70, 70), 1))
        p.setBrush(QBrush(QColor(20, 20, 20)))
        p.drawRect(rect.x(), rect.y(), rect.width(), rect.height())
        p.restore()

        # Transform to center; rotate if heading-up
        cx = rect.center().x()
        cy = rect.center().y()

        p.save()
        if self.heading_up:
            p.translate(cx, cy)
            p.rotate(-self.yaw)
            p.translate(-cx, -cy)

        # Grid (panned and optionally rotated)
        p.setPen(QPen(QColor(60, 60, 60), 1))
        spacing = 50  # px
        half_w = rect.width() // 2
        half_h = rect.height() // 2
        # Determine line indices to cover rect
        nx = int((half_w + abs(self.pan_x)) // spacing) + 2
        ny = int((half_h + abs(self.pan_y)) // spacing) + 2
        # Vertical lines
        for i in range(-nx, nx + 1):
            x = cx + self.pan_x + i * spacing
            p.drawLine(QPointF(x, rect.y()), QPointF(x, rect.bottom()))
        # Horizontal lines
        for j in range(-ny, ny + 1):
            y = cy + self.pan_y + j * spacing
            p.drawLine(QPointF(rect.x(), y), QPointF(rect.right(), y))

        # Trail
        p.setPen(QPen(Qt.green, 2))
        last_pt = None
        for lat, lon in self.trail:
            q = self.latlon_to_xy(lat, lon)
            pt = QPointF(cx + self.pan_x + q.x(), cy + self.pan_y + q.y())
            if last_pt is not None:
                p.drawLine(last_pt, pt)
            last_pt = pt

        # Current position
        p.setPen(QPen(Qt.yellow, 3))
        p.setBrush(QBrush(Qt.yellow))
        q = self.latlon_to_xy(self.lat, self.lon)
        cur = QPointF(cx + self.pan_x + q.x(), cy + self.pan_y + q.y())
        p.drawEllipse(cur, 4, 4)

        # Heading/course vector from VN/VE (ground track)
        if self.vn != 0.0 or self.ve != 0.0:
            spd = math.hypot(self.vn, self.ve)
            course = math.degrees(math.atan2(self.ve, self.vn))
            length = min(60.0, 5.0 + spd * 2.0)
            a = math.radians(90 - course)  # screen angle (0 deg up)
            x2 = cur.x() + length * math.cos(a)
            y2 = cur.y() - length * math.sin(a)
            p.setPen(QPen(Qt.cyan, 2))
            p.drawLine(cur, QPointF(x2, y2))

        p.restore()

        # Readouts (unrotated overlay)
        p.save()
        p.setPen(Qt.white)
        p.setFont(QFont('Arial', 9))
        gs = math.hypot(self.vn, self.ve)
        vsi = -self.vd
        fix_lbl = FIX_LABELS.get(self.fix, str(self.fix))
        status_lbl = STATUS_LABELS.get(self.status, str(self.status))
        # Fixed decimal formatting to avoid scientific notation
        p.drawText(rect.x() + 10, rect.y() + 15, f"Lat {self.lat:.7f}  Lon {self.lon:.7f}")
        p.drawText(rect.x() + 10, rect.y() + 32, f"Alt {self.alt_m:.2f} m  GS {gs:.2f} m/s  VSI {vsi:.2f} m/s")
        p.drawText(rect.x() + 10, rect.y() + 49, f"Throttle {self.throttle:.0f}%  Fix {fix_lbl}  Status {status_lbl}  ({'HDG-UP' if self.heading_up else 'N-UP'})")
        # Debug: show endian and header offset to confirm decode settings
        p.drawText(rect.right() - 220, rect.y() + 15, f"Endian: {'BE' if self.py_endian == '>' else 'LE'} Off: {self.hdr_offset}")
        p.restore()

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        w = self.width()
        h = self.height()
        # Layout: left horizon; right split into compass (top) and map (bottom)
        left = (0, 0, int(w * 0.5), h)
        right = (int(w * 0.5), 0, int(w * 0.5), h)
        comp = (right[0], right[1], right[2], int(h * 0.3))
        mapr = (right[0] + 10, right[1] + int(h * 0.3) + 10, right[2] - 20, int(h * 0.7) - 20)

        self.draw_horizon(p, QRectLike(*left))
        self.draw_compass(p, QRectLike(*comp))
        self.draw_map(p, QRectLike(*mapr))

    # Interaction: zoom/pan + toggle heading-up
    def wheelEvent(self, event):
        delta = event.angleDelta().y() / 120.0
        factor = 1.15 ** delta
        self.map_scale = max(0.001, min(0.05, self.map_scale * factor))
        self.update()

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.dragging = True
            self.last_mouse = event.pos()
        elif event.button() == Qt.RightButton:
            self.heading_up = not self.heading_up
            self.update()

    def mouseMoveEvent(self, event):
        if self.dragging and self.last_mouse is not None:
            dx = event.pos().x() - self.last_mouse.x()
            dy = event.pos().y() - self.last_mouse.y()
            self.pan_x += dx
            self.pan_y += dy
            self.last_mouse = event.pos()
            self.update()

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.dragging = False
            self.last_mouse = None


class QRectLike:
    def __init__(self, x, y, w, h):
        self._x, self._y, self._w, self._h = x, y, w, h

    def x(self): return self._x
    def y(self): return self._y
    def width(self): return self._w
    def height(self): return self._h
    def center(self):
        class C:  # tiny helper
            def __init__(self, x, y): self._x, self._y = x, y
            def x(self): return self._x
            def y(self): return self._y
        return C(self._x + self._w / 2, self._y + self._h / 2)
    def bottom(self): return self._y + self._h
    def right(self): return self._x + self._w


class Main(QMainWindow):
    def __init__(self, title: str, endian: str, subscr: str):
        super().__init__()
        self.viz = NavViz(title, endian)
        self.setCentralWidget(self.viz)
        self.rx = TlmReceiver(subscr)
        self.rx.signal_packet.connect(self.viz.update_from_packet)
        self.rx.start()

    def closeEvent(self, event):
        self.rx.runs = False
        self.rx.wait(2000)
        super().closeEvent(event)


def usage():
    print("Usage: CenturioNavViz.py --title=... --appid=0xNNN --endian=L|B|A --sub=GroundSystem.0xNNN")


if __name__ == '__main__':
    page_title = "Centurio Nav Viz"
    app_id = 0
    endian = 'A'  # auto-detect by default
    subscription = ''

    try:
        opts, _ = getopt.getopt(sys.argv[1:], "htpafl", [
            "help", "title=", "port=", "appid=", "file=", "endian=", "sub="
        ])
    except getopt.GetoptError:
        usage(); sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h", "--help"): usage(); sys.exit(0)
        elif opt in ("-t", "--title"): page_title = arg
        elif opt in ("-a", "--appid"): app_id = int(arg, 16) if isinstance(arg, str) and arg.startswith('0x') else int(arg)
        elif opt in ("-e", "--endian"): endian = arg
        elif opt in ("-s", "--sub"): subscription = arg

    if not subscription:
        subscription = "GroundSystem"

    app = QApplication(sys.argv)
    win = Main(page_title, endian, subscription)
    win.resize(900, 600)
    win.show()
    sys.exit(app.exec_())
