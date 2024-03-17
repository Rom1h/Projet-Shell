#ifndef JOBS_J
#define JOBS_J
#define MAX_JOBS 100
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>



struct Processus {
    pid_t pid;
    char commande[256];

};
struct Job {
    pid_t pid; 
    char commande[256];
    int status; // État du job (running/1, done/0, stopped/2, supprimé/-1, killed/-2, interrupt/3, terminated/4)
    int jobsId;
    bool background;
    bool gotSig;
    struct Processus processus[100];
    int nbOfProc;
    bool isPipe;
};

int addProcToJob(struct Job* job,struct Processus newProc);
struct Processus createProc(pid_t pid,char* cmd);
int printJobsDone(struct Job * jobs,unsigned* size,int* nbJobs);
struct Job createJob(pid_t pid,char* cmd,int status);
int printJob(struct Job* job, int sortie, int* nb_jobs);
int printJobID(struct Job* jobsList,unsigned* jobsListSize,int* nbJobs,int jobId,bool printArborescence);
int printJobs(struct Job * jobs ,unsigned *jobsSize,int* nbJobs,bool printArborescence);
int printJobArborescence(struct Job* job,int* nb_jobs,char* cmd);
int printProcessusFils(pid_t ppid ,char* cmd, char* status,bool isPipe);
void majStatusJobs(pid_t pid,struct Job* jobs,int status,int size);
int jobsEnd(const struct Job* jobs,int nbJobs);
#endif