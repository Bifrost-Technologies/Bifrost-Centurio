#!/usr/bin/env python3
"""Create /tmp/OffsetData with default header offsets (v1) for Windows.

This avoids needing to launch the full GroundSystem (which uses ipc:// ZMQ)
when only the Command System is needed.
"""
import os
from pathlib import Path

def main():
    # Use header version 2 offsets by default (matches modern cFE v7+)
    # See GroundSystem.py constants: TLM_HDR_V2_OFFSET=4, CMD_HDR_PRI_V2_OFFSET=4, CMD_HDR_SEC_V2_OFFSET=4
    TLM_HDR_V2_OFFSET = 4
    CMD_HDR_PRI_V2_OFFSET = 4
    CMD_HDR_SEC_V2_OFFSET = 4

    offsets = bytes((TLM_HDR_V2_OFFSET, CMD_HDR_PRI_V2_OFFSET, CMD_HDR_SEC_V2_OFFSET))
    # On Windows, '/tmp' becomes 'C:\\tmp'
    tmp_dir = Path('/tmp')
    tmp_dir.mkdir(parents=True, exist_ok=True)
    with open(tmp_dir / 'OffsetData', 'wb') as f:
        f.write(offsets)
    print('Wrote offsets to', tmp_dir / 'OffsetData')

if __name__ == '__main__':
    main()
