#ifndef _builtin_h_
#define _builtin_h_

#include "parse.h"

int is_builtin (char* cmd);
void builtin_execute (Task T);
int builtin_which (Task T);
void builtin_fg(Task T);
void builtin_bg(Task T);
void builtin_kill(Task T);
void builtin_jobs(Task T);

#endif /* _builtin_h_ */

