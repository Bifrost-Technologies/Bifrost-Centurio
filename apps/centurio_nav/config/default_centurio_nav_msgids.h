/************************************************************************
 * Message IDs for centurio_nav
 ************************************************************************/
#ifndef CENTURIO_NAV_MSGIDS_H
#define CENTURIO_NAV_MSGIDS_H

#include "cfe_core_api_base_msgids.h"
#include "centurio_nav_topicids.h"

/* Map mission topic IDs to platform message IDs */
#define CENTURIO_NAV_CMD_MID     CFE_PLATFORM_CMD_TOPICID_TO_MIDV(CFE_MISSION_CENTURIO_NAV_CMD_TOPICID)
#define CENTURIO_NAV_SEND_HK_MID CFE_PLATFORM_CMD_TOPICID_TO_MIDV(CFE_MISSION_CENTURIO_NAV_SEND_HK_TOPICID)
#define CENTURIO_NAV_HK_TLM_MID  CFE_PLATFORM_TLM_TOPICID_TO_MIDV(CFE_MISSION_CENTURIO_NAV_HK_TLM_TOPICID)

#endif
