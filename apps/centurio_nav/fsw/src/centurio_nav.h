/************************************************************************
 * Main header for centurio_nav app
 ************************************************************************/
#ifndef CENTURIO_NAV_H
#define CENTURIO_NAV_H

#include "cfe.h"
#include "cfe_config.h"

#include "centurio_nav_mission_cfg.h"
#include "centurio_nav_platform_cfg.h"
#include "centurio_nav_internal_cfg.h"
#include "centurio_nav_perfids.h"
#include "centurio_nav_msgids.h"
#include "centurio_nav_msg.h"

#include "centurio_nav_eventids.h"

typedef struct
{
    uint8 CmdCounter;
    uint8 ErrCounter;

    CENTURIO_NAV_HkTlm_t HkTlm;

    uint32 RunStatus;

    CFE_SB_PipeId_t CommandPipe;

    CFE_TBL_Handle_t TblHandles[CENTURIO_NAV_NUMBER_OF_TABLES];

    struct
    {
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
        uint8  SystemStatus; /* 0=INIT,1=SAFE,2=GUIDANCE,3=MANUAL */
        uint8  NavFixType;   /* 0=NOFIX,2=2D,3=3D... */
    } Nav;

    struct
    {
        uint8 Mode; /* 1=SAFE,2=GUIDANCE,3=MANUAL */
        double LatDeg; double LonDeg; float AltM;
        float VN_MS; float VE_MS; float VD_MS;
        float YawDeg; float PitchDeg; float RollDeg;
        float ThrottlePercent;
    } Target;

} CENTURIO_NAV_Data_t;

extern CENTURIO_NAV_Data_t CENTURIO_NAV_Data;

void         CENTURIO_NAV_Main(void);
CFE_Status_t CENTURIO_NAV_Init(void);
void         CENTURIO_NAV_RunControlLoop(float DtSec);

#endif
