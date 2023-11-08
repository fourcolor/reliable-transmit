#include "config.h"
#include "socket_api.h"
#define SOL_TCP 6 /* TCP level */
#define NIC_NUMBER 4
#define MPTCP_INFO_FLAG_SAVE_MASTER 0x01

int sock;
void signalHandler(int signum);

int main(int argc, char **argv) {
  char *ADDR;
  int PORT;
  int fsize = 0, nsize = 0;

  int enable = 1;
  int val = MPTCP_INFO_FLAG_SAVE_MASTER;

  char *scheduler = "default";

  struct mptcp_info minfo;

  ADDR = argv[1];
  PORT = atoi(argv[2]);

  struct mptcp_meta_info meta_info;
  struct tcp_info initial;
  struct tcp_info others[NIC_NUMBER];
  struct mptcp_sub_info others_info[NIC_NUMBER];

  // subflow setup
  minfo.tcp_info_len = sizeof(struct tcp_info);
  minfo.sub_len = sizeof(others);
  minfo.meta_len = sizeof(struct mptcp_meta_info);
  minfo.meta_info = &meta_info;
  minfo.initial = &initial;
  minfo.subflows = others;
  minfo.sub_info_len = sizeof(struct mptcp_sub_info);
  minfo.total_sub_info_len = sizeof(others_info);
  minfo.subflow_info = others_info;

  config_t config;
  config.minfo = minfo;
  config.bitrate = 1024;
  config.scheduler = scheduler;
  config.subflow_num = NIC_NUMBER;
  config.mp_enable = 1;
  config.shost = ADDR;
  config.sport = PORT;
  config.timeout = 3000;
  config.mp_len = sizeof(minfo);
  client_sock_run(&config);
  return 0;
}

