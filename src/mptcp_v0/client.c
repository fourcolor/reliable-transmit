#include "config.h"
#include "socket_api.h"
#include <bits/getopt_core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define SOL_TCP 6 /* TCP level */
#define NIC_NUMBER 4
#define MPTCP_INFO_FLAG_SAVE_MASTER 0x01

int sock;
void signalHandler(int signum);
void help() {
  printf("-h: help\n");
  printf("-e: mptcp enable\n");
  printf("-s ip: server port\n");
  printf("-b rate: bitrate\n");
  printf("-S sched: Scheduler\n");
  printf("-P pm: PathManage\n");
  printf("-n size: subflow size\n");
};
int main(int argc, char **argv) {
  int c;
  char *addr = NULL;
  int port = -1;
  char *scheduler = "default";
  char *pm = "fullmesh";
  int bitrate = 0;
  int subflow_num = 1;
  int mptcp_enable = 0;

  while ((c = getopt(argc, argv, "hes:p:b:S:P:n:")) != -1) {
    switch (c) {
    case 'h':
      help();
      exit(-1);
      break;
    case 'e':
      mptcp_enable = 1;
      break;
    case 's':
      addr = strdup(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'b':
      bitrate = atoi(optarg);
      break;
    case 'S':
      scheduler = strdup(optarg);
      break;
    case 'P':
      pm = strdup(optarg);
      break;
    case 'n':
      subflow_num = atoi(optarg);
      break;
    }
  }

  if (optind == 1 || port == -1 || addr == NULL) {
    printf("Error:-p and -s is required\n");
    help();
    exit(-1);
  }

  struct mptcp_info minfo;
  struct mptcp_meta_info meta_info;
  struct tcp_info initial;
  struct tcp_info *others = malloc(subflow_num * sizeof(struct tcp_info));
  struct mptcp_sub_info *others_info =
      malloc(subflow_num * sizeof(struct mptcp_sub_info));

  // subflow setup
  minfo.tcp_info_len = sizeof(struct tcp_info);
  minfo.sub_len = subflow_num * sizeof(struct tcp_info);
  minfo.meta_len = sizeof(struct mptcp_meta_info);
  minfo.meta_info = &meta_info;
  minfo.initial = &initial;
  minfo.subflows = others;
  minfo.sub_info_len = sizeof(struct mptcp_sub_info);
  minfo.total_sub_info_len = subflow_num * sizeof(struct mptcp_sub_info);
  minfo.subflow_info = others_info;

  config_t config;
  config.minfo = minfo;
  config.bitrate = bitrate;
  config.scheduler = scheduler;
  config.subflow_num = subflow_num;
  config.mp_enable = mptcp_enable;
  config.shost = addr;
  config.sport = port;
  config.timeout = 3000;
  config.mp_len = sizeof(minfo);
  client_sock_run(&config);
  return 0;
}
