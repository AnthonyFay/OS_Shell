#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <termcap.h>
#include <termios.h>
#include <curses.h>
#include <limits.h>
#include <pwd.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/select.h>

#include <minix/com.h>
#include <minix/config.h>
#include <minix/type.h>
#include <minix/endpoint.h>
#include <minix/const.h>
#include <minix/u64.h>
#include <minix/procfs.h>
#include <paths.h>

#include "yeeshell.h"

/* record cmdline history */
char *history[CMDLINE_HISTORY_MAX_QUANTITY];
int cmdline_amount = 0;

struct proc *proc = NULL, *prev_proc = NULL;
int order = ORDER_CPU;
int blockedverbose = 0;
int nr_total = 0;
int slot = 1;
unsigned int nr_procs, nr_tasks;

int main()
{
	char *cmdline = NULL, *pwd = NULL;
	char *args[ARGS_MAX_QUANTITY];
	int status = 1;
	pwd = (char *)calloc(PATH_MAX_SIZE, sizeof(char));
	for (int i = 0; i < CMDLINE_HISTORY_MAX_QUANTITY; i++)
	{
		history[i] = (char *)calloc(CMDLINE_MAX_SIZE, sizeof(char));
	}

	/* execute the shell's read, parse and execution loop */
	do
	{
		if (!getcwd(pwd, PATH_MAX_SIZE))
		{
			printf("yeeshell: The current path cannot be obtained!\n");
			exit(0);
		}
		printf("[root@yeeshell %s]# ", pwd);
		cmdline = readline();
		strcpy(history[cmdline_amount++], cmdline);
		status = execute(cmdline, args);
		free(cmdline);
	} while (status);
	for (int i = 0; i < CMDLINE_HISTORY_MAX_QUANTITY; i++)
	{
		free(history[i]);
	}
	exit(EXIT_SUCCESS);
}

char *readline()
{
	char *cmdline = NULL;
	ssize_t bufsize = 0;
	getline(&cmdline, &bufsize, stdin);
	return cmdline;
}

