#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <netlink/socket.h>
#include <netlink/netlink.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include "./genltest.h"

#define prerr(...) fprintf(stderr, "error: " __VA_ARGS__)

/*
 * libnl docs and API: https://www.infradead.org/~tgr/libnl/
 * Current libnl repo: https://github.com/thom311/libnl
 */

/*
 * Handler for all received messages from our Generic Netlink family, both
 * unicast and multicast.
 */
static int echo_reply_handler(struct nl_msg *msg, void *arg)
{
	int err = 0;
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[GENLTEST_A_MAX + 1];

	/* Parse the attributes */
	err = nla_parse(tb, GENLTEST_A_MAX, genlmsg_attrdata(genlhdr, 0),
			genlmsg_attrlen(genlhdr, 0), NULL);
	if (err) {
		prerr("unable to parse message: %s\n", strerror(-err));
		return NL_SKIP;
	}
	/* Check that there's actually a payload */
	if (!tb[GENLTEST_A_MSG]) {
		prerr("msg attribute missing from message\n");
		return NL_SKIP;
	}

	/* Print it! */
	printf("message received: %s\n", nla_get_string(tb[GENLTEST_A_MSG]));

	return NL_OK;
}

/* Send (unicast) GENLTEST_CMD_ECHO request message */
static int send_echo_msg(struct nl_sock *sk, int fam)
{
	int err = 0;
	struct nl_msg *msg = nlmsg_alloc();
	if (!msg) {
		return -ENOMEM;
	}

	/* Put the genl header inside message buffer */
	void *hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, fam, 0, 0,
				GENLTEST_CMD_ECHO, GENLTEST_GENL_VERSION);
	if (!hdr) {
		return -EMSGSIZE;
	}

	/* Put the string inside the message. */
	err = nla_put_string(msg, GENLTEST_A_MSG,
			     "Hello from User Space, Netlink!");
	if (err < 0) {
		return -err;
	}
	printf("message sent\n");

	/* Send the message. */
	err = nl_send_auto(sk, msg);
	err = err >= 0 ? 0 : err;

	nlmsg_free(msg);

	return err;
}

/* Allocate netlink socket and connect to generic netlink */
static int conn(struct nl_sock **sk)
{
	*sk = nl_socket_alloc();
	if (!sk) {
		return -ENOMEM;
	}

	return genl_connect(*sk);
}

/* Disconnect and release socket */
static void disconn(struct nl_sock *sk)
{
	nl_close(sk);
	nl_socket_free(sk);
}

/* Modify the callback for replies to handle all received messages */
static inline int set_cb(struct nl_sock *sk)
{
	return nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM,
				   echo_reply_handler, NULL);
}

int main(void)
{
	int ret = 1;
	struct nl_sock *ucsk, *mcsk;

	/*
	 * We use one socket to receive asynchronous "notifications" over
	 * multicast group, and another for ops. We do this so that we don't mix
	 * up responses from ops with notifications to make handling easier.
	 */
	if ((ret = conn(&ucsk)) || (ret = conn(&mcsk))) {
		prerr("failed to connect to generic netlink\n");
		goto out;
	}

	/* Resolve the genl family. One family for both unicast and multicast. */
	int fam = genl_ctrl_resolve(ucsk, GENLTEST_GENL_NAME);
	if (fam < 0) {
		prerr("failed to resolve generic netlink family: %s\n",
		      strerror(-fam));
		goto out;
	}

	/* Disable sequence checks for asynchronous multicast messages. */
	nl_socket_disable_seq_check(mcsk);

	/* Resolve the multicast group. */
	int mcgrp = genl_ctrl_resolve_grp(mcsk, GENLTEST_GENL_NAME,
					  GENLTEST_MC_GRP_NAME);
	if (mcgrp < 0) {
		prerr("failed to resolve generic netlink multicast group: %s\n",
		      strerror(-mcgrp));
		goto out;
	}
	/* Join the multicast group. */
	if ((ret = nl_socket_add_membership(mcsk, mcgrp) < 0)) {
		prerr("failed to join multicast group: %s\n", strerror(-ret));
		goto out;
	}

	if ((ret = set_cb(ucsk)) || (ret = set_cb(mcsk))) {
		prerr("failed to set callback: %s\n", strerror(-ret));
		goto out;
	}

	/* Send unicast message and listen for response. */
	if ((ret = send_echo_msg(ucsk, fam))) {
		prerr("failed to send message: %s\n", strerror(-ret));
	}
	printf("listening for messages\n");
	nl_recvmsgs_default(ucsk);

	/* Listen for "notifications". */
	while (1) {
		nl_recvmsgs_default(mcsk);
	}

	ret = 0;
out:
	disconn(ucsk);
	disconn(mcsk);
	return ret;
}
