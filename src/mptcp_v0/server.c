#include "expr_time.h"
#include "timer.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/mptcp.h>
#include <linux/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define SOL_TCP 6 /* TCP level */
#define MAX_EVENTS 10
#define DEFAULT_TCP_BLKSIZE (128 * 1024)
#define SEC_TO_US 1000000LL
int server_sock, nfds, epollfd;
struct epoll_event ev, events[MAX_EVENTS];
void signalHandler(int signum) {
  close(server_sock);
  for (int n = 0; n < nfds; ++n) {
    close(events[n].data.fd);
  }
  exit(signum);
}
static int setnonblocking(int fd);
int connection_handler();
int main(int argc, char **argv) {
  int PORT;
  int client_sock, flag = 1;
  struct sockaddr_in server_addr, client_addr;

  int len, addr_len, recv_len, ret;
  struct timespec start, end;
  FILE *file;
  int fsize = 0, nsize = 0;
  char buffer[1024];

  int enable = 1;

  if (argc != 2) {
    fprintf(stderr, "usage: %s [port_number]\n", argv[0]);
    return -1;
  }
  PORT = atoi(argv[1]);

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("[server] socket() ");
    return -1;
  }

  signal(SIGINT, signalHandler);
  memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("192.168.1.249");
  server_addr.sin_port = htons(PORT);

  ret = bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr_in));
  if (ret < 0) {
    perror("[server] bind() ");
    return -1;
  }
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) <
      0) {
    return -1;
  }
  if (setsockopt(server_sock, SOL_TCP, MPTCP_ENABLED, &enable, sizeof(int)) <
      0) {
    return -1;
  }
  ret = listen(server_sock, MAX_EVENTS);
  if (ret < 0) {
    perror("[server] listen() ");
    return -1;
  }
  clock_gettime(0, &start);
  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  addr_len = sizeof(struct sockaddr_in);
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = server_sock;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_sock, &ev) == -1) {
    perror("epoll_ctl: server_sock");
    exit(EXIT_FAILURE);
  }
  while (1) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
      printf("n:%d, fd: %d", n, events[n].data.fd);
      fflush(stdout);
      if (events[n].data.fd == server_sock) {
        client_sock =
            accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        printf("[server] connected client %s:%d\n",
               (char *)inet_ntoa((struct in_addr)client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        if (client_sock == -1) {
          perror("accept");
          exit(EXIT_FAILURE);
        }
        setnonblocking(client_sock);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_sock;
        printf("Add cfd: %d\n", events[n].data.fd);
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &ev) == -1) {
          perror("epoll_ctl: client_sock");
          exit(EXIT_FAILURE);
        }
      } else {
        printf("cfd: %d\n", events[n].data.fd);
        fflush(stdout);
        connection_handler(events[n].data.fd);
      }
    }
  }

  close(server_sock);

  return 0;
}

static int setnonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    return -1;
  }

  flags |= O_NONBLOCK;
  int s = fcntl(fd, F_SETFL, flags);
  if (s == -1) {
    perror("fcntl");
    return -1;
  }
  return 0;
}
void printStatus(TimerClientData client_data, struct expr_time *nowP) {
  printf("alive%c\n", client_data.p);
  fflush(stdout);
}
int connection_handler(int sock) {
  struct expr_time now;
  TimerClientData cd;
  expr_time_now(&now);

  int read_size;
  char *message, client_message[DEFAULT_TCP_BLKSIZE];
  cd.p = message;
  Timer *tmr = tmr_create(&now, printStatus, cd, 1 * SEC_TO_US, 1);
  // Receive a message from client
  while ((read_size = recv(sock, client_message, DEFAULT_TCP_BLKSIZE, 0)) !=
         0) {
    expr_time_now(&now);
    // printf("hi\n");
    tmr_run(&now);
    memset(client_message, '\0', DEFAULT_TCP_BLKSIZE);
    // fprintf(stderr, "%s", client_message);
    // fflush(stdout);
  }

  if (read_size == 0) {
    puts("Client disconnected");
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }
  close(sock);
  epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, &ev);
  return 0;
}