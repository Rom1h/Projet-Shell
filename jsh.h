#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "jobs.h"

int nb_chiffres(int n);
char* chemin_tronque(char* path);
int nombreMots(char* s);
bool is_redirect(char* r);
int evaluate_ligne(char* ligne, char* previous_path, char* prompt_buffer,struct Job* jobs,unsigned* jobsSize,pid_t pgid,bool isPipe);
void evaluate_cmd(char * cmd, int nbr_mots, char* previous_path, char* prompt_buffer,struct Job* jobs,unsigned nbJobs,bool backround,pid_t pgid,bool isPipe);
char* prompt();