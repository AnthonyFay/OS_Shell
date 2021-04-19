#pragma once

#define CMDLINE_MAX_SIZE 1024 /* max length of a single command line */
#define ARGS_MAX_QUANTITY 128 /* max args on a command line */
#define BUFFER_MAX_SIZE 64    /* max size of a buffer which contains parsed arguments */
#define CMDLINE_DIV ' \t\r\n\a'
#define CMDLINE_HISTORY_MAX_QUANTITY 256
#define JOBS_MAX_QUANTITY 16
#define PATH_MAX_SIZE 256
#define LS_BUF_SIZE 1024
#define REDIRECT_FILENAME_MAX_SIZE 64 /* redirection filename */
#define REDIRECT_ARG_MAX_SIZE 16      /* redirection argument */

#define REDIRECT_NO 0  /* no redirection */
#define REDIRECT_OUT 1 /* redirect output */
#define REDIRECT_IN 2  /* redirect input */

#define _PATH_PROC "/proc/"
#define PSINFO_VERSION 0
#define TYPE_TASK 'T'
#define TYPE_SYSTEM 'S'
#define TYPE_USER 'U'
#define STATE_RUN 'R'
#define TIMECYCLEKEY 't'
#define ORDERKEY 'o'

#define USED 0x1
#define IS_TASK 0x2
#define IS_SYSTEM 0x4
#define BLOCKED 0x8

#define ORDER_CPU 0
#define ORDER_MEMORY 1
#define ORDER_HIGHEST ORDER_MEMORY

#define TC_BUFFER 1024 /* Size of termcap(3) buffer    */
#define TC_STRINGS 200 /* Enough room for cm,cl,so,se  */

/* name of cpu cycle types, in the order they appear in /psinfo. */
const char *cputimenames[] = {"user", "ipc", "kernelcall"};
#define CPUTIMENAMES (sizeof(cputimenames) / sizeof(cputimenames[0]))

#define CPUTIME(m, i) (m & (1L << (i)))

struct proc
{
    int p_flags;
    endpoint_t p_endpoint;
    pid_t p_pid;
    u64_t p_cpucycles[CPUTIMENAMES];
    int p_priority;
    endpoint_t p_blocked;
    time_t p_user_time;
    vir_bytes p_memory;
    uid_t p_effuid;
    int p_nice;
    char p_name[PROC_NAME_LEN + 1];
};

struct tp
{
    struct proc *p;
    u64_t ticks;
};

/* readline - Get the command line */
char *readline();

/* parseline - Evaluate the command line that the user has just typed in */
int parseline(char *cmdline, char **args);

/* check_redirect - check if the command contains redirection */
int check_redirect(char **args, char *redirect_filename, char **redirect_args);

/* do_redirect - execute redirection command */
int do_redirect(int redirect_flag, char *redirect_filename, char **redirect_args);

/* check_redirect - check if the command contains pipe */
int check_pipe(char **args, char **pipe_arg_1, char **pipe_arg_2);

/* do_pipe - execute pipe command */
int do_pipe(char **pipe_arg_1, char **pipe_arg_2);

/* do_bgfg - fork and execute background/foreground tasks */
int do_bg_fg(char **args, int bg);

/* execute - Execute the command line */
int execute(char *cmdline, char **args);

/* builtin functions - Handle built-in command */
int built_in(char **args);
int builtin_cd(char **args);
int builtin_history(char **args);
int builtin_mytop();

/* mytop routine */
void mytop_memory();
void mytop_CPU();
void get_procs();
void parse_dir();
void parse_file(pid_t pid);
void getkinfo();
void print_procs(struct proc *proc1, struct proc *proc2, int cputimemode);
u64_t cputicks(struct proc *p1, struct proc *p2, int timemode);