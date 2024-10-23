#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(void) {
  int fd1[2];
  int fd2[2];
  pid_t pid;
  char data;

  if (pipe(fd1) == -1) {
    perror("pipe 1");
    exit(EXIT_FAILURE);
  }
  if (pipe(fd2) == -1) {
    perror("pipe 2");
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }
if (pid == 0) {
close(fd1[1]);
    close(fd2[0]);
    while (read(fd1[0], &data, 1) > 0) {
      data = toupper(data);
      write(fd2[1], &data, 1);
    }
    close(fd1[0]);
    close(fd2[1]);
    exit(EXIT_SUCCESS);
  } else {
close(fd1[0]);
    close(fd2[1]);
    while (read(STDIN_FILENO, &data, 1) > 0) {
      write(fd1[1], &data, 1);
      read(fd2[0], &data, 1);
      printf("%c", data);
    }
    close(fd1[1]);
    close(fd2[0]);
    wait(NULL);
    exit(EXIT_SUCCESS);
  }
}





