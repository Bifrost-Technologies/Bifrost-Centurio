/************************************************************************
 * Message definition types for centurio_nav
 ************************************************************************/
#ifndef CENTURIO_NAV_MSGDEFS_H
#define CENTURIO_NAV_MSGDEFS_H

#include "common_types.h"

/* Housekeeping telemetry payload */
typedef struct
{
    uint8  CommandErrorCounter;
    uint8  CommandCounter;
    uint8  SystemStatus;   /* 0=INIT,1=SAFE,2=GUIDANCE,3=MANUAL */
    uint8  NavFixType;     /* 0=NONE, 2=2D, 3=3D */

    double LatitudeDeg;
    double LongitudeDeg;
    float  AltitudeM;

    float  VelNorthMS;
    float  VelEastMS;
    float  VelDownMS;

    float  YawDeg;
    float  PitchDeg;
    float  RollDeg;

    float  ThrottlePercent;
    uint16 Spare2; /* align to 4-byte boundary */
} CENTURIO_NAV_HkTlm_Payload_t;

/* Command payloads */
typedef struct { uint8 Mode; } CENTURIO_NAV_SetMode_Payload_t;

typedef struct { double LatDeg; double LonDeg; float AltM; } CENTURIO_NAV_SetTargetPos_Payload_t;

typedef struct { float VN_MS; float VE_MS; float VD_MS; } CENTURIO_NAV_SetTargetVel_Payload_t;

typedef struct { float YawDeg; float PitchDeg; float RollDeg; } CENTURIO_NAV_SetTargetAtt_Payload_t;

typedef struct { float Percent; } CENTURIO_NAV_SetThrottle_Payload_t;

#endif /* CENTURIO_NAV_MSGDEFS_H */
