#!/usr/bin/env python3
import argparse
import json
import socket

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 15001  # match UE5_BRIDGE_JSON_RX_PORT


def send(obj, host, port):
    data = json.dumps(obj).encode("utf-8")
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto(data, (host, port))
    s.close()


def main():
    ap = argparse.ArgumentParser(description="Send JSON control to ue5_bridge")
    ap.add_argument("type", choices=[
        "noop", "reset", "set_mode", "set_throttle",
        "set_target_pos", "set_target_vel", "set_target_att"
    ])
    ap.add_argument("--host", default=DEFAULT_HOST)
    ap.add_argument("--port", type=int, default=DEFAULT_PORT)
    ap.add_argument("--mode", type=int)
    ap.add_argument("--percent", type=float)
    ap.add_argument("--lat", type=float)
    ap.add_argument("--lon", type=float)
    ap.add_argument("--alt_m", type=float)
    ap.add_argument("--vn", type=float)
    ap.add_argument("--ve", type=float)
    ap.add_argument("--vd", type=float)
    ap.add_argument("--yaw", type=float)
    ap.add_argument("--pitch", type=float)
    ap.add_argument("--roll", type=float)
    args = ap.parse_args()

    obj = {"type": args.type}

    if args.type == "set_mode":
        if args.mode is None:
            ap.error("set_mode requires --mode")
        obj.update({"mode": int(args.mode)})

    elif args.type == "set_throttle":
        if args.percent is None:
            ap.error("set_throttle requires --percent")
        obj.update({"percent": float(args.percent)})

    elif args.type == "set_target_pos":
        for k in ("lat", "lon", "alt_m"):
            if getattr(args, k) is None:
                ap.error("set_target_pos requires --lat --lon --alt_m")
        obj.update({"lat": args.lat, "lon": args.lon, "alt_m": args.alt_m})

    elif args.type == "set_target_vel":
        for k in ("vn", "ve", "vd"):
            if getattr(args, k) is None:
                ap.error("set_target_vel requires --vn --ve --vd")
        obj.update({"vn": args.vn, "ve": args.ve, "vd": args.vd})

    elif args.type == "set_target_att":
        for k in ("yaw", "pitch", "roll"):
            if getattr(args, k) is None:
                ap.error("set_target_att requires --yaw --pitch --roll")
        obj.update({"yaw": args.yaw, "pitch": args.pitch, "roll": args.roll})

    send(obj, args.host, args.port)
    print(f"Sent to {args.host}:{args.port}: {json.dumps(obj)}")


if __name__ == "__main__":
    main()
