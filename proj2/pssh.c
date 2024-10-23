#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#include "builtin.h"
#include "parse.h"
#include "job_handler.h"

/*******************************************
 *  * Set to 1 to view the command line parse *
 *   *******************************************/
#define DEBUG_PARSE 0

#define READ_SIDE  0
#define WRITE_SIDE 1


static pid_t pssh_pgrp;


static void print_banner ()
{
    printf ("                    ________   \n");
    printf ("_________________________  /_  \n");
    printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf ("__  /_/ /(__  )_(__  )_  / / / \n");
    printf ("_  .___//____/ /____/ /_/ /_/  \n");
    printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}


/* returns a string for building the prompt
 *  *
 *   * Note:
 *    *   If you modify this function to return a string on the heap,
 *     *   be sure to free() it later when appropirate!  */
static char* build_prompt ()
{
    char* full;
    char* cwd = getcwd (NULL, 0);
    char prompt[] = "$ ";

    full = malloc (strlen (cwd) + strlen(prompt) + 1);
    sprintf (full, "%s%s", cwd, prompt);
    free (cwd);

    return full;
}


/* return true if command is found, either:
 *  *   - a valid fully qualified path was supplied to an existing file
 *   *   - the executable file was found in the system's PATH
 *    * false is returned otherwise */
static int command_found (const char* cmd)
{
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX];

    int ret = 0;

    if (access (cmd, X_OK) == 0)
        return 1;

    PATH = strdup (getenv("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        strncpy (probe, dir, PATH_MAX);
        strncat (probe, "/", PATH_MAX);
        strncat (probe, cmd, PATH_MAX);

        if (access (probe, X_OK) == 0) {
            ret = 1;
            break;
        }
    }

    free (PATH);
    return ret;
}

void set_fg_process_group (pid_t pgid){

    void (*old)(int);
    old = signal (SIGTTOU, SIG_IGN);
    tcsetpgrp (STDIN_FILENO, pgid);
    tcsetpgrp (STDOUT_FILENO, pgid);
    signal (SIGTTOU, old);
}

static void redirect(int fd_old, int fd_new){
    if (fd_new != fd_old){
        dup2 (fd_new, fd_old);
        close (fd_new);
    }
}


static int close_safe(int fd){
    if ((fd != STDIN_FILENO) && fd != (STDOUT_FILENO)){
        return close(fd);
    }

    return -1;
}

static void run(Task* T, int in, int out){
    redirect (STDIN_FILENO, in);
    redirect (STDOUT_FILENO, out);

    if (is_builtin (T->cmd)){
        builtin_execute (*T);
    }else if (command_found (T->cmd)){
        execvp (T->cmd, T->argv);
    }
}


static int get_infile(Parse* P){
    if (P->infile){
        return open (P->infile, 0);
    }else{
        return STDIN_FILENO;
    }
}

static int get_outfile(Parse* P){
    if (P->outfile){
        return creat(P->outfile, 0664);
    }else{
        return STDOUT_FILENO;
    }
}

static int is_possible(Parse* P){
    unsigned int t;
    Task* T;
    int fd;

    for (t = 0; t < P->ntasks; t++) {
        T = &P->tasks[t];
        if (!is_builtin (T->cmd) && !command_found (T->cmd)){
            printf ("pssh: command not found: %s\n", T->cmd);
            return 0;
        }
    }


    if (P->infile){
        if (access (P->infile, R_OK) != 0){
            printf ("pssh: no such file or directory: %s\n", P->infile);
            return 0;
        }
    }

    if (P->outfile){
        if ((fd = creat(P->outfile, 0664)) == -1){
            printf ("pssh: permission denied: %s\n", P->outfile);
            return 0;
        }
        close (fd);
    }

    return 1;
}


static int job_controller (Parse* P){
    if (!strcmp (P->tasks[0].cmd, "exit")) { //builtin exit
        exit (EXIT_SUCCESS);
    }else if (!strcmp (P->tasks[0].cmd, "fg")) { //send to foreground
        builtin_fg (P->tasks[0]);
        return 1;
    }else if (!strcmp (P->tasks[0].cmd, "bg")) { //send to background
        builtin_bg (P->tasks[0]);
        return 1;
    }else if (!strcmp (P->tasks[0].cmd, "jobs")) { //print jobs
        builtin_jobs (P->tasks[0]);
        return 1;
    }else if (!strcmp (P->tasks[0].cmd, "kill")) { //kill a function
        builtin_kill (P->tasks[0]);
        return 1;
    }

    return 0;
}

