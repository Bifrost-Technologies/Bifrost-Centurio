/************************************************************************
 * Internal config for centurio_nav
 ************************************************************************/
#ifndef CENTURIO_NAV_INTERNAL_CFG_H
#define CENTURIO_NAV_INTERNAL_CFG_H

#define CENTURIO_NAV_CFG_MAX_VERSION_STR_LEN 64

/* Control loop tuning (wakeup driven by sch_lab) */
#define CENTURIO_NAV_CONTROL_RATE_HZ    10   /* must match sch_lab wakeup rate      */
#define CENTURIO_NAV_MAX_SPEED_MS       20.0f /* max horizontal/vertical speed (m/s) */
#define CENTURIO_NAV_POS_GAIN           0.5f  /* proportional gain, dist -> velocity */
#define CENTURIO_NAV_ATT_SLEW_DPS       30.0f /* attitude slew rate (deg/s)          */
#define CENTURIO_NAV_ARRIVAL_M          1.0f  /* arrival radius (m)                  */

#endif
