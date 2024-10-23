#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "builtin.h"
#include "parse.h"
#include "job_handler.h"

static char* builtin[] = {
    "exit",   /* exits the shell */
    "which",  /* displays full path to command */
    "fg", /* moves job to foreground */
    "bg", /* moves job to background */
    "kill", /* kills job */
    "jobs", /* displays all active jobs */
    NULL
};

void set_fg_process_group (pid_t pgrp);


int is_builtin (char* cmd)
{
    int i;

    for (i=0; builtin[i]; i++) {
        if (!strcmp (cmd, builtin[i]))
            return 1;
    }

    return 0;
}

int builtin_which(Task T){
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX];

    int ret = EXIT_FAILURE;

    if (!T.argv || !T.argv[1])
        exit (EXIT_FAILURE);

    if (access (T.argv[1], F_OK) == 0) {
        printf ("%s\n", T.argv[1]);
        exit (EXIT_SUCCESS);
    }

    if (is_builtin (T.argv[1])) {
        printf ("%s: shell built-in command\n", T.argv[1]);
        exit (EXIT_SUCCESS);
    }

    PATH = strdup (getenv ("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        strncpy (probe, dir, PATH_MAX);
        strncat (probe, "/", PATH_MAX);
        strncat (probe, T.argv[1], PATH_MAX);

        if (access (probe, X_OK) == 0) {
            printf ("%s\n", probe);
            ret = EXIT_SUCCESS;
            break;
        }
    }

    free (PATH);
    exit (ret);
}

void builtin_fg(Task T){

    int argc = 0;
    while(T.argv[argc]){
        argc++;
    }

    if(argc != 2){ //not exactly 2 args
        printf("Usage: fg %%<job number>\n");
        return;
    }

    int jobNum = atoi(T.argv[1]+1);
    if(check_job(jobNum)){
        set_job_status(jobNum, FG);
        set_fg_process_group(get_job_group(jobNum));
        kill_job(jobNum, SIGCONT);
    }else{
        printf("pssh: invalid job number: %s\n", T.argv[1]+1);
    }
}

void builtin_bg(Task T){
    int argc;

    while(T.argv[argc]){
        argc++;
    }

    if(argc != 2){
        printf("Usage: bg %%<job number>\n");
        return;
    }

    int jobNum = atoi(T.argv[1]+1);

    if(check_job(jobNum)){
        kill_job(jobNum, SIGCONT);
    }else{
        printf("pssh: invalid job number: %d\n", jobNum);
    }


}

void builtin_jobs (Task T){
    print_all_jobs();
}

void builtin_kill(Task T){
    int argc;
    int i = 1;
    int sig = SIGINT;

    while(T.argv[argc]){
        argc++;
    }

    if(argc < 2){
        printf ("Usage: kill [-s <signal>] <pid> ...\n");
        return;
    }

    if(!strcmp(T.argv[1], "-s")){
        sig = atoi(T.argv[2]);
        i = 3;
    }

    for(; i < argc; i++){
        if(*T.argv[i] == '%'){
            if(check_job(atoi(T.argv[i]+1))){
                kill_job(atoi(T.argv[i]+1), sig);
            }else{
                printf("pssh: invalid job number: %s\n", T.argv[i]+1);
            }
        }else{
            int ret = kill(atoi(T.argv[i]), sig);

            if(ret < 0){
                printf("pssh: inavlid pid: %s\n", T.argv[i]);
            }
        }
    }
}


void builtin_execute (Task T)
{
    if(!strcmp (T.cmd, "which")){
        builtin_which(T);
    }
if (!strcmp (T.cmd, "exit")) {
        exit (EXIT_SUCCESS);
    }
else {
        printf ("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
