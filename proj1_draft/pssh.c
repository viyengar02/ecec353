#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "builtin.h"
#include "parse.h"

/*******************************************
 *  * Set to 1 to view the command line parse *
 *   *******************************************/
#define DEBUG_PARSE 0
char cwd[1024]; //current directory
#define READ_SIDE  0
#define WRITE_SIDE 1

void print_banner ()
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
 getcwd(cwd, sizeof(cwd));
    strcat(cwd, " $ ");
    return  cwd;

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

        strncpy (probe, dir, PATH_MAX-1);
        strncat (probe, "/", PATH_MAX-1);
        strncat (probe, cmd, PATH_MAX-1);

        if (access (probe, X_OK) == 0) {
            ret = 1;
            break;
        }
    }

    free (PATH);
    return ret;
}

static void redirect (int fd_old, int fd_new){
    if (fd_new != fd_old){
        dup2 (fd_new, fd_old);
        close (fd_new);
    }
}


static int closeFD (int fd){
    if ((fd != STDIN_FILENO) && fd != (STDOUT_FILENO)){
        return close (fd);
    }

    return -1;
}


/* exec the specified task (either builtin or on the filesystem)
 *  * with the specified input and output file descriptors */
static void run (Task* T, int in, int out)
{
    redirect (STDIN_FILENO, in);
    redirect (STDOUT_FILENO, out);

    if (is_builtin (T->cmd))
        builtin_execute (*T);
    else if (command_found (T->cmd))
        execvp (T->cmd, T->argv);
}

/* Opens the infile from the Parse, if it exists, and returns
 *  * the associated file descriptor.  Otherwise, returns the
 *   * default stdin (0) file descriptor */
static int get_infile (Parse* P)
{
    if (P->infile)
        return open (P->infile, 0);
    else
        return STDIN_FILENO;
}


/* Opens the outfile from the Parse, if it exists, or creates
 *  * it with -rw-rw-r-- permissions if it does not. Returns
 *   * the associated file descriptor.  Otherwise, returns the
 *    * default stdout (1) file descriptor */
static int get_outfile (Parse* P)
{
    if (P->outfile)
        return creat (P->outfile, 0664);
    else
        return STDOUT_FILENO;
}

/* Checks to make sure that all of the commands specified
 *  * in the Parse actually exist and can be executed.  Also
 *   * checks to see if specified infile can actually be read and
 *    * if the outfile can be created/written.
 *     *
 *      * Also checks for the exit command, which requires special
 *       * handling since it must be executed prior to a fork().
 *        *
 *         * Returns 1 if the Parse is runnable
 *          *         0 otherwise & prints error to stderr */
static int is_possible (Parse* P)
{
    unsigned int t;
    Task* T;
    int fd;

    for (t=0; t<P->ntasks; t++) {
        T = &P->tasks[t];
        if (!is_builtin (T->cmd) && !command_found (T->cmd)) {
            printf ("pssh: command not found: %s\n", T->cmd);
            fflush (stdout);
            return 0;
        }
    }


    if (P->infile) {
        if (access (P->infile, R_OK) != 0) {
            printf ("pssh: no such file or directory: %s\n", P->infile);
            fflush (stdout);
            return 0;
        }
    }

    if (P->outfile) {
        if ((fd = creat (P->outfile, 0664)) == -1) {
            printf ("pssh: permission denied: %s\n", P->outfile);
            fflush (stdout);
            return 0;
        }
        close (fd);
    }

    return 1;
}



static void execute_tasks (Parse* P)
{
    unsigned int t;
    int fd[2];
    int in, out;
    pid_t* pid;


    if (!is_possible (P))
        return;


    /* free()d in SIGCLD handler when job is done */
    pid = malloc (P->ntasks * sizeof(*pid));

    in = get_infile (P);

    for (t=0; t<P->ntasks-1; t++) {
        pipe (fd);
        pid[t] = fork ();
        setpgid (pid[t], pid[0]);  /* both prnt & chld do this */

        if (!pid[t]) {
            close (fd[READ_SIDE]);
            run (&P->tasks[t], in, fd[WRITE_SIDE]);
        }

        close (fd[WRITE_SIDE]);
        closeFD (in);

        in = fd[READ_SIDE];
    }

    out = get_outfile (P);

    pid[t] = fork ();
    setpgid (pid[t], pid[0]);  /* both prnt & chld do this */


    if (!pid[t])
        run (&P->tasks[t], in, out);

    closeFD (in);
    closeFD (out);

}


int main (int argc, char** argv)
{
    char* cmdline;
    Parse* P;

    print_banner ();

    while (1) {
        cmdline = readline (build_prompt());
        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit (EXIT_SUCCESS);

        P = parse_cmdline (cmdline);
        if(strcmp(P, "exit\n")==0){
		 exit(EXIT_SUCCESS);
	}
	if (!P)
            goto next;

        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
            goto next;
        }

#if DEBUG_PARSE
        parse_debug (P);
#endif

        execute_tasks (P);

    next:
        parse_destroy (&P);
        free(cmdline);
    }
}

