#ifndef GENLTEST_H
#define GENLTEST_H

/*
 * This header includes definitions that are shared with kernel space and user
 * space. This header would be put in a place visible to user space.
 */

#define GENLTEST_GENL_NAME "genltest"
#define GENLTEST_GENL_VERSION 1
#define GENLTEST_MC_GRP_NAME "mcgrp"

/* Attributes */
enum genltest_attrs {
	GENLTEST_A_UNSPEC,
	GENLTEST_A_MSG,
	__GENLTEST_A_MAX,
};

#define GENLTEST_A_MAX (__GENLTEST_A_MAX - 1)

/* Commands */
enum genltest_cmds {
	GENLTEST_CMD_UNSPEC,
	GENLTEST_CMD_ECHO,
	__GENLTEST_CMD_MAX,
};

#define GENLTEST_CMD_MAX (__GENLTEST_CMD_MAX - 1)

#endif 
