#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
pid_t pid;
pid = fork();
if(pid<0){
printf("failed to fork\n");
return EXIT_FAILURE;
}

if (pid>0){
int cs;
printf("P: created child with PID: %d\n", pid);
printf("P: waiting for child to terminate\n");
waitpid(pid, &cs, 0);
printf("P: child returned: %d\n", WEXITSTATUS(cs));
return 0;
}
else{
printf("C: I'm the child (PID: %d)\n", getpid());
execlp("ls","ls","-l","-h",NULL);
return 5;


}
}
