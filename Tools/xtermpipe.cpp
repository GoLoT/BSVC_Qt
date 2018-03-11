// This simple program can be used with the Motorola M68681 DUART
// to pipe input and output to an xterm.
#ifndef _WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

int WaitForIO(int pipe_id) {
  fd_set readfds;
  fd_set writefds;
  fd_set exceptfds;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);

  FD_SET(0, &readfds);
  FD_SET(pipe_id, &readfds);

  select(pipe_id + 1, &readfds, &writefds, &exceptfds, NULL);

  if (FD_ISSET(0, &readfds))
    return (0);

  return (1);
}

int main() {
  int read_id, write_id;

  if (system("stty -echo -echoe -echonl raw") < 0) {
    fprintf(stderr, "Can't set terminal mode. Failing.\n");
    exit(1);
  }

  read_id = 3;
  write_id = 4;

  for (;;) {
    if (WaitForIO(read_id)) {
      char c;

      if (read(read_id, &c, 1) != 1) {
        fprintf(stderr, "Read failed.\n");
        exit(1);
      }
      if (write(1, &c, 1) != 1) {
        fprintf(stderr, "Write failed.\n");
        exit(1);
      }
    } else {
      char c;

      if (read(0, &c, 1) != 1) {
        fprintf(stderr, "Read failed.\n");
        exit(1);
      }
      if (write(write_id, &c, 1) < 0) {
        fprintf(stderr, "Error on write.\n");
        exit(1);
      }
    }
  }
}

#else
#undef UNICODE
#include <conio.h>
#include <iostream>
#include <windows.h>
#include <sstream>

int main(int argc, char** argv) {
  void* hIn;
  void* hOut;
  char c;
  DWORD rw;
  //std::stringstream ss(argv[1]);
  LPTSTR var = (LPTSTR) malloc(4096*sizeof(TCHAR));
  if(var == NULL) {
    printf("Sin memoria al leer ENV VARs");
  }
  if(!GetEnvironmentVariable("HandleRd", var, 4096)) {
    printf("Error al leer handle de lectura");
  }
  std::stringstream ss(var);
  ss >> hIn;
  if(!GetEnvironmentVariable("HandleWr", var, 4096)) {
    printf("Error al leer handle de escritura");
  }
  //std::stringstream ss2(argv[2]);
  std::stringstream ss2(var);
  ss2 >> hOut;
  //fprintf(stderr, "Handles %s %s %p %p.\n", argv[1], argv[2], hIn, hOut);
  //fprintf(stderr, "Title %s\n", argv[3]);
  SetConsoleTitle(argv[1]);
  while(true)
    if(_kbhit()) {
      rw = 0;
      c = _getch();
      if (!WriteFile(hOut, &c, 1, &rw, NULL) || rw < 1) {
        fprintf(stderr, "Write failed ERR: %lu Handle de escritura: %p.\n", GetLastError(), hOut);
      }
    } else {
      rw = 0;
          PeekNamedPipe(hIn, NULL, 0, NULL, &rw, NULL);
      if(rw > 0 && ReadFile(hIn, &c, 1, &rw, NULL) && rw == 1) {
        std::cout << c;
            }
    }
}

#endif
