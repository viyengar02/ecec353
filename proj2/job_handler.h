#include "parse.h"
#include <fcntl.h>
#include <unistd.h>

typedef enum {
    STOPPED,
    TERM,
    BG,
    FG,
} JobStatus;

typedef struct {
    char* name;
    unsigned int npids;
    pid_t* pids;
    pid_t pgrp;
    JobStatus status;
} Job;

void init_jobs();

int add_job (pid_t* pids, char* cmdline, Parse* P);
int check_job (int jnum);
void remove_pid (pid_t pid);
int job_finished (int jnum);
void del_job (int jnum);
void kill_job (int jnum, int sig);
