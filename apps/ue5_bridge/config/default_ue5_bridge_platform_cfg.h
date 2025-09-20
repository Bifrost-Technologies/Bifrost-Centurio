/************************************************************************
 * UE5 Bridge platform config
 ************************************************************************/
#ifndef UE5_BRIDGE_PLATFORM_CFG_H
#define UE5_BRIDGE_PLATFORM_CFG_H

#define UE5_BRIDGE_PIPE_DEPTH     32
/* Short name to stay within OSAL API name limits */
#define UE5_BRIDGE_PIPE_NAME      "UE5B_CMD"

/* UDP ports for JSON I/O (adjust as needed) */
#define UE5_BRIDGE_JSON_RX_PORT   15001
#define UE5_BRIDGE_JSON_TX_PORT   15002

#endif
