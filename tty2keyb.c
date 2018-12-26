/**
 @author ktwice@mail.ru

 The program reads the COM-port and "types" the digits on the keyboard.
 The program can accept two parameters-filenames (COM-port and keyboard-events)
 from command line (space-separated) or from environment variable TTY2KEYB
 (colon-or-space)-separated.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/input.h>

#define BUF_SIZE 256
#define PARAM_COUNT 3

// param[1] - COM-port
// param[2] - keyboard-events
void param_init(char* param[], int argc, char* argv[]) {
  static char param_buf[BUF_SIZE];
// Environment var for (colon-or-space)-separated parameters
  char* ename = "TTY2KEYB";
// parameters default values
  char* def_param[] ={NULL, "/dev/ttyS0", "/dev/input/event2"};
  for(int i=0; i<PARAM_COUNT; i++) param[i] = NULL;
  if(argc > 1) {
    printf("\nCommand line %d argument(s) readed", argc-1);
    for(int i=0; i<PARAM_COUNT; i++) {
      if(i >= argc) break;
      param[i] = argv[i];
    }
  } else {
    char* p  = getenv(ename);
    if(p != NULL) {
      printf("\nEnvironment var %s readed as <%s>", ename, p);
      param[0] = p;
      p = strcpy(param_buf, p);
      for(int i=1; i<PARAM_COUNT; i++) {
        param[i] = p;
        p = strpbrk(p, ": "); // p = strchr(p, ':');
        if(p == NULL) break;
        *p++ = '\0';
      }
    } else printf("\nEnvironment var %s not found", ename);
  }
  for(int i=0; i<PARAM_COUNT; i++) {
    if(param[i] != NULL) continue;
    if(def_param[i] == NULL) continue;
    param[i] = def_param[i];
  }
  printf("\nRead from <%s>", param[1]);
  printf("\nWrite to <%s>", param[2]);
  printf(" (check to see /proc/bus/input/devices)\n");
}

int e2(int iret, char* s) {
  if(iret != -1) return iret;
  perror(s);
  exit(2);
}

void p2(char* buf, int count) {
  char s[BUF_SIZE];
  char* p = s;
  *p++ = '<';
  for(int i=0; i<count; i++) {
    int c = buf[i];
    *p++ = (c>='0' && c<='9')? c:(c==10)? ';':(c<' ')? '.':'*';
  }
  *p++ = '>';
  *p = '\0';
  puts(s);
}

int char2code(int c) {
  if(c >= '1' && c <= '9') return c - 47; // digit keys
  if(c == 10) return 28; // ENTER key
  return 11; // zero-digit[0] key
}

void char_typing(char* fname, char* buf, int count) {
  struct input_event event;
  event.type = EV_KEY;
  int fd = e2(open(fname, O_WRONLY | O_NONBLOCK), fname);
  for(int i=0; i<count; i++) {
    event.code = char2code(buf[i]);
    event.value = 1;
    e2(write(fd, &event, sizeof(event)), "write(press) error");
    event.value = 0;
    e2(write(fd, &event, sizeof(event)), "write(release) error");
  }
  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  e2(write(fd, &event, sizeof(event)), "write(syn) error");
  close(fd);
}

int com_open(char* fname) {
  struct termios t;
  memset(&t, 0, sizeof(t));
  t.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  t.c_iflag = IGNPAR | IGNCR;
  t.c_lflag = ICANON;
  int fd = e2(open(fname, O_RDONLY | O_NOCTTY), fname);
  e2(tcsetattr(fd, TCSANOW, &t), "tcsetattr() error");
  e2(tcflush(fd, TCIFLUSH), "tcflash() error");
  return fd;
}

int main(int argc, char* argv[]) {
  char* param[PARAM_COUNT];
  param_init(param, argc, argv);
  int fd = com_open(param[1]);
  while(1) {
    char buf[BUF_SIZE];
    int count = e2(read(fd, buf, BUF_SIZE-1), "read() error");
    if(count <= 0) {
      printf(".");
      usleep(1000);
      continue;
    }
    p2(buf, count);
    char_typing(param[2], buf, count);
  }
  close(fd);
  return 0;
}
