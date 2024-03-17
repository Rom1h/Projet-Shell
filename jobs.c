#include "jobs.h"
static unsigned lastJobsId=0;

struct Job createJob(pid_t pid,char* cmd,int status){
    struct Job newJob;
    newJob.pid=pid;
    strcpy(newJob.commande, cmd);
    newJob.status=status;
    newJob.jobsId=++lastJobsId;
    newJob.gotSig=false;
    struct Processus processus;
    strcpy(processus.commande,cmd);
    processus.pid=pid;
    newJob.processus[0]=processus;
    newJob.nbOfProc=1;
    newJob.isPipe=false;
    return newJob;
}
//methode pour créer un processus
struct Processus createProc(pid_t pid,char* cmd){
    struct Processus processus;
    strcpy(processus.commande,cmd);
    processus.pid=pid;
    return processus;
}

//methode pour ajouter un processus à un job
int addProcToJob(struct Job* job,struct Processus newProc){
    job->processus[job->nbOfProc]=newProc;
    ++job->nbOfProc;
    return 1;
}

//on verifie si tout les jobs sont finis 
int jobsEnd(const struct Job* jobs,int nbJobs){
    for(int i=0;i<nbJobs;i++){
        if(jobs[i].status!=-1)return 0;
    }
    return 1;
}

//pour mettre à jour le status d'un job;
void majStatusJobs(pid_t pid,struct Job* jobs,int status,int size){
    for(int i =0;i<size;++i){
        if(jobs[i].pid==pid&&jobs[i].status!=-2&&jobs[i].status!=3&&jobs[i].status!=4){
            jobs[i].status=status;
        }
    }
}

//affiche les jobs terminé
int printJobsDone(struct Job * jobs,unsigned* size,int* nbJobs){
    for(int i = 0 ;i < *size;++i){ 
        char* mot = malloc(sizeof(char)*12);
        bool b = true;
       
        switch (jobs[i].status)
        {
        case -2:
        case 4 :
            strcpy(mot,"Killed    ");
            break;
        case 3 :
            strcpy(mot,"Interrupt ");
            break;
        case 0 :
            strcpy(mot,"Done      ");
            break;
        default : 
            b = false;
            break;
        }
        if(!jobs[i].gotSig){
            if(b){
                --(*nbJobs);  
                jobs[i].status=-1;
                if(jobs[i].jobsId==lastJobsId)--lastJobsId;
                fprintf(stderr, "[%d] %d %s %s\n", jobs[i].jobsId, jobs[i].pid, mot, jobs[i].commande);
            }
        }
        else {jobs[i].gotSig=false;}
        free(mot);
    }
     if(jobsEnd(jobs,*size)){     
        lastJobsId=0;
        *size=0;
    }
    return 1;
}

//affiche un job sur la sortie standart ou erreur
int printJob(struct Job* job, int sortie, int* nb_jobs){
    char* mot = malloc(sizeof(char)*11);
        switch (job->status)
        {
            case 0:
                strcpy(mot,"Done      ");     
                job->status=-1;
                --*nb_jobs;
                break;
            case 1:      
                strcpy(mot,"Running   ");
                break;
            case 2:
                strcpy(mot,"Stopped   ");
            break;
            case -2 :
                strcpy(mot,"Killed    ");
                job->status=-1;
                break;
            case 3 : 
                strcpy(mot,"Interrupt ");
                break;
            case 4 : 
                strcpy(mot,"Terminated");
                job->status=-1;
                break;
            default:
                free(mot);
                return 1 ;
        }
        if(sortie==1){ fprintf(stdout,"[%d] %d %s %s\n", job->jobsId, job->pid, mot, job->commande); } 
        else if(sortie==2) fprintf(stderr, "[%d] %d %s %s\n", job->jobsId, job->pid, mot, job->commande);    
    free(mot);    
    return 0;
}

//affiche tout les jobs du tableau de jobs
int printJobs(struct Job * jobs ,unsigned* jobsSize,int* nbJobs,bool printArborescence){ 
    for(int i=0;i<*jobsSize;++i){
        if(printArborescence){
            if(printJobArborescence(&jobs[i],nbJobs,jobs[i].commande)==1) return 1;
        }
        else{
            if(printJob(&jobs[i],1, nbJobs)==1) return 1;
        }
    }
    //si tout les jobs sont finis alors on free les commandes de tout les du tableau puis on reinitialise les pointeur 
    if(jobsEnd(jobs,*jobsSize)){      
        lastJobsId=0;
        *jobsSize=0;
    }
    return 0;
}

