/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
 /** @file
  * PAL pipe functions.
  *
  * @author Jim Tan
  * @version 1.0
  */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "itp_cfg.h"

int pipe(int __fildes[2])
{
	int s = 0;
	int timeout = 0;
	static int aport = 10500;
	struct sockaddr_in raddr;
	int j;

	s = (int) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (0 > s) {
		return -1;
	}
	__fildes[1] = (int) socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (0 > __fildes[1]) {
		closesocket(s);
		return -1;
	}

	raddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	raddr.sin_family = AF_INET;

	j = 50;
	while (aport++ && j-- > 0) {
		raddr.sin_port = htons((short) aport);
		if (bind(s, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
			LOG_ERR "Failed to bind one local socket %i!\n", aport LOG_END
		} else
			break;
	}

	if (j == 0) {
		LOG_ERR "Failed to bind a local socket, aborting!\n" LOG_END
		closesocket(s);
		closesocket(__fildes[1]);
		return -1;
	}

	j = listen(s, 1);
	if (j != 0) {
		LOG_ERR "Failed to listen on a local socket, aborting!\n" LOG_END
		closesocket(s);
		closesocket(__fildes[1]);
		return -1;
	}

	j = setsockopt(__fildes[1],
				   SOL_SOCKET,
				   SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
	if (j != 0) {
		/* failed for some reason... */
		LOG_ERR "udp plugin; cannot set O_NONBLOCK to the file desciptor!\n" LOG_END
		closesocket(s);
		closesocket(__fildes[1]);
		return -1;
	}

	connect(__fildes[1], (struct sockaddr *) &raddr, sizeof(raddr));

	__fildes[0] = accept(s, NULL, NULL);

	if (__fildes[0] <= 0) {
		LOG_ERR "udp plugin; Failed to call accept!\n" LOG_END
		closesocket(s);
		closesocket(__fildes[1]);
		return -1;
	}

  closesocket (s);

	return 0;
}