static void handler (int sig){ //beefy boy sig handy over here holding all them signals what will he do???
    pid_t child;
    char* name;
    int jstatus;

    switch (sig) {
    case SIGTTOU: //for background processes
        while (tcgetpgrp (STDIN_FILENO) != pssh_pgrp){
            pause ();
        }

        return;

    case SIGTTIN: //foreground processes
        while (tcgetpgrp (STDIN_FILENO) != pssh_pgrp){
            pause ();
        }

        break;

    case SIGCHLD: //if sigchild
        while ((child = waitpid (-1, &jstatus, WNOHANG | WUNTRACED | WCONTINUED)) > 0){ //while somethin is going on with the signal
            int jnum = get_job_num (child);

            if (WIFSTOPPED (jstatus)){ //if stopped
                if (get_job_status (jnum) != STOPPED){
                    set_job_status (jnum, STOPPED);
                    set_fg_process_group (pssh_pgrp);
                    name = get_job_name (jnum);
                    printf ("[%u] + suspended\t%s\n", jnum, name);
                    free (name);
                }

                continue;
            }else if (WIFCONTINUED (jstatus)){ //if continued 
                if (get_job_status (jnum) == STOPPED){
                    name = get_job_name (jnum);
                    printf ("[%u] + continued\t%s\n", jnum, name);
                    free (name);
                    set_job_status (jnum, BG);
                }

                continue;
            }else { //if none, remove pid
                remove_pid (child);
            }

            if (job_finished (jnum)){ //if job is finished then wrap it up
                name = get_job_name (jnum);

                if (get_job_status (jnum) != FG){
                    printf ("[%u] + done\t%s\n", jnum, name);
                }else if (get_job_status (jnum) == FG){
                    set_fg_process_group (pssh_pgrp);
                }

                del_job (jnum);
                free (name);
            }
        }
    }
}


/* Called upon receiving a successful parse.
 *  * This function is responsible for cycling through the
 *   * tasks, and forking, executing, etc as necessary to get
 *    * the job done! */
static void execute_tasks (Parse* P, char* job_name)
{
    unsigned int t;
    int fd[2];
    int in, out;
    pid_t* pid;


    if (!is_possible (P)){
        return;
    }

    if (job_controller (P)){
        return;
    }

    /* free()d in SIGCLD handler when job is done */
    pid = malloc (P->ntasks * sizeof(*pid));

    in = get_infile(P);

    for (t = 0; t < P->ntasks-1; t++){
        pipe (fd);
        pid[t] = fork();
        setpgid (pid[t], pid[0]);

        if (!pid[t]){
            close (fd[READ_SIDE]);
            run (&P->tasks[t], in, fd[WRITE_SIDE]);
        }

        close (fd[WRITE_SIDE]);
        close_safe (in);

        in = fd[READ_SIDE];
    }

    out = get_outfile (P);

    pid[t] = fork();
    setpgid(pid[t], pid[0]);

    if (!P->background){
        set_fg_process_group (pid[0]);
    }

    if (!pid[t]){
        run (&P->tasks[t], in, out);
    }

    close_safe (in);
    close_safe (out);
    add_job (pid, job_name, P);
}


int main (int argc, char** argv)
{
    char* prompt;
    char* cmdline;
    char* job_name;
    Parse* P;

    signal (SIGCHLD, handler);
    signal (SIGTTIN, handler);
    signal (SIGTTOU, handler);

    print_banner();
    init_jobs (); //initalize jobs when starting the shel
    pssh_pgrp = getpgrp();

    while (1) {
        prompt = build_prompt ();
        cmdline = readline (prompt);
        free (prompt);

        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit (EXIT_SUCCESS);

        job_name = strdup (cmdline);
        P = parse_cmdline (cmdline);
        
        if (!P)
            goto next;

        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
            goto next;
        }

#if DEBUG_PARSE
        parse_debug (P);
#endif

        execute_tasks (P, job_name);
        free (job_name);

    next:
        parse_destroy (&P);
        free(cmdline);
    }
}
