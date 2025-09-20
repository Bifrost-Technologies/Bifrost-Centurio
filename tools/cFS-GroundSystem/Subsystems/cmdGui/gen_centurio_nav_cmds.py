#!/usr/bin/env python3
"""
Generate CommandFiles and ParameterFiles pickles for CENTURIO_NAV app so the
GroundSystem command GUI can send commands and "Send HK".

Outputs:
  CommandFiles/CENTURIO_NAV_CMD
  CommandFiles/CENTURIO_NAV_SENDHK
  ParameterFiles/CENTURIO_NAV_NOOP_CC
  ParameterFiles/CENTURIO_NAV_RESET_COUNTERS_CC
  ParameterFiles/CENTURIO_NAV_SET_MODE_CC
  ParameterFiles/CENTURIO_NAV_SET_TARGET_POS_CC
  ParameterFiles/CENTURIO_NAV_SET_TARGET_VEL_CC
  ParameterFiles/CENTURIO_NAV_SET_TARGET_ATT_CC
  ParameterFiles/CENTURIO_NAV_SET_THROTTLE_CC
  ParameterFiles/CENTURIO_NAV_SEND_HK_CC
"""

import os
import pickle
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CMD_DIR = ROOT / 'CommandFiles'
PAR_DIR = ROOT / 'ParameterFiles'


def ensure_dirs():
    CMD_DIR.mkdir(parents=True, exist_ok=True)
    PAR_DIR.mkdir(parents=True, exist_ok=True)


def write_cmd_file(filename: str, desc: list, codes: list, params: list):
    out = (desc, [str(c) for c in codes], params)
    with open(CMD_DIR / filename, 'wb') as f:
        pickle.dump(out, f, protocol=pickle.HIGHEST_PROTOCOL)


def write_param_file(filename: str, names: list, types: list, desc: list):
    # Parameter.py expects a sequence with 6 items
    # (_, paramNames, _, paramDesc, dataTypesNew, stringLen)
    string_len = [0 for _ in names]
    payload = ([], names, [], desc, types, string_len)
    with open(PAR_DIR / filename, 'wb') as f:
        pickle.dump(payload, f, protocol=pickle.HIGHEST_PROTOCOL)


def main():
    ensure_dirs()

    # Full command set for CENTURIO_NAV CMD MID (0x1892)
    cmd_desc = [
        'No-Op',
        'Reset Counters',
        'Set Mode',
        'Set Target Position',
        'Set Target Velocity',
        'Set Target Attitude',
        'Set Throttle',
    ]
    cmd_codes = [0, 1, 2, 3, 4, 5, 6]
    param_files = [
        'CENTURIO_NAV_NOOP_CC',
        'CENTURIO_NAV_RESET_COUNTERS_CC',
        'CENTURIO_NAV_SET_MODE_CC',
        'CENTURIO_NAV_SET_TARGET_POS_CC',
        'CENTURIO_NAV_SET_TARGET_VEL_CC',
        'CENTURIO_NAV_SET_TARGET_ATT_CC',
        'CENTURIO_NAV_SET_THROTTLE_CC',
    ]
    write_cmd_file('CENTURIO_NAV_CMD', cmd_desc, cmd_codes, param_files)

    # Minimal command page for Send HK on its own MID (0x1893)
    write_cmd_file('CENTURIO_NAV_SENDHK', ['Send Housekeeping'], [0], ['CENTURIO_NAV_SEND_HK_CC'])

    # Parameter files
    write_param_file('CENTURIO_NAV_NOOP_CC', [], [], [])
    write_param_file('CENTURIO_NAV_RESET_COUNTERS_CC', [], [], [])
    write_param_file('CENTURIO_NAV_SEND_HK_CC', [], [], [])

    write_param_file(
        'CENTURIO_NAV_SET_MODE_CC',
        names=['Mode'],
        types=['--uint8'],
        desc=['0=INIT,1=SAFE,2=GUIDANCE,3=MANUAL'],
    )

    write_param_file(
        'CENTURIO_NAV_SET_TARGET_POS_CC',
        names=['LatDeg', 'LonDeg', 'AltM'],
        types=['--double', '--double', '--float'],
        desc=['Latitude (deg)', 'Longitude (deg)', 'Altitude (m)'],
    )

    write_param_file(
        'CENTURIO_NAV_SET_TARGET_VEL_CC',
        names=['VN_MS', 'VE_MS', 'VD_MS'],
        types=['--float', '--float', '--float'],
        desc=['North vel (m/s)', 'East vel (m/s)', 'Down vel (m/s)'],
    )

    write_param_file(
        'CENTURIO_NAV_SET_TARGET_ATT_CC',
        names=['YawDeg', 'PitchDeg', 'RollDeg'],
        types=['--float', '--float', '--float'],
        desc=['Yaw (deg)', 'Pitch (deg)', 'Roll (deg)'],
    )

    write_param_file(
        'CENTURIO_NAV_SET_THROTTLE_CC',
        names=['Percent'],
        types=['--float'],
        desc=['Throttle percent (0-100)'],
    )

    print('Generated CENTURIO_NAV command and parameter files:')
    print('  -', CMD_DIR / 'CENTURIO_NAV_CMD')
    print('  -', CMD_DIR / 'CENTURIO_NAV_SENDHK')


if __name__ == '__main__':
    main()
