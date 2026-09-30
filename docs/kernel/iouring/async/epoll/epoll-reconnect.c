#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <poll.h>
#include <sys/wait.h>
#include <time.h>
#include <err.h>
#include <assert.h>

#define SOCKET_PATH "/tmp/epoll_socket"
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
#define DEFAULT_CLIENTS 9

// Helper function to remove client fd from tracking array
void remove_client_fd(int *client_fds, int fd, int max_size)
{
	for (int j = 0; j < max_size; j++) {
		if (client_fds[j] == fd) {
			client_fds[j] = 0;
			break;
		}
	}
}

// Helper function to add client fd to tracking array
void add_client_fd(int *client_fds, int fd, int max_size)
{
	for (int j = 0; j < max_size; j++) {
		if (client_fds[j] == 0) {
			client_fds[j] = fd;
			break;
		}
	}
}

// Helper function to setup socket address structure
void setup_socket_addr(struct sockaddr_un *addr, const char *path)
{
	memset(addr, 0, sizeof(*addr));
	addr->sun_family = AF_UNIX;
	strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);
}

// Server function
void server_function()
{
	struct sockaddr_un addr;
	int server_sock, client_sock;

	if ((server_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	setup_socket_addr(&addr, SOCKET_PATH);

	unlink(SOCKET_PATH);
	if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		err(1, "bind");

	if (listen(server_sock, 5) == -1)
		err(1, "listen");

	printf("Server listening on %s\n", SOCKET_PATH);

	int server_epoll_fd = epoll_create1(0);
	if (server_epoll_fd == -1)
		err(1, "epoll_create1");

	struct epoll_event server_ev;
	server_ev.events = EPOLLIN;
	server_ev.data.fd = server_sock;
	if (epoll_ctl(server_epoll_fd, EPOLL_CTL_ADD, server_sock,
		      &server_ev) == -1)
		err(1, "epoll_ctl: add server socket");

	struct epoll_event events[MAX_EVENTS];

	int client_fds[MAX_EVENTS * 10] = { 0 };
	int connections_closed = 0;
	time_t start_time = time(NULL);
	while (1) {
		if (!connections_closed && time(NULL) - start_time > 3) {
			/*
			 * 如果去掉 close(server_sock)，那么会出现一个诡异的问题
			 * 就是会有 client 连到这个 socket 上。
			 */
			close(server_sock);
			exit(1);
		}

		int nfds =
			epoll_wait(server_epoll_fd, events, MAX_EVENTS, 1000);
		if (nfds == -1) {
			perror("epoll_wait server");
			break;
		}

		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == server_sock) {
				// New client connection
				client_sock = accept(server_sock, NULL, NULL);
				if (client_sock == -1) {
					perror("accept");
					continue;
				}

				// Add client socket to epoll
				struct epoll_event client_ev;
				client_ev.events = EPOLLIN | EPOLLERR |
						   EPOLLHUP;
				client_ev.data.fd = client_sock;
				if (epoll_ctl(server_epoll_fd, EPOLL_CTL_ADD,
					      client_sock, &client_ev) == -1)
					err(1, "epoll_ctl: add client socket");

				add_client_fd(client_fds, client_sock,
					      MAX_EVENTS * 10);

				printf("New client connected, fd: %d\n",
				       client_sock);
			} else {
				char buffer[BUFFER_SIZE];
				ssize_t bytes_read = read(events[i].data.fd,
							  buffer,
							  sizeof(buffer) - 1);
				assert(bytes_read > 0);
				printf("Server received from client fd %d: %s\n",
				       events[i].data.fd, buffer);
			}
		}
	}
}

typedef struct {
	int socket_fd;
	int epoll_fd;
	int running;
	char *socket_path;
} socket_monitor_t;

