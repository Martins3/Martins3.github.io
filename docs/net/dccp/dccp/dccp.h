/* This file only contains constants necessary for user space to call
 * into the kernel and thus, contains no copyrightable information. */

#ifndef DCCP_DCCP_H
#define DCCP_DCCP_H

// From the kernel's include/linux/socket.h
#define SOL_DCCP                        269

// From kernel's include/uapi/linux/dccp.h
#define DCCP_SOCKOPT_SERVICE            2
#define DCCP_SOCKOPT_CHANGE_L           3
#define DCCP_SOCKOPT_CHANGE_R           4
#define DCCP_SOCKOPT_GET_CUR_MPS        5
#define DCCP_SOCKOPT_SERVER_TIMEWAIT    6
#define DCCP_SOCKOPT_SEND_CSCOV         10
#define DCCP_SOCKOPT_RECV_CSCOV         11
#define DCCP_SOCKOPT_AVAILABLE_CCIDS    12
#define DCCP_SOCKOPT_CCID               13
#define DCCP_SOCKOPT_TX_CCID            14
#define DCCP_SOCKOPT_RX_CCID            15
#define DCCP_SOCKOPT_QPOLICY_ID         16
#define DCCP_SOCKOPT_QPOLICY_TXQLEN     17
#define DCCP_SOCKOPT_CCID_RX_INFO       128
#define DCCP_SOCKOPT_CCID_TX_INFO       192

#endif //DCCP_DCCP_H
