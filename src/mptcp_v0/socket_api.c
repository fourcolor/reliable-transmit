#include "socket_api.h"
#include "expr_time.h"
#include "timer.h"
#include <errno.h>

#include <netinet/in.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#define SOL_TCP 6 /* TCP level */
#define DEFAULT_TCP_BLKSIZE (32 * 1024)
#define SEC_TO_US 1000000LL
/*
 * timeout_connect adapted from netcat, via OpenBSD and FreeBSD
 * Copyright (c) 2001 Eric Jackson <ericj@monkey.org>
 */
int timeout_connect(int s, const struct sockaddr *name, socklen_t namelen,
                    int timeout) {
  struct pollfd pfd;
  socklen_t optlen;
  int flags, optval;
  int ret;

  flags = 0;
  if (timeout != -1) {
    flags = fcntl(s, F_GETFL, 0);
    if (fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1)
      return -1;
  }

  if ((ret = connect(s, name, namelen)) != 0) {
    pfd.fd = s;
    pfd.events = POLLOUT;
    if ((ret = poll(&pfd, 1, timeout)) == 1) {
      optlen = sizeof(optval);
      if ((ret = getsockopt(s, SOL_SOCKET, SO_ERROR, &optval, &optlen)) == 0) {
        ret = optval == 0 ? 0 : -1;
      }
    } else if (ret == 0) {
      ret = -1;
    } else
      ret = -1;
  }

  if (timeout != -1 && fcntl(s, F_SETFL, flags) == -1)
    ret = -1;

  return (ret);
}

int Nsendfile(int tofd, int fromfd, size_t count) {
  register ssize_t r;
  register size_t nleft;
  off_t offset;
  nleft = count;

  while (nleft > 0) {
    offset = count - nleft;
    r = sendfile(tofd, fromfd, &offset, nleft);
    if (r > 0)
      nleft -= r;
    if (r < 0) {
      switch (errno) {
      case EINTR:
      case EAGAIN:
        if (count == nleft)
          return NET_SOFTERROR;
        return count - nleft;

      case ENOBUFS:
      case ENOMEM:
        return NET_SOFTERROR;

      default:
        return NET_HARDERROR;
      }
    } else if (r == 0)
      return NET_SOFTERROR;
  }
  return count;
}

int create_sock_connect(config_t *config) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(config->shost);
  addr.sin_port = htons(config->sport);
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("[client] socket() ");
    return -1;
  }
  if (config->mp_enable) {
    // MPTCP_ENABLED
    int flag = 1;
    int ret = setsockopt(sock, SOL_TCP, MPTCP_ENABLED, &flag, sizeof(int));
    if (ret < 0) {
      perror("[client] setsockopt(MPTCP_ENABLED) ");
      return -1;
    }

    // MPTCP_SCHEDULER
    ret = setsockopt(sock, SOL_TCP, MPTCP_SCHEDULER, config->scheduler,
                     strlen(config->scheduler));
    if (ret < 0) {
      perror("[client] setsockopt(MPTCP_SCHEDULER) ");
      return -1;
    }
    flag = MPTCP_INFO_FLAG_SAVE_MASTER;
    ret = setsockopt(sock, IPPROTO_TCP, MPTCP_INFO, &flag, sizeof(int));
    if (ret < 0) {
      perror("[client] setsockopt(MPTCP_INFO) ");
      return -1;
    }
  }

  // set TCP_NODELAY for lower latency on control messages
  int flag = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int))) {
    perror("[client] setsockopt(TCP_NODELAY) ");
    return -1;
  }
  if (timeout_connect(sock, (struct sockaddr *)&addr, sizeof(addr),
                      config->timeout) < 0) {
    close(sock);
    perror("[client] timeout_connect()");
  }

  return sock;
}

void check_throttle(config_t *config, struct expr_time *nowP) {
  struct expr_time temp_time;
  double seconds;
  uint64_t bits_per_second;

  if (config->bitrate == 0)
    return;
  expr_time_diff(&config->start, nowP, &temp_time);
  seconds = expr_time_in_secs(&temp_time);
  bits_per_second = config->bytes_sent * 8 / seconds;

  if (bits_per_second < config->bitrate) {
    config->green_light = 1;
    FD_SET(config->sock, &config->write_set);
  } else {
    config->green_light = 0;
    FD_CLR(config->sock, &config->write_set);
  }
}
int Nwrite(int fd, const char *buf, size_t count) {
  register ssize_t r;
  register size_t nleft = count;

  while (nleft > 0) {
    r = write(fd, buf, nleft);
    if (r < 0) {
      switch (errno) {
      case EINTR:
      case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
      case EWOULDBLOCK:
#endif
        return count - nleft;

      case ENOBUFS:
        return NET_SOFTERROR;

      default:
        return NET_HARDERROR;
      }
    } else if (r == 0)
      return NET_SOFTERROR;
    nleft -= r;
    buf += r;
  }
  return count;
}