int printJobArborescence(struct Job* job,int* nb_jobs,char* cmd){
    char* mot = malloc(sizeof(char)*11);
        switch (job->status)
        {
            case 0:
                strcpy(mot,"Done      ");     
                job->status=-1;
                --*nb_jobs;
                break;
            case 1:      
                strcpy(mot,"Running   ");
                break;
            case 2:
                strcpy(mot,"Stopped   ");
            break;
            case -2 :
                strcpy(mot,"Killed    ");
                job->status=-1;
                break;
            case 3 : 
                strcpy(mot,"Interrupt ");
                break;
            case 4 : 
                strcpy(mot,"Terminated");
                job->status=-1;
                break;
            default:
                free(mot);
                return 1 ;
        }
    printf("[%d] %d %s %s\n", job->jobsId, job->pid, mot, job->commande);
    if(printProcessusFils(job->pid,job->commande,mot,false)==1){free(mot); return 1;;}
    for(int i =1;i<(job->nbOfProc)-1;i++){//affiche tout les processus du job
        struct Processus proc=job->processus[i];
        printf("     |%d %s %s\n",proc.pid,mot,proc.commande);
        if(printProcessusFils(proc.pid,proc.commande,mot,true)==1){free(mot);return 1;}
    }
    if(job->nbOfProc>1){//affiche le dernier processus
        struct Processus proc=job->processus[job->nbOfProc-1];
        printf("     %d %s %s\n",proc.pid,mot,proc.commande);
        if(printProcessusFils(proc.pid,proc.commande,mot,false)==1){free(mot);return 1;}
    }
    free(mot); 
    return 0;
}

//fonction pour afficher tout les processus fils 
int printProcessusFils(pid_t ppid ,char* cmd, char* status,bool isPipe){
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int fd;
    char ref[500],path[500], buffer[1024];
    int nb;
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("Erreur lors de l'ouverture de /proc");
        return 1;
    }
    //on parcourt tout le repertoire proc puis on regarde la ligne ppid du fichier status si c'est egale à ppid alors on l'affiche
    while ((entry = readdir(dir)) != NULL) {
        sprintf(ref, "/proc/%s",entry->d_name);  
        if (stat(ref, &st) == -1)perror("stat");
        if (S_ISDIR(st.st_mode)) {
            sprintf(path, "/proc/%s/status", entry->d_name);
            if((fd = open(path, O_RDONLY))==-1);
            else{
                nb= read(fd, buffer, 1024- 1);
                if (nb > 0) {
                    buffer[nb] = '\0';
                    char *linePPid = strstr(buffer, "PPid:");
                    char *linePid = strstr(buffer, "Pid:");
                    if (linePPid&&linePid) {
                        pid_t ppidProc = atoi(linePPid + 5);
                        pid_t pidProc=atoi(linePid+4);
                        if (ppidProc == ppid) {
                            if(isPipe)printf("     |%d %s %s\n", pidProc,status,cmd);
                            else printf("     %d %s %s\n", pidProc,status,cmd);
                            
                        }
                    }
                }
            }
            close(fd);
        }
    }
    closedir(dir);
    return 0;
}

int printJobID(struct Job* jobsList,unsigned* jobsListSize,int* nbJobs,int jobId,bool printArborescence){
   for(int i=0;i<*jobsListSize;++i){
        if(jobsList[i].jobsId==jobId){
            if(printArborescence){
                if(printJobArborescence(&jobsList[i],nbJobs,jobsList[i].commande)==1) return 1;
            }
            else{
                if(printJob(&jobsList[i],1,nbJobs)==1) return 1;
            }
            break;
        }
    }
    //si tout les jobs sont finis alors on free les commandes de tout les du tableau puis on reinitialise les pointeur 
    if(jobsEnd(jobsList,*jobsListSize)){      
        lastJobsId=0;
        *jobsListSize=0;
    }
    return 0;
}