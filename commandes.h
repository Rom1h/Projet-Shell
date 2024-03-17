#include "jsh.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int mon_cd(const char* path);
char* construit_chemin();
int pwd();
int maj_statut_sig(int sig, int statut_actuel);
int kill_pid(int sig, pid_t pid);
int mon_kill(int sig, int pid, struct Job* jobs, int nb_jobs,int* jobsCur);
int fg(int jobsId,struct Job* jobsList,int jobsListSize,int* nb_jobs);
int bg(int jobsId,struct Job* jobsList,int jobsListSize);
int mon_exit(int val);