int Nsend(config_t *config) {
  int r;
  struct expr_time now;
  if (config->pending_size <= 0)
    config->pending_size = 200;
  expr_time_now(&now);

  // r = Nsendfile(config->sock, config->infilefd, config->pending_size);
  if (config->green_light && FD_ISSET(config->sock, &config->write_set)) {
    r = Nwrite(config->sock, config->buffer, config->pending_size);
    if (r < 0)
      return r;
    config->pending_size -= r;
    config->bytes_sent += r;
  }
  if (config->bitrate)
    check_throttle(config, &now);

  return r;
}

#define PARSE_ADDR_TO_IP(addr, str)                                            \
  snprintf(str, 18, "%d.%d.%d.%d", ((unsigned char *)&addr)[0],                \
           ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2],           \
           ((unsigned char *)&addr)[3]);

void subflow_info(TimerClientData client_data, struct expr_time *nowP) {
  config_t *config = (config_t *)client_data.p;
  struct mptcp_info minfo = config->minfo;
  char s_ip[18] = {'\0'}, c_ip[18] = {'\0'};
  socklen_t len = sizeof(minfo);
  if (getsockopt(config->sock, IPPROTO_TCP, MPTCP_INFO, &minfo,
                 &config->mp_len) >= 0) {
    int subflows_info_len = minfo.total_sub_info_len;
    int subflows_number = subflows_info_len / sizeof(struct mptcp_sub_info);
    int i;
    for (i = 0; i < subflows_number; i++) {
      PARSE_ADDR_TO_IP(minfo.subflow_info[i].src_v4.sin_addr, c_ip);
      PARSE_ADDR_TO_IP(minfo.subflow_info[i].dst_v4.sin_addr, s_ip);
      printf("%d,%s:%d,%s:%d,%d,%d,%d,%d,%d,%d\n", i, c_ip,
             ntohs(minfo.subflow_info[i].src_v4.sin_port), s_ip,
             ntohs(minfo.subflow_info[i].dst_v4.sin_port),
             minfo.subflows[i].tcpi_rto, minfo.subflows[i].tcpi_ato,
             minfo.subflows[i].tcpi_rtt, minfo.subflows[i].tcpi_snd_mss,
             minfo.subflows[i].tcpi_rcv_mss,
             minfo.subflows[i].tcpi_total_retrans);
      fflush(stdout);
      memset(minfo.subflows,'\0',sizeof(subflows_info_len));
    }
  } else {
    perror("getsockopt");
  }
}

int timer_setting(config_t *config) {
  struct expr_time now;
  TimerClientData cd;
  cd.p = config;
  if (expr_time_now(&now) < 0) {
    return -1;
  }
  config->reporter_timer = tmr_create(&now, subflow_info, cd, 1 * SEC_TO_US, 1);
  printf("subflow_id,caddr,saddr,rto,ato,rtt,snd_mss,rcv_mss,total_retrans,\n");
  fflush(stdout);
}

int create_infd(config_t *config) {
  int fd;
  char template[1024];

  snprintf(template, sizeof(template) / sizeof(char), "/tmp/ntu_expr.XXXXXX");
  fd = mkstemp(template);
  if (fd == -1) {
    return -1;
  }
  if (unlink(template) < 0) {
    return -1;
  }
  if (ftruncate(fd, DEFAULT_TCP_BLKSIZE) < 0) {
    return -1;
  }
  config->infilefd = fd;
  config->buffer = malloc(sizeof(char) * (DEFAULT_TCP_BLKSIZE));
  memset(config->buffer, '1', (DEFAULT_TCP_BLKSIZE));
  if (config->buffer == MAP_FAILED) {
    perror("mmap()");
    return 1;
  }
  return fd;
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

int client_sock_run(config_t *config) {
  struct timeval timeout;
  struct expr_time now;
  struct expr_time last_receive_time;

  int result = 0;
  FD_ZERO(&config->write_set);
  expr_time_now(&now);
  config->start = now;
  config->bytes_sent = 0;

  if (create_infd(config) < 0) {
    return -1;
  }

  config->sock = create_sock_connect(config);
  if (config->sock < 0) {
    perror("[client] socket() ");
    return -1;
  }

  if (setnonblocking(config->sock) < 0) {
    return -1;
  }

  FD_SET(config->sock, &config->write_set);
  timer_setting(config);
  while (config->state != IPERF_DONE) {
    expr_time_now(&now);
    timeout.tv_sec = 1;
    result = select(config->sock + 1, NULL, &config->write_set, NULL, &timeout);

    if (Nsend(config) < -1) {
      perror("[client] Nsend");
      return -1;
    }
    expr_time_now(&now);
    tmr_run(&now);
  }
}