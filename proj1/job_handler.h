#include <unistd.h>
#include "parse.h"

typedef enum {
    STOPPED,
    TERM,
    BG,
    FG,
} Jobstatus;

typedef struct {
    char* name;
    Jobstatus status;
    pid_t* pids;
    unsigned int  npids;
    pid_t pgrp;
} Job;

void init_jobs();

int add_job(pid_t* pids, char* cmdline, Parse* P);
int check_job(unsigned int jobNum);
void set_job_status(unsigned int jobNum, Jobstatus status);
int job_finished(unsigned int jobNum);
void kill_job(unsigned int jobNum, int signal);
void del_job(unsigned int jobNum);
void remove_pid(pid_t pid);

int get_job_num(pid_t pid);
char* get_job_name(unsigned int jobNum);
pid_t get_job_group(unsigned int jobNum);
Jobstatus get_job_status(unsigned int jobNum);

void print_job(unsigned int jobNum);
void print_all_jobs();
