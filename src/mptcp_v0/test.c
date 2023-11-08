#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>
int main() {

  int fd;
  char template[1024];
  char *tempdir = getenv("TMPDIR");
  if (tempdir == 0) {
    tempdir = getenv("TEMP");
  }
  if (tempdir == 0) {
    tempdir = getenv("TMP");
  }
  if (tempdir == 0) {
    tempdir = "/tmp";
  }
  snprintf(template, sizeof(template) / sizeof(char), "/tmp/ntu_expr.XXXXXX",
           tempdir);
  fd = mkstemp(template);
  if (fd == -1) {
    return -1;
  }
  if (unlink(template) < 0) {
    return -1;
  }
  if (ftruncate(fd, 1024) < 0) {
    return -1;
  }
  char buf[100];

  printf("%d\n", write(fd, "Hello", strlen("Hello")));
  if (-1 == lseek(fd, 0, SEEK_SET)) {
    // printf("\n lseek failed with error [%s]\n", strerror(errno));
    return -1;
  }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(26424);
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  printf("connect(): %d\n",
         connect(sock, (struct sockaddr *)&addr, sizeof(addr)));
  printf("sendfile(): %d\n", sendfile(sock, fd, 0, 1024));
  // printf("%d\n", read(fd, buf, 2025));
  printf("%s\n", buf);
  return 0;
}