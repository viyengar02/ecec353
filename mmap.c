#include <sys/mman.h> /* mmap(), munmap(), PROT_*, MAP_* */
#include <sys/wait.h> /* wait() */
#include <unistd.h> /* fork() */
#include <stdlib.h> /* exit(), EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <fcntl.h>


int main()
{
 int fd;
 fd = open("/dev/zero", O_RDWR);
 if(fd==-1){
 	fprintf(stderr, "failed to open file\n");
	exit(EXIT_FAILURE);
 }
 int *addr;
 addr = mmap(NULL, sizeof(*addr), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
 if(addr==MAP_FAILED){
 	fprintf(stderr, "failed to map memory\n");
        exit(EXIT_FAILURE);	
 }
 if(close(fd)==-1){
 	fprintf(stderr, "failed to close file\n");
        exit(EXIT_FAILURE);
 }
 
 *addr = 1;


 switch(fork()) {
	 case -1: 
		fprintf(stderr, "failed to fork \n");
		exit(EXIT_FAILURE);
	 case 0:
		printf("Child: value = %d\n", *addr);
 		(*addr)++;
 		if(munmap(addr, sizeof(*addr))==-1){
			fprintf(stderr, "failed to unmap\n");
        		exit(EXIT_FAILURE);
		}
	
 		exit(EXIT_SUCCESS);
 	default:
 		wait(NULL);
 		printf("Parent: value = %d\n", *addr);
 		munmap(addr, sizeof(*addr));
 		exit(EXIT_SUCCESS);
 	}
}
