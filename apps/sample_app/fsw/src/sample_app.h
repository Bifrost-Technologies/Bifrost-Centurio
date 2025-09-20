/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * Main header file for the Sample application
 */

#ifndef SAMPLE_APP_H
#define SAMPLE_APP_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_config.h"

#include "sample_app_mission_cfg.h"
#include "sample_app_platform_cfg.h"

#include "sample_app_perfids.h"
#include "sample_app_msgids.h"
#include "sample_app_msg.h"

/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    SAMPLE_APP_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    CFE_TBL_Handle_t TblHandles[SAMPLE_APP_NUMBER_OF_TABLES];

    /* Navigation and status runtime state (not directly in HK packet) */
    struct
    {
        double LatitudeDeg;   /* degrees */
        double LongitudeDeg;  /* degrees */
        float  AltitudeM;     /* meters */
        float  VelNorthMS;    /* m/s */
        float  VelEastMS;     /* m/s */
        float  VelDownMS;     /* m/s (down positive) */
        float  YawDeg;        /* degrees */
        float  PitchDeg;      /* degrees */
        float  RollDeg;       /* degrees */
        uint8  SystemStatus;  /* 0=INIT,1=OK,2=WARN,3=ERROR */
        uint8  NavFixType;    /* 0=NOFIX,2=2D,3=3D,4=DGPS,5=RTK */
    } Nav;
} SAMPLE_APP_Data_t;

/*
** Global data structure
*/
extern SAMPLE_APP_Data_t SAMPLE_APP_Data;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (SAMPLE_APP_Main), these
**       functions are not called from any other source module.
*/
void         SAMPLE_APP_Main(void);
CFE_Status_t SAMPLE_APP_Init(void);

#endif /* SAMPLE_APP_H */