int parseline(char *cmdline, char **args)
{
	static char array[CMDLINE_MAX_SIZE]; /* holds local copy of command line */
	char *buf = array;					 /* ptr that traverses command line */
	char *delim;						 /* points to first space delimiter */
	int argc;							 /* number of args */
	int bg;								 /* background job? */

	strcpy(buf, cmdline);
	buf[strlen(buf) - 1] = ' ';	  /* replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* ignore leading spaces */
	{
		buf++;
	}

	/* Build the argv list */
	argc = 0;
	if (*buf == '\'')
	{
		buf++;
		delim = strchr(buf, '\'');
	}
	else
	{
		delim = strchr(buf, ' ');
	}

	while (delim)
	{
		args[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* ignore spaces */
		{
			buf++;
		}

		if (*buf == '\'')
		{
			buf++;
			delim = strchr(buf, '\'');
		}
		else
		{
			delim = strchr(buf, ' ');
		}
	}
	args[argc] = NULL;

	if (argc == 0) /* ignore blank line */
	{
		return 1;
	}

	/* should the job run in the background? */
	if ((bg = (*args[argc - 1] == '&')) != 0)
	{
		args[--argc] = NULL;
	}
	return bg;
}

int check_redirect(char **args, char *redirect_filename, char **redirect_args)
{
	int i = 0, j = 0, redirect_flag = REDIRECT_NO;
	while (args[i] != NULL)
	{
		if (!strcmp(args[i], ">"))
		{
			redirect_flag = REDIRECT_OUT;
			break;
		}
		else if (!strcmp(args[i], "<"))
		{
			redirect_flag = REDIRECT_IN;
			break;
		}
		i++;
	}

	if ((redirect_flag == 1) || (redirect_flag == 2))
	{
		strcpy(redirect_filename, args[i + 1]);
		for (j = 0; j < i; j++)
		{
			redirect_args[j] = args[j];
		}
	}

	return redirect_flag;
}

int do_redirect(int redirect_flag, char *redirect_filename, char **redirect_args)
{
	pid_t pid;
	int fd = 1;
	if ((pid = fork()) == 0) /* Child process */
	{
		if (redirect_flag == 1) /* out */
		{
			fd = open(redirect_filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR);
			close(1);
			dup(fd);
		}
		else if (redirect_flag == 2) /* in */
		{
			fd = open(redirect_filename, O_RDONLY, S_IRUSR);
			close(0);
			dup(fd);
		}
		if (execvp(redirect_args[0], redirect_args) <= 0)
		{
			printf("%s: Command not found\n", redirect_args[0]);
			exit(0);
		}
		close(fd);
	}
	else /* parent process */
	{
		waitpid(pid, NULL, 0);
	}
	return 1;
}

int check_pipe(char **args, char **pipe_arg_1, char **pipe_arg_2)
{
	int pipe_flag = 0, i = 0, j = 0;

	while (args[i] != NULL)
	{
		if (!strcmp(args[i], "|"))
		{
			pipe_flag = 1;
			break;
		}
		pipe_arg_1[j++] = args[i++];
	}
	pipe_arg_1[j] = NULL;

	j = 0;
	i++;
	while (args[i] != NULL)
	{
		pipe_arg_2[j++] = args[i++];
	}
	pipe_arg_2[j] = NULL;
	return pipe_flag;
}

int do_pipe(char **pipe_arg_1, char **pipe_arg_2)
{
	int fds[2];
	pipe(fds);
	pid_t prog_1, prog_2;

	if ((prog_1 = fork()) == 0) /* Child process 1 */
	{
		close(1);
		dup(fds[1]);
		close(fds[0]);
		close(fds[1]);

		if (execvp(pipe_arg_1[0], pipe_arg_1) <= 0)
		{
			printf("%s: Command not found\n", pipe_arg_1[0]);
			exit(0);
		}
	}

	if ((prog_2 = fork()) == 0) /* Child process 2 */
	{
		close(0);
		dup(fds[0]);
		close(fds[0]);
		close(fds[1]);

		if (execvp(pipe_arg_2[0], pipe_arg_2) <= 0)
		{
			printf("%s: Command not found\n", pipe_arg_2[0]);
			exit(0);
		}
	}

	close(fds[0]);
	close(fds[1]);

	waitpid(prog_1, NULL, 0);
	waitpid(prog_2, NULL, 0);
	return 1;
}

int do_bg_fg(char **args, int bg)
{
	pid_t pid;
	if ((pid = fork()) == 0)
	{
		if (execvp(args[0], args) <= 0)
		{
			printf("%s: Command not found\n", args[0]);
			exit(0);
		}
	}
	else
	{
		if (bg)
		{
			signal(SIGCHLD, SIG_IGN);
		}
		else
		{
			waitpid(pid, NULL, 0);
		}
	}
	return 1;
}

int execute(char *cmdline, char **args)
{
	int bg = 0, i = 0, redirect_flag = 0, pipe_num = 0;
	pid_t pid;
	char *redirect_filename = NULL;
	char *redirect_args[ARGS_MAX_QUANTITY];
	char *pipe_arg_1[ARGS_MAX_QUANTITY];
	char *pipe_arg_2[ARGS_MAX_QUANTITY];

	redirect_filename = (char *)calloc(REDIRECT_FILENAME_MAX_SIZE, sizeof(char));
	memset(redirect_args, NULL, sizeof(redirect_args));

	bg = parseline(cmdline, args);
	redirect_flag = check_redirect(args, redirect_filename, redirect_args);
	pipe_num = check_pipe(args, pipe_arg_1, pipe_arg_2);

	if (args[0] == NULL)
	{
		return 1;
	}
	if (pipe_num == 0) /* no pipe */
	{
		if (!built_in(args)) /* built-in cmd? */
		{
			if (redirect_flag != 0) /* redirection? */
			{
				return do_redirect(redirect_flag, redirect_filename, redirect_args);
			}
			else
			{
				return do_bg_fg(args, bg);
			}
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return do_pipe(pipe_arg_1, pipe_arg_2);
	}
}

int built_in(char **args)
{
	if (!strcmp(args[0], "exit"))
	{
		exit(0);
	}
	else if (!strcmp(args[0], "cd"))
	{
		return builtin_cd(args);
	}
	else if (!strcmp(args[0], "history"))
	{
		return builtin_history(args);
	}
	else if (!strcmp(args[0], "mytop"))
	{
		return builtin_mytop();
	}
	else
	{
		return 0;
	}
}

int builtin_cd(char **args)
{
	if (args[1] == NULL)
	{
		return 1;
	}
	else
	{
		if (chdir(args[1]) != 0)
		{
			perror("yeeshell");
		}
		return 1;
	}
}

int builtin_history(char **args)
{
	int n = 0;
	if (args[1] == NULL)
	{
		n = cmdline_amount;
	}
	else
	{
		n = atoi(args[1]) < cmdline_amount ? atoi(args[1]) : cmdline_amount;
	}
	printf("ID\tCommandline\n");
	for (int i = 0; i < n; i++)
	{
		printf("%d\t%s\n", i + 1, history[i]);
	}
	return 1;
}

int builtin_mytop()
{

	int cputimemode = 1;
	mytop_memory();
	getkinfo();
	get_procs();
	if (prev_proc == NULL)
		get_procs();
	print_procs(prev_proc, proc, cputimemode);

	return 1;
}

void mytop_memory()
{
	FILE *fp = NULL;
	int pagesize;
	long total = 0, free = 0, cached = 0;

	if ((fp = fopen("/proc/meminfo", "r")) == NULL)
	{
		exit(0);
	}

	fscanf(fp, "%u %lu %lu %lu", &pagesize, &total, &free, &cached);
	fclose(fp);

	printf("memory(KBytes):\t%ld total\t%ld free\t%ld cached\n", (pagesize * total) / 1024, (pagesize * free) / 1024, (pagesize * cached) / 1024);

	return;
}

void get_procs()
{
	struct proc *p;
	int i;

	p = prev_proc;
	prev_proc = proc;
	proc = p;

	if (proc == NULL)
	{
		proc = malloc(nr_total * sizeof(proc[0]));

		if (proc == NULL)
		{
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}
	}

	for (i = 0; i < nr_total; i++)
	{
		proc[i].p_flags = 0;
	}

	parse_dir();
}

void parse_dir()
{
	DIR *p_dir;
	struct dirent *p_ent;
	pid_t pid;
	char *end;

	if ((p_dir = opendir("/proc")) == NULL)
	{
		perror("opendir on " _PATH_PROC);
		exit(1);
	}

	for (p_ent = readdir(p_dir); p_ent != NULL; p_ent = readdir(p_dir))
	{
		pid = strtol(p_ent->d_name, &end, 10);

		if (!end[0] && pid != 0)
		{
			parse_file(pid);
		}
	}

	closedir(p_dir);
}

void parse_file(pid_t pid)
{
	char path[PATH_MAX], name[256], type, state;
	int version, endpt, effuid;
	unsigned long cycles_hi, cycles_lo;
	FILE *fp;
	struct proc *p;
	int i;

	sprintf(path, "/proc/%d/psinfo", pid);

	if ((fp = fopen(path, "r")) == NULL)
	{
		return;
	}

	if (fscanf(fp, "%d", &version) != 1)
	{
		fclose(fp);
		return;
	}

	if (version != PSINFO_VERSION)
	{
		fputs("procfs version mismatch!\n", stderr);
		exit(1);
	}

	if (fscanf(fp, " %c %d", &type, &endpt) != 2)
	{
		fclose(fp);
		return;
	}

	slot++;

	if (slot < 0 || slot >= nr_total)
	{
		fprintf(stderr, "top: unreasonable endpoint number %d\n", endpt);
		fclose(fp);
		return;
	}

	p = &proc[slot];

	if (type == TYPE_TASK)
	{
		p->p_flags |= IS_TASK;
	}
	else if (type == TYPE_SYSTEM)
	{
		p->p_flags |= IS_SYSTEM;
	}

	p->p_endpoint = endpt;
	p->p_pid = pid;

	if (fscanf(fp, " %255s %c %d %d %lu %*u %lu %lu",
			   name, &state, &p->p_blocked, &p->p_priority,
			   &p->p_user_time, &cycles_hi, &cycles_lo) != 7)
	{
		fclose(fp);
		return;
	}

	strncpy(p->p_name, name, sizeof(p->p_name) - 1);
	p->p_name[sizeof(p->p_name) - 1] = 0;

	if (state != STATE_RUN)
	{
		p->p_flags |= BLOCKED;
	}
	p->p_cpucycles[0] = make64(cycles_lo, cycles_hi);
	p->p_memory = 0L;

	if (!(p->p_flags & IS_TASK))
	{
		int j;
		if ((j = fscanf(fp, " %lu %*u %*u %*c %*d %*u %u %*u %d %*c %*d %*u",
						&p->p_memory, &effuid, &p->p_nice)) != 3)
		{

			fclose(fp);
			return;
		}

		p->p_effuid = effuid;
	}
	else
		p->p_effuid = 0;

	for (i = 1; i < CPUTIMENAMES; i++)
	{
		if (fscanf(fp, " %lu %lu",
				   &cycles_hi, &cycles_lo) == 2)
		{
			p->p_cpucycles[i] = make64(cycles_lo, cycles_hi);
		}
		else
		{
			p->p_cpucycles[i] = 0;
		}
	}

	if ((p->p_flags & IS_TASK))
	{
		if (fscanf(fp, " %lu", &p->p_memory) != 1)
		{
			p->p_memory = 0;
		}
	}

	p->p_flags |= USED;

	fclose(fp);
}

void getkinfo()
{
	FILE *fp;

	if ((fp = fopen("/proc/kinfo", "r")) == NULL)
	{
		fprintf(stderr, "opening " _PATH_PROC "kinfo failed\n");
		exit(1);
	}

	if (fscanf(fp, "%u %u", &nr_procs, &nr_tasks) != 2)
	{
		fprintf(stderr, "reading from " _PATH_PROC "kinfo failed\n");
		exit(1);
	}

	fclose(fp);

	nr_total = (int)(nr_procs + nr_tasks);
}

void print_procs(struct proc *proc1, struct proc *proc2, int cputimemode)
{
	int p, nprocs;
	u64_t systemticks = 0;
	u64_t userticks = 0;
	u64_t total_ticks = 0;
	int blockedseen = 0;
	static struct tp *tick_procs = NULL;

	if (tick_procs == NULL)
	{
		tick_procs = malloc(nr_total * sizeof(tick_procs[0]));

		if (tick_procs == NULL)
		{
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}
	}

	for (p = nprocs = 0; p < nr_total; p++)
	{
		u64_t uticks;
		if (!(proc2[p].p_flags & USED))
		{
			continue;
		}
		tick_procs[nprocs].p = proc2 + p;
		tick_procs[nprocs].ticks = cputicks(&proc1[p], &proc2[p], cputimemode);
		uticks = cputicks(&proc1[p], &proc2[p], 1);
		total_ticks = total_ticks + uticks;

		if (!(proc2[p].p_flags & IS_TASK))
		{
			if (proc2[p].p_flags & IS_SYSTEM)
			{
				systemticks = systemticks + tick_procs[nprocs].ticks;
			}
			else
			{
				userticks = userticks + tick_procs[nprocs].ticks;
			}
		}

		nprocs++;
	}

	if (total_ticks == 0)
	{
		return;
	}

	printf("CPU states: %6.2f%% user\t", 100.0 * userticks / total_ticks);
	printf("%6.2f%% system\t", 100.0 * systemticks / total_ticks);
	printf("%6.2f%% in total\n", 100.0 * (systemticks + userticks) / total_ticks);
}

u64_t cputicks(struct proc *p1, struct proc *p2, int timemode)
{
	int i;
	u64_t t = 0;
	for (i = 0; i < CPUTIMENAMES; i++)
	{
		if (!CPUTIME(timemode, i))
		{
			continue;
		}
		if (p1->p_endpoint == p2->p_endpoint)
		{
			t = t + p2->p_cpucycles[i] - p1->p_cpucycles[i];
		}
		else
		{
			t = t + p2->p_cpucycles[i];
		}
	}

	return t;
}