void *socket_monitor_thread(void *arg)
{
	socket_monitor_t *monitor = (socket_monitor_t *)arg;
	struct epoll_event events[MAX_EVENTS];

	printf("Socket monitor thread started\n");

	while (monitor->running) {
		int nfds =
			epoll_wait(monitor->epoll_fd, events, MAX_EVENTS, 1000);
		if (nfds == -1) {
			if (errno != EINTR) {
				perror("epoll_wait");
			}
			continue;
		}

		for (int i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLERR ||
			    events[i].events & EPOLLHUP) {
				printf("get events %x\n", events[i].events);
				// 接受的是 EPOLLHUP 信号

				// Close the current socket
				close(monitor->socket_fd);

				// Attempt to reconnect
				int new_sock = socket(AF_UNIX, SOCK_STREAM, 0);
				if (new_sock == -1) {
					perror("socket (reconnect)");
					sleep(1);
					continue;
				}

				struct sockaddr_un addr;
				setup_socket_addr(&addr, monitor->socket_path);

				if (!connect(new_sock, (struct sockaddr *)&addr,
					     sizeof(addr))) {
					printf("we found the it\n");
				} else {
					printf("reconnect failed\n");
				}
			}
		}
	}
	printf("Socket monitor thread exiting\n");
	return NULL;
}

// Client function
void client_function(int client_id)
{
	struct sockaddr_un addr;
	int client_sock;
	pthread_t monitor_thread;
	socket_monitor_t monitor;

	// Client process - create client socket and connect
	setup_socket_addr(&addr, SOCKET_PATH);

	client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_sock == -1) {
		perror("client socket");
		exit(1);
	}

	if (connect(client_sock, (struct sockaddr *)&addr, sizeof(addr)) ==
	    -1) {
		perror("client connect");
		exit(1);
	}

	printf("Client %d connected to server\n", client_id);

	// Create epoll instance for monitoring
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		perror("epoll_create1");
		close(client_sock);
		exit(1);
	}

	// Add client socket to epoll
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	ev.data.fd = client_sock;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev) == -1) {
		perror("epoll_ctl: add client_sock");
		close(client_sock);
		close(epoll_fd);
		exit(1);
	}

	// Initialize monitor structure
	monitor.socket_fd = client_sock;
	monitor.epoll_fd = epoll_fd;
	monitor.running = 1;
	monitor.socket_path = SOCKET_PATH;

	// Create monitoring thread
	if (pthread_create(&monitor_thread, NULL, socket_monitor_thread,
			   &monitor) != 0)
		err(1, "pthread_create");

	if (write(client_sock, "hello", 5) < 0)
		err(1, "write");

	sleep(8);
	exit(0);
}

int main(int argc, char *argv[])
{
	pid_t server_pid;
	int num_clients = DEFAULT_CLIENTS;
	int opt;

	while ((opt = getopt(argc, argv, "n:")) != -1) {
		switch (opt) {
		case 'n':
			num_clients = atoi(optarg);
			if (num_clients <= 0) {
				fprintf(stderr,
					"Invalid number of clients: %s\n",
					optarg);
				exit(1);
			}
			break;
		default:
			fprintf(stderr, "Usage: %s [-n number_of_clients]\n",
				argv[0]);
			exit(1);
		}
	}

	signal(SIGPIPE, SIG_IGN);

	// Fork to create server process
	server_pid = fork();
	if (server_pid == 0) {
		// Server process
		server_function();
	} else if (server_pid > 0) {
		// Main process - fork client processes
		sleep(1); // Give server time to start

		// Array to store client process IDs
		pid_t *client_pids = malloc(num_clients * sizeof(pid_t));
		if (client_pids == NULL) {
			perror("malloc");
			exit(1);
		}

		// Fork client processes
		for (int i = 0; i < num_clients; i++) {
			client_pids[i] = fork();
			if (client_pids[i] == 0) {
				// Client process
				client_function(i);
			} else if (client_pids[i] < 0) {
				perror("fork client");
				exit(1);
			}
		}

		// Main process - wait for server and client processes to complete
		int status;
		pid_t result;
		int server_finished = 0;
		int clients_finished = 0;

		// Wait for all processes
		while (!server_finished || clients_finished < num_clients) {
			result = wait(&status);
			if (result == server_pid) {
				printf("Server process terminated\n");
				server_finished = 1;
			} else if (result > 0) {
				// Check if it's one of the client processes
				for (int i = 0; i < num_clients; i++) {
					if (result == client_pids[i]) {
						clients_finished++;
						printf("Client process %d terminated (remaining: %d)\n",
						       i,
						       num_clients -
							       clients_finished);
						break;
					}
				}
			} else if (result == -1) {
				// Error or no more children
				if (errno == ECHILD) {
					// No more children to wait for
					break;
				}
			}
		}
		printf("Main process exiting\n");
	} else {
		perror("fork server");
		exit(1);
	}

	return 0;
}
