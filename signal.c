#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int main(int argc, char** argv){

	if(argc < 2){ //no arguments
        	fprintf(stderr, "Usage: %s [options] <pid>\n", argv[0]);
        	fprintf(stderr, "\n");
        	fprintf(stderr, "Options: \n");
        	fprintf(stderr, "   -s <signal>     Sends <signal> to <pid>\n");
        	fprintf(stderr, "   -l              Lists all signal numbers with their names\n");
        	exit(EXIT_FAILURE);
	}else{ //approriate number of arguments
		if(strcmp(argv[1], "-l") == 0){
            		printf(" 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL       5) SIGTRAP\n");
            		printf(" 6) SIGABRT      7) SIGBUS       8) SIGFPE       9) SIGKILL      10) SIGUSR1\n ");
            		printf("11) SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM     15) SIGTERM\n ");
            		printf("16) SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP     20) SIGTSTP\n");
            		printf(" 21) SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ\n");
            		printf(" 26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO       30) SIGPWR\n");
            		printf(" 31) SIGSYS\n");
        	}
 		pid_t pid = (int)atoi(argv[1]); //checks if nothing but the pid is given

        	if(pid != 0){ //pid provided
            		printf("SIGTERM sent to pid: %d\n", pid);
           		kill(pid, SIGTERM);
        	}

        	if(argc == 4){ //3 arguments provided, -s, pid, and signal
            		pid_t sig = (int)atoi(argv[2]);
            		pid_t pid = (int)atoi(argv[3]);

            		if(sig == 0){
                		kill(pid, sig);
                		if(errno == 0){
                    			printf("PID %d exists and is able to receive signals\n", pid);
                		}else if(errno == 1){
                    			printf("PID %d exists, but we can't send it signals\n", pid);
                		}else if(errno == 3){
                    			printf("PID %d does not exist\n", pid);
                		}
            }		else{
            		    printf("Sending signal: %d to pid: %d\n", sig, pid);
                       	    kill(pid, sig);
            }
        }
    }
}



