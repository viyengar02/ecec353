#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "parse.h"
#include "job_handler.h"

#define MAX_JOBS 100

static volatile Job jobList[MAX_JOBS];

void init_jobs(){ //initalize job objects
unsigned int i;
    for (i = 0; i < MAX_JOBS; i++){
        jobList[i].name = NULL;
        jobList[i].pids = NULL;
        jobList[i].npids = 0;
        jobList[i].status = STOPPED;
    }
}

int add_job(pid_t* pids, char* cmdline, Parse* P){ //if full, return 0, otherwise add job and return 1
    unsigned int i;

    for (i=0; i<MAX_JOBS; i++) {
        if (jobList[i].name == NULL)
            break;
    }

    if (i == MAX_JOBS)
        return 0;

    jobList[i].name = strdup (cmdline);
    jobList[i].pids = pids;
    jobList[i].npids = P->ntasks;
    jobList[i].pgrp = pids[0];

    if (P->background)
        jobList[i].status = BG;
    else
        jobList[i].status = FG;

    if (jobList[i].status == BG) {
        printf ("[%u] ", i);
        for (i=0; i<P->ntasks; i++)
            printf ("%i ", pids[i]);

        printf ("\n");
        fflush (stdout);
    }

    return 1;
}

int check_job(unsigned int num){ //checks if job exists

    if(!jobList[num].name){
        return 0;
    }

    return 1;
}


int get_job_num(pid_t pid){ //return job num from pid
unsigned int i,j;
    for(i = 0; i < MAX_JOBS; i++){
        for(j = 0; j < jobList[j].npids; j++){
            if(jobList[i].pids[j] == pid){
                return i;
            }
        }
    }

    return -1;
}

char* get_job_name(unsigned int num){ //return job name from from num

    return strdup(jobList[num].name);
}

pid_t get_job_group(unsigned int num){ //get process group job is in  

    return jobList[num].pgrp;
}

Jobstatus get_job_status(unsigned int num){

    if(check_job(num)){
        jobList[num].status;
    }

    return TERM;
}

void set_job_status(unsigned int num, Jobstatus status){ //set status of job

    if(check_job(num)){
        jobList[num].status = status;
    }

    return;
}

int job_finished (unsigned int num){ //checks if job is finished
unsigned int i;
    for (i = 0; i < jobList[num].npids; i++){
        if (jobList[num].pids[i] != 0){
            return 0;
        }
    }

    return 1;
}

void kill_job(unsigned int num, int signal){ //kills job 

    if(check_job(num)){
        kill((jobList[num].pgrp)*(-1), signal);
    }

    return;
}

void del_job(unsigned int num){ //frees all data for that job
    jobList[num].npids = 0;
    free (jobList[num].pids);
    free (jobList[num].name);
    jobList[num].name = NULL;

    return;
}

void remove_pid(pid_t pid){ //remove pid from job
unsigned int i;
    int num = get_job_num(pid);
    if (num < 0){
        return;
    }

    for (i = 0; i < jobList[num].npids; i++){
        if(jobList[num].pids[i] == pid){
            jobList[num].pids[i] = 0;
        }
    }

    return;
}

void print_job(unsigned int num){ //print a job
    
    if(check_job(num)){
        printf ("[%u] + ", num);
        if(jobList[num].status == STOPPED){
            printf("STOPPED\t");
        }else{
            printf("RUNNING\t");
        }

        printf("%s\n", jobList[num].name);
    }

    return;
}

void print_all_jobs(){ //print all jobs
 unsigned int i;   
    for (i = 0; i < MAX_JOBS; i++)
        if ((jobList[i].name != NULL) && strcmp (jobList[i].name, "jobs" ))
            print_job(i);
}

