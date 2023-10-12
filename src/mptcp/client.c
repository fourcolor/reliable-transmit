#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <linux/mptcp.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
// #include <netinet/tcp.h>
#define SOL_TCP 6 /* TCP level */
#define NIC_NUMBER 2
#define MPTCP_INFO_FLAG_SAVE_MASTER 0x01
void print_subflow_info(struct mptcp_info);
int get_fsize(FILE *);

int main(int argc, char **argv)
{
    char *ADDR;
    int PORT;
    char *FILE_PATH;

    int sock;
    struct sockaddr_in addr;
    int ret;

    FILE *file;
    char send_buff[1024] = {
        '\0',
    };
    int fsize = 0, nsize = 0;

    int enable = 1;
    int val = MPTCP_INFO_FLAG_SAVE_MASTER;

    char *scheduler = "default";

    struct mptcp_info minfo;
    struct mptcp_meta_info meta_info;
    struct tcp_info initial;
    struct tcp_info others[NIC_NUMBER];
    struct mptcp_sub_info others_info[NIC_NUMBER];
    socklen_t len;

    struct timespec start, end;

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
    len = sizeof(minfo);

    if (argc != 4)
    {
        fprintf(stderr, "usage: %s [host_address] [port_number] [file_path]\n",
                argv[0]);
        return -1;
    }
    ADDR = argv[1];
    PORT = atoi(argv[2]);
    FILE_PATH = argv[3];

    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("[client] socket() ");
        return -1;
    }

    // MPTCP_ENABLED
    ret = setsockopt(sock, SOL_TCP, MPTCP_ENABLED, &enable, sizeof(int));
    if (ret < 0)
    {
        perror("[client] setsockopt(MPTCP_ENABLED) ");
        return -1;
    }

    // MPTCP_SCHEDULER
    ret =
        setsockopt(sock, SOL_TCP, MPTCP_SCHEDULER, scheduler, strlen(scheduler));
    if (ret < 0)
    {
        perror("[client] setsockopt(MPTCP_SCHEDULER) ");
        return -1;
    }

    // Enable subflow info
    ret = setsockopt(sock, IPPROTO_TCP, MPTCP_INFO, &val, sizeof(val));
    if (ret < 0)
    {
        perror("[client] setsockopt(MPTCP_INFO) ");
        return -1;
    }

    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ADDR);
    addr.sin_port = htons(PORT);

    ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        perror("[client] connect() ");
        return -1;
    }
    printf("[client] connected\n");

    file = fopen(FILE_PATH, "rb");
    if (file == NULL)
    {
        perror("[client] fopen() ");
        return -1;
    }

    fsize = get_fsize(file);

    clock_gettime(0, &start);
    printf("[client] sending file...(%s)\n", FILE_PATH);
    while (nsize != fsize)
    {
        int fpsize = fread(send_buff, 1, 1024, file);
        nsize += fpsize;
        send(sock, send_buff, fpsize, 0);

        clock_gettime(0, &end);
        if (1 < (end.tv_sec - start.tv_sec))
        {
            clock_gettime(0, &start);
            getsockopt(sock, IPPROTO_TCP, MPTCP_INFO, &minfo, &len);
            print_subflow_info(minfo);
        }
    }

    fclose(file);
    close(sock);

    return 0;
}

void print_subflow_info(struct mptcp_info minfo)
{
    int subflows_info_len = minfo.total_sub_info_len;
    int subflows_number = subflows_info_len / sizeof(struct mptcp_sub_info);
    int i;

    printf("-----------------------\n");
    printf("[subflows number] %d\n", subflows_number);
    for (i = 0; i < subflows_number; i++)
    {
        printf("[subflow %d] %s:%d ", i,
               inet_ntoa(minfo.subflow_info[i].src_v4.sin_addr),
               ntohs(minfo.subflow_info[i].src_v4.sin_port));
        printf("-> %s:%d", inet_ntoa(minfo.subflow_info[i].dst_v4.sin_addr),
               ntohs(minfo.subflow_info[i].dst_v4.sin_port));

        printf(
            "  { total_sub: %d, rto %d, ato %d , rtt %d, snd_mss %d, rcv_mss %d, "
            "total_retrans %d "
            "}\n",
            subflows_number, minfo.subflows[i].tcpi_rto, minfo.subflows[i].tcpi_ato,
            minfo.subflows[i].tcpi_rtt, minfo.subflows[i].tcpi_snd_mss,
            minfo.subflows[i].tcpi_rcv_mss, minfo.subflows[i].tcpi_total_retrans);
    }
}

int get_fsize(FILE *file)
{
    int fsize;

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    return fsize;
}
