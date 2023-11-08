#ifndef CONFIG_H
#define CONFIG_H
#include "expr_time.h"
// #include "mptcp.h"
#include "timer.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
// #include <netinet/tcp.h>
#include <netdb.h>
#include <linux/tcp.h>

#define EXCHANGE_DATA_SIZE 4
#define IPERF_START 15
#define IPERF_DONE 16
typedef struct config {
  int bitrate;
  uint16_t sport;
  char *shost;
  bool mp_enable;
  socklen_t mp_len;
  char *scheduler;
  int infilefd;
  int csock;
  int sock;
  int timeout;
  int subflow_num;
  char *pm;
  char *exchange_data;
  int pending_size;
  fd_set read_set;  /* set of read sockets */
  fd_set write_set; /* set of write sockets */
  int state;
  struct expr_time start;
  int green_light;
  long long bytes_sent;
  Timer *reporter_timer;
  struct mptcp_info minfo;
  char *buffer;
} config_t;
/*
    mp_enable:
        true: 1
        false: 0
    scheduler:
        default: 0
        roundrobin: 1
        redundant: 2
    subflow Num (1 digit)
    pm:
        default: 0
        fullmesh: 1
        ndiffports: 2
        binder: 3
        netlink: 4
*/
inline void config_to_exchange_data(config_t *c) {
  char *res = (char *)malloc(sizeof(char) * EXCHANGE_DATA_SIZE + 1);
  res[0] = c->mp_enable + '0';
  if (!strcmp("default", c->scheduler)) {
    res[1] = '0';
  } else if (!strcmp("roundrobin", c->scheduler)) {
    res[1] = '1';
  } else if (!strcmp("redundant", c->scheduler)) {
    res[1] = '2';
  }
  res[2] = c->subflow_num + '0';
  if (!strcmp("default", c->scheduler)) {
    res[3] = '0';
  } else if (!strcmp("fullmesh", c->scheduler)) {
    res[3] = '1';
  } else if (!strcmp("ndiffports", c->scheduler)) {
    res[3] = '2';
  }
  if (!strcmp("binder", c->scheduler)) {
    res[3] = '2';
  }
  if (!strcmp("netlink", c->scheduler)) {
    res[3] = '2';
  }
  res[4] = '\0';
  c->exchange_data = res;
}
#endif