// https://man7.org/linux/man-pages/man2/timerfd_create.2.html
//
// https://stackoverflow.com/questions/63241720/how-to-use-timerfd-properly
// 中代码:
#include <stdio.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

int main()
{
	int tfd = timerfd_create(CLOCK_REALTIME, 0);

	printf("Starting at (%d)...\n", (int)time(NULL));

	if (tfd <= 0)
		return -1;

	char dummybuf[8];
	struct itimerspec spec = {
		{ 5, 0 }, // Set to {0, 0} if you need a one-shot timer
		{ 4, 0 }
	};
	timerfd_settime(tfd, 0, &spec, NULL);

	/* Wait */
	fd_set rfds;
	int retval;

	/* Watch timefd file descriptor */
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	FD_SET(tfd, &rfds);

	/* Let's wait for initial timer expiration */
	retval = select(tfd + 1, &rfds, NULL, NULL,
			NULL); /* Last parameter = NULL --> wait forever */
	printf("Expired at %d! (%d) (%ld)\n", (int)time(NULL), retval,
	       read(tfd, dummybuf, 8));

	/* Let's wait (twice) for periodic timer expiration */
	retval = select(tfd + 1, &rfds, NULL, NULL, NULL);
	printf("Expired at %d! (%d) (%ld)\n", (int)time(NULL), retval,
	       read(tfd, dummybuf, 8));

	retval = select(tfd + 1, &rfds, NULL, NULL, NULL);
	printf("Expired at %d! (%d) (%ld)\n", (int)time(NULL), retval,
	       read(tfd, dummybuf, 8));

	return 0;
}
