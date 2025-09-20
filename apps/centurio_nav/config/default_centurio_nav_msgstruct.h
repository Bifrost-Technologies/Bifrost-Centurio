/************************************************************************
 * Message struct wrappers for centurio_nav
 ************************************************************************/
#ifndef CENTURIO_NAV_MSGSTRUCT_H
#define CENTURIO_NAV_MSGSTRUCT_H

#include "cfe_msg_hdr.h"
#include "centurio_nav_msgdefs.h"

/* No-args commands */
typedef struct { CFE_MSG_CommandHeader_t CommandHeader; } CENTURIO_NAV_NoopCmd_t;

typedef struct { CFE_MSG_CommandHeader_t CommandHeader; } CENTURIO_NAV_ResetCountersCmd_t;

typedef struct { CFE_MSG_CommandHeader_t CommandHeader; } CENTURIO_NAV_SendHkCmd_t;

/* HK telemetry */
typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    CENTURIO_NAV_HkTlm_Payload_t Payload;
} CENTURIO_NAV_HkTlm_t;

/* Mission commands with payloads */
typedef struct { CFE_MSG_CommandHeader_t CommandHeader; CENTURIO_NAV_SetMode_Payload_t Payload; } CENTURIO_NAV_SetModeCmd_t;

typedef struct { CFE_MSG_CommandHeader_t CommandHeader; CENTURIO_NAV_SetTargetPos_Payload_t Payload; } CENTURIO_NAV_SetTargetPosCmd_t;

typedef struct { CFE_MSG_CommandHeader_t CommandHeader; CENTURIO_NAV_SetTargetVel_Payload_t Payload; } CENTURIO_NAV_SetTargetVelCmd_t;

typedef struct { CFE_MSG_CommandHeader_t CommandHeader; CENTURIO_NAV_SetTargetAtt_Payload_t Payload; } CENTURIO_NAV_SetTargetAttCmd_t;

typedef struct { CFE_MSG_CommandHeader_t CommandHeader; CENTURIO_NAV_SetThrottle_Payload_t Payload; } CENTURIO_NAV_SetThrottleCmd_t;

#endif
