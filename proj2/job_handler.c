#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "parse.h"
#include "job_handler.h"

#define MAX_JOBS 100
static volatile Job jobList[MAX_JOBS];

void init_jobs(){ //initalize job list
	unsigned int i;
    for (i=0; i<MAX_JOBS; i++) {
        jobList[i].name = NULL;
        jobList[i].pids = NULL;
        jobList[i].npids = 0;
        jobList[i].status = STOPPED;
    }
}


int add_job(pid_t* pids, char* cmdline, Parse* P){ //add a job

    unsigned int i;

    for (i=0; i<MAX_JOBS; i++){ //if there is space for a new job
        if (jobList[i].name == NULL)
            break;
    }

    if (i == MAX_JOBS){ //if max jobs then return
        return 0;
    }

    jobList[i].name = strdup (cmdline);
    jobList[i].pids = pids;
    jobList[i].npids = P->ntasks;
    jobList[i].pgrp = pids[0];

    if(P->background){
        jobList[i].status = BG; //create job in foreground or background
    }else{
        jobList[i].status = FG;
    }

    if(jobList[i].status == BG){ //if background
        printf ("[%u] ", i);
        for (i=0; i<P->ntasks; i++){
            printf ("%i ", pids[i]);
        }

        printf ("\n");
    }

    return 1;
}

int get_job_num(pid_t pid){ //get job number
unsigned int i,j;
    for (j = 0; j < MAX_JOBS; j++)
        for (i = 0; i < jobList[j].npids; i++){
            if (jobList[j].pids[i] == pid){
                return j;
            }
        }

    return -1;
}


char* get_job_name(unsigned int jobNum){ //get job name

    char* name = strdup (jobList[jobNum].name);
    return name;
}


pid_t get_job_proc_group(int jobNum){ //get process group of job

    return jobList[jobNum].pgrp;
}

void remove_pid(pid_t pid){ //remove pid from job

    int jobNum = get_job_num (pid);
unsigned int i;
    if (jobNum < 0){ //if job doesn't exist
        return;
    }

    for ( i = 0; i < jobList[jobNum].npids; i++){
        if (jobList[jobNum].pids[i] == pid){
            jobList[jobNum].pids[i] = 0;
        }
    }
}

int job_finished(int jobNum){ //check if job is finished
unsigned int i;
    for (i = 0; i < jobList[jobNum].npids; i++){
        if (jobList[jobNum].pids[i] != 0){
            return 0;
        }
    }

    return 1;
}


void del_job(int jobNum){ //delete a job

    jobList[jobNum].npids = 0;
    free (jobList[jobNum].pids);
    free (jobList[jobNum].name);
    jobList[jobNum].name = NULL;
}


int check_job(int jobNum){ //check if job exists

    if (!jobList[jobNum].name){
        return 0;
    }

    return 1;
}


void kill_job(int jobNum, int sig){ //kills a job

    if (!check_job(jobNum)){
        return;
    }

    kill (-1*jobList[jobNum].pgrp, sig);
}


void set_job_status(int jobNum, JobStatus status){ //set a job's status

    if (!check_job (jobNum)){
        return;
    }

    jobList[jobNum].status = status;
}


JobStatus get_job_status(int jobNum){ //checks a job's status

    if (!check_job(jobNum)){
        return TERM;
    }

    return jobList[jobNum].status;
}


void print_job (int jobNum){ //print a job

    if (!check_job (jobNum)){
        return;
    }

    printf ("[%u] + ", jobNum);
    if (jobList[jobNum].status == STOPPED){
        printf ("stopped\t");
    }else{
        printf ("running\t");
    }

    printf ("%s\n", jobList[jobNum].name);
}


void print_all(){ //print all
unsigned int i; 
   for ( i=0; i<MAX_JOBS; i++){
        if ((jobList[i].name != NULL) && strcmp (jobList[i].name, "jobs" )){
            print_job (i);
        }
    }
}
