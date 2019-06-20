#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <linux/input.h>

#define NODE "/dev/input/event2"
#define NORMALIZE 4096
#define REAL_X 1080		//resolution width
#define REAL_Y 2340		//resolution height
static unsigned int FAKE_X = 720;
static unsigned int FAKE_Y = 540;

static int send_x(struct input_event event, int fd, unsigned int x)
{
	unsigned int tmp = REAL_Y * FAKE_X / FAKE_Y;

	// Report X
	memset(&event, 0, sizeof(struct input_event));
	event.type = EV_ABS;
	event.code = ABS_MT_POSITION_X;
	if (FAKE_X < REAL_X)
		event.value =
		    REAL_X / 2 - tmp / 2 +
		    (x * REAL_X / NORMALIZE * tmp / REAL_X);
	else
		event.value = x * REAL_X / NORMALIZE;
	write(fd, &event, sizeof(struct input_event));

	return 0;
}

static int send_y(struct input_event event, int fd, unsigned int y)
{
	unsigned int tmp = REAL_X * FAKE_Y / FAKE_X;

	// Report Y
	memset(&event, 0, sizeof(struct input_event));
	event.type = EV_ABS;
	event.code = ABS_MT_POSITION_Y;
	if (FAKE_Y < REAL_Y)
		event.value =
		    REAL_Y / 2 - tmp / 2 +
		    (y * REAL_Y / NORMALIZE * tmp / REAL_Y);
	else
		event.value = y * REAL_Y / NORMALIZE;
	write(fd, &event, sizeof(struct input_event));

	return 0;
}

static int send_report(struct input_event event, int fd)
{

	// Report SYN_REPORT
	memset(&event, 0, sizeof(struct input_event));
	event.type = EV_SYN;
	event.code = SYN_REPORT;
	write(fd, &event, sizeof(struct input_event));

	return 0;
}

static int send_touch(struct input_event event, int fd, int touched)
{

	// Drag start
	memset(&event, 0, sizeof(struct input_event));
	event.type = EV_KEY;
	event.code = BTN_TOUCH;
	event.value = touched;	//1 is down, 0 is up
	write(fd, &event, sizeof(struct input_event));

	return 0;
}

static int send_id(struct input_event event, int fd, int id)
{

	memset(&event, 0, sizeof(struct input_event));
	event.type = EV_ABS;
	event.code = ABS_MT_TRACKING_ID;
	event.value = id;
	write(fd, &event, sizeof(struct input_event));

	return 0;
}

static void update_fake_res()
{
	// Scale FAKE_X and FAKE_Y appropriately
	if ((double)REAL_Y / REAL_X > (double)FAKE_Y / FAKE_X) {
		FAKE_Y = FAKE_Y * REAL_X / FAKE_X;
		FAKE_X = REAL_X;
	} else {
		FAKE_X = FAKE_X * REAL_Y / FAKE_Y;
		FAKE_Y = REAL_Y;
	}
}

int main()
{
	int fd;
	char click_bool = 0;
	size_t s_read;
	char readbuf[64];
	struct input_event event;
	unsigned int x, y;
	static int id = 97;
	char wmarg[4096];

	//for socket
	int sock = 0;
	int sock_i = 0;
	int sock_size = 0;
	struct sockaddr_in serv_addr;
	char ip_addr[] = "127.0.0.1";
	int sock_opt = 1;

	// Disable buffering on stdout/stderr
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	update_fake_res();

	fd = open(NODE, O_RDWR);

	//keyboard event
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		printf("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip_addr);
	serv_addr.sin_port = htons(5678);

	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &sock_opt, sizeof(sock_opt));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) ==
	    -1)
		printf("connect() error!");
	else
		printf("server connected\n");

	memset(&readbuf, 0, sizeof(readbuf));
	while ((s_read = read(0, readbuf, 18)) != -1) {	// 0: stdin
		printf("input: read: %s\n", readbuf);

		// Parse X and Y from readbuf
		readbuf[9] = '\0';
		readbuf[18] = '\0';
		x = atoi(readbuf + 1);
		y = atoi(readbuf + 10);

		switch (readbuf[0]) {
		case '3':
			write(sock, readbuf, 18);
			break;
		case '2':
			//window resize event
			memset(wmarg, 0, sizeof(wmarg));
			FAKE_X = x;
			FAKE_Y = y;
			update_fake_res();
			sprintf(wmarg, "wm size %ux%u", x, y);
			printf("WMARG: %s\n", wmarg);
			system(wmarg);
			break;
		case '1':
			//mouse click event
			if (readbuf[1] == '1') {
				if (x == 0 && y == 0) {
					printf("Ignoring 0, 0 input\n");
					break;
				}
				id = (++id == 0xffffffff ? id = 97 : id);
				send_id(event, fd, id);
				send_x(event, fd, x);
				send_y(event, fd, y);
				send_touch(event, fd, 1);
			} else if (readbuf[1] == '0') {
				id = 0xffffffff;
				send_id(event, fd, id);
				send_touch(event, fd, 0);
			}
			break;
		case '0':
			//mouse move event
			send_x(event, fd, x);
			send_y(event, fd, y);
			break;
		}
		send_report(event, fd);
	}

	close(sock);

	return 0;
}
