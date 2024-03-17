#include "commandes.h"

// se déplacer dans l'arborescence
int mon_cd(const char* path){
    // Cas "cd ~" : (cd sans argument)
    if(!strcmp(path,"~")){
        const char* home = getenv("HOME");
        if(home==NULL){
            fprintf(stderr, "Impossible de récupérer le répertoire personnel.\n");
            return 1;    
        }
        if(chdir(home)!=0){
            perror("Une erreur est survenue lors de l'opération.");
            return 1;
        }
        return 0;
    }

    // Cas "cd chem" : (cd avec un argument)
    if(chdir(path)==-1){
        if(errno == ENOENT){
            fprintf(stderr, "%s : Il n'existe pas de tel fichier ou dossier.\n", path);
        }
        else if(errno == ENOTDIR){
            fprintf(stderr, "%s : N'est pas un dossier.\n", path);
        }
        else if(errno == EACCES){
            fprintf(stderr, "%s : Vous n'avez pas les permissions nécessaires.\n", path);
        }
        else if(errno == EBADF){
            fprintf(stderr, "%s : N'est pas un descripteur de fichiers valide.\n", path);
        }
        else{
            fprintf(stderr, "%s : Une erreur est survenue lors de l'opération.\n", path);
        }
        return 1;
    }
    return 0;
}


//affiche le chemin absolu du répertoire courant
int pwd(){
  char * pwd=calloc(PATH_MAX+1,sizeof(char));
  // récupérer le chemin
  if(getcwd(pwd,PATH_MAX+1)==NULL){
    free(pwd);
    return 1;
  }
  // écrire le chemin sur la sortie standard
  write(1,pwd,strlen(pwd));
  write(1,"\n",1);
  free(pwd);
  return 0;  

}

//envoie le signale de commande kill au groupe de processus pid
int kill_pid(int sig, pid_t pid){
    if(killpg(pid, sig)==-1){
        switch(errno){
            case EINVAL :
                fprintf(stderr,"sig %d pid %d\n",sig,pid);
                perror("ERREUR kill : Le signal donné n'existe pas");
                return 1;
            case EPERM : 
                perror("ERREUR kill : Opération non permise");
                return 1;
            case ESRCH :
                perror("Erreur kill : Le pid donné n'existe pas");
                return 1;
            default : 
                perror("kill : Une erreur s'est produite");
                return 1;
        }
    }
    return 0;
}

//renvoie le statut en fonction du signal reçu
int maj_statut_sig(int sig, int statut_actuel){
    switch(sig){
        case SIGKILL :
            return -2; 
        case SIGTERM : 
            return 4;
        case SIGSTOP :
        case SIGTSTP :
            return 2;
        case SIGCONT :
            if(statut_actuel==2) return 1; 
            else return statut_actuel;
        case SIGINT :
        case SIGQUIT :
            return 3;
        case SIGUSR1 :
            return 4;
        default : 
            return statut_actuel;
    }
}

int mon_kill(int sig, int pid, struct Job* jobs, int nb_jobs,int* jobsCur){
    for(size_t i = 0; i<nb_jobs; ++i){
        if(jobs[i].pid==pid){
            if(!jobs[i].isPipe){
                pid_t pgid = getpgid(jobs[i].pid);//recupere le groupe du processus pid
                int statut = maj_statut_sig(sig,jobs[i].status);
                majStatusJobs(jobs[i].pid,jobs,statut,nb_jobs);//met à jour le statut du job
                jobs[i].gotSig=true;
                return kill_pid(sig, pgid);
            }
            else{
                int statut = maj_statut_sig(sig,jobs[i].status);
                majStatusJobs(jobs[i].pid,jobs,statut,nb_jobs);//met à jour le statut du job
                jobs[i].gotSig=true;
                return kill_pid(sig, jobs[i].pid);
            }
        }
    }
    return 1;
}

int bg(int jobsId,struct Job* jobsList,int jobsListSize){
    for(int i=0;i<jobsListSize;++i){
        struct Job job=jobsList[i];
        if(jobsId==job.jobsId){
            jobsList[i].background=true;
            return mon_kill(SIGCONT,job.pid,jobsList,jobsListSize,0);
        }
    }
    fprintf(stderr,"Le job n'existe pas.");
    return 1;
}

int fg(int jobsId,struct Job* jobsList,int jobsListSize,int* nb_jobs){
    for(int i=0;i<jobsListSize;++i){
        struct Job job=jobsList[i];
        if(jobsId==job.jobsId){
            int status;
            jobsList[i].background=false;
            mon_kill(SIGCONT,job.pid,jobsList,jobsListSize,0);
            //cas ou le jobs c'est un pipe on attends ainsi tout les procesus du pipe
            if(jobsList[i].isPipe){     
                tcsetpgrp(STDIN_FILENO,job.pid);
                while ((waitpid(-job.pid, &status, WUNTRACED)) > 0) {
                    if(WIFSTOPPED(status)){  
                        majStatusJobs(job.pid, jobsList, 2, jobsListSize);//si le processus à reçu le signal SIGSTP ou SIGSTOP on met le statut du jobs à 2(stopped)
                        printJob(&job, 2, nb_jobs);//on affiche que le procesus à reçu  le signal stopped
                        break;
                    }

                }
               if(WIFEXITED(status)||WIFSIGNALED(status)){//on récupère la valeur de retour du processus fils
                        //si le processus a été tué
                        int sig=WTERMSIG(status);
                        if(sig==15){
                            fprintf(stderr,"Terminated\n");
                        }
                        majStatusJobs(job.pid,jobsList,-1,jobsListSize);//si le processus a terminé normalement on l'enleve de la liste des jobs
                        --(*nb_jobs);
                    }
                    
                tcsetpgrp(STDIN_FILENO, getpgrp());
            }

            else {
                tcsetpgrp(STDIN_FILENO,job.pid);
                waitpid(job.pid, &status,WUNTRACED);
                tcsetpgrp(STDIN_FILENO, getpgrp());
                if(WIFEXITED(status)){//on récupère la valeur de retour du processus fils
                    majStatusJobs(job.pid,jobsList,-1,jobsListSize);//si le processus a terminé normalement on l'enleve de la liste des jobs
                    --(*nb_jobs);
                }
                else if(WIFSTOPPED(status)){
                    majStatusJobs(job.pid, jobsList, 2, jobsListSize);//si le processus à reçu le signal SIGSTP ou SIGSTOP on met le statut du jobs à 2(stopped)
                    printJob(&job, 2, nb_jobs);//on affiche que le procesus à reçu  le signal stopped
                }
                else if(WIFSIGNALED(status)){
                
                    majStatusJobs(job.pid, jobsList, -2, jobsListSize);//si le processus à reçu le signal SIGSTP ou SIGSTOP on met le statut du jobs à 2(stopped)
                    printJob(&job, 2, nb_jobs);//on affiche que le procesus à reçu  le signal stopped
                }
            }
            

        }
    }

    return 0;
}

// quitter le programme
int mon_exit(int val){
    write(1,"fermeture...\n", 13);
    exit(val);
}