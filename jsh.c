#include "jsh.h"
#include "commandes.h"
#include "redirections.h"
#define RED_TEXT "\033[1;31m"
#define GREEN_TEXT "\033[1;32m"
#define BLUE_TEXT "\033[34m"
#define PURPLE_TEXT "\033[0;35m"
#define CYAN_TEXT "\033[36m"
#define RESET_COLOR "\033[0m"

int nb_jobs = 0;
int last_return_value=0; // la dernière valeur retournée

struct Job* jobsList;
unsigned int jobsListSize = 0;

// pour la taille du prompt 
// en fonction du nbr de jobs
int nb_chiffres(int n){
    if(n==0){ return 1;}
    int res = 0;
    while(n>0){
        n=n/10;
        ++res;
    }
    return res;
}

// fonction pour tronquer le chemin 
// en fonction de la taille du reste
// des éléments du prompt 
char* chemin_tronque(char* path){
    /* "[nb de jobs]path$ " + couleurs */
    // 4 = "[]$ " 
    int taille_max_chemin = 30-4-nb_chiffres(nb_jobs);
    char* new_path = calloc(taille_max_chemin+1,sizeof(char));

    // Si le chemin dépasse la taille max, on le tronque
    if(strlen(path)>taille_max_chemin){
        int debut = strlen(path)-(taille_max_chemin-3);
        char* chem = malloc((taille_max_chemin-3)*(sizeof(char)+1));
        strncpy(chem, path+debut, taille_max_chemin+3);
        strcat(new_path, "...");
        strcat(new_path, chem);
        free(chem);
    }
    // Sinon, on le garde tel quel
    else{
        strcpy(new_path, path);
    }
    return new_path;
}

//fonction pour remettre les sa handler a default
void signal_default(){
    // ignorer les signaux SIGINT SIGTERM SIGTTIN SIGQUIT SIGTTOU SITSTP
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIG_DFL;
    for(int i = 0; i<NSIG; i++){
        sigaction(i, &action, NULL);
    }
}

//compte le nombre de mots d'une chaine de characteres
int nombreMots(char* s){
    int size = (strlen(s)+1);
    char* copy = malloc(size*sizeof(char));
    memmove(copy,s,size);
    char *rest;
    char* mot = strtok_r(copy, " \n", &rest);
    int nbr = 0;
    while(mot!=NULL){
        ++nbr;
        mot = strtok_r(NULL," \n", &rest);
    }
    free(copy);
    return nbr;
}

int nbr_occurrences(char* chaine, char* sous_chaine){
    int cmpt = 0;
    size_t scLength = strlen(sous_chaine);

    if(scLength==0){
        return 0;
    }

    const char* currentPosition = chaine;
    while((currentPosition=strstr(currentPosition, sous_chaine))!=NULL){
        cmpt++;
        currentPosition+=scLength;
    }
    return cmpt;
}

bool is_redirect(char* r){
    return (strstr(" < > >| >> 2> 2>| 2>> ",r)!=NULL
            && (strcmp(r,"2")!=0) && (strcmp(r,"|")!=0));
}

void procesusMaj(struct Job* jobs, unsigned jobsSize) {
    for (int i = 0; i < jobsSize; ++i) {
        if (jobs[i].status == 1 ||jobs[i].status == 2) { // Job en cours d'exécution
            int status;
            if(!jobs[i].isPipe){
                pid_t wpid = waitpid(jobs[i].pid, &status, WNOHANG | WCONTINUED | WUNTRACED);
                if (wpid == -1) {
                    perror("waitpid");
                } 
                else if (wpid != 0) { // Le processus fils a changé d'état
                    if (WIFCONTINUED(status)) {
                        majStatusJobs(jobs[i].pid, jobs, 1, jobsSize); // Mettre à jour le statut du job
                        printJob(&jobs[i], 2, &nb_jobs);
                    }
                    else if(WIFSTOPPED(status)){
                            majStatusJobs(jobs[i].pid, jobs, 2, jobsSize); // Mettre à jour le statut du job
                            printJob(&jobs[i], 2, &nb_jobs);//on affiche que le procesus à reçu  le signal stopped
                    }
                }
            }
            else{
                pid_t wpid;
                while((wpid=waitpid(-jobs[i].pid, &status, WNOHANG | WCONTINUED | WUNTRACED))>0){
                    if(WIFSTOPPED(status)){
                            majStatusJobs(jobs[i].pid, jobs, 2, jobsSize); // Mettre à jour le statut du job
                            break;
                    }
                }
                if (wpid == -1) {
                    perror("waitpidici ");
                } 
                else if (wpid != 0) { // Le processus fils a changé d'état
                    if (WIFCONTINUED(status)) {
                        majStatusJobs(jobs[i].pid, jobs, 1, jobsSize); // Mettre à jour le statut du job
                        printJob(&jobs[i], 2, &nb_jobs);
                    }

                }

            }
        }
    }
}


/*Cette fonction verifie si les procesus fils ce sont terminé normalement si oui alors change le statut à done 
sinon change le statut à killed*/
void sigchld_handler(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            last_return_value = WEXITSTATUS(status);
            majStatusJobs(pid, jobsList, 0, jobsListSize); 
        }
        else{
            majStatusJobs(pid,jobsList,-2,jobsListSize);
        }
    }
}

int evaluate_ligne(char* ligne, char* previous_path, char* prompt_buffer,struct Job* jobs,unsigned int* jobsSize,pid_t pgid,bool isPipe){
    int original_stdout = dup(1);
    int original_stdin = dup(0);
    int original_stderr = dup(2);

    char* copy_line = malloc(sizeof(char)*(strlen(ligne)+1));
    

    bool is_sub_proc = false;
    // substitution de processus
    if(strstr(ligne, " <( ")){ 
        is_sub_proc = true;

        char* ligne_substituée = calloc(strlen(ligne)+1, sizeof(char));
        int nb_substi = 0;

        char* copy_line3 = calloc(strlen(ligne)+1,sizeof(char));
        strcpy(copy_line3,ligne);
        char* rest1;
        char* tmp = strtok_r(copy_line3, "\t ", &rest1);

        while(tmp!=NULL){
            
            // on crée les tubes anonymes
            int tube[2];
            pipe(tube);

            while(tmp!=NULL && strcmp(tmp,"<(")!=0 && strcmp(tmp,")")!=0){
                strcat(ligne_substituée, " ");
                strcat(ligne_substituée, tmp);
                tmp = strtok_r(NULL, "\t ", &rest1);
            }
            
            if(tmp!=NULL){
                char* subline = calloc(strlen(ligne)+1,sizeof(char));
                int parentheses = 1;
                tmp = strtok_r(NULL, "\t ", &rest1);
                
                while(tmp!=NULL && parentheses!=0){
                    if(strcmp(tmp,"<(")==0) ++parentheses;
                    else if(strcmp(tmp,")")==0) --parentheses;
                    if(parentheses!=0){
                        strcat(subline," ");
                        strcat(subline,tmp);
                    }
                    tmp = strtok_r(NULL, "\t ", &rest1);
                }

                switch(fork()){
                    case -1 :
                        perror("fork substitution de processus");
                        free(tmp);
                        free(ligne_substituée);
                        free(copy_line);
                        free(copy_line3);
                        free(subline);
                        //on redirige les sorties et entrées telles qu'elles étaient à la base
                        dup2(original_stdin, 0);
                        dup2(original_stdout, 1);
                        dup2(original_stderr, 2);
                        return 1;

                    case 0 :
                        close(tube[0]);
                        dup2(tube[1],1);
                        // on évalue la ligne de commande entre les parantheses
                        evaluate_ligne(subline, previous_path, prompt_buffer, jobs, jobsSize,pgid,isPipe);
                        close(tube[1]); // fermer le tube
                        
                        free(ligne_substituée);
                        free(copy_line3);
                        free(copy_line);
                        free(subline);
                        exit(0);

                    default : // on ajoute a la nouvelle ligne de commande l'adresse du tube
                        strcat(ligne_substituée, " ");
                        char* buffer1 = malloc(9+nb_chiffres(tube[0]));
                        sprintf(buffer1, "/dev/fd/%d", tube[0] );
                        strcat(ligne_substituée, buffer1);
                        close(tube[1]);

                        nb_substi++;
                        int status;
                        wait(&status);

                        free(buffer1);
                        free(subline);
                        break;         
                }
            }
        }
        strcpy(copy_line,ligne_substituée);
        free(ligne_substituée);
        free(copy_line3);
    }
    else{
        strcpy(copy_line,ligne);
    }

    char* rest;

    // les PIPES
    if(strstr(copy_line, " | ")){
       
        int occ_pipe = nbr_occurrences(copy_line, "|");
       
        //on crée les pipes
        int tab_pipe[occ_pipe][2];
        for(int i = 0; i<occ_pipe; i++){
            pipe(tab_pipe[i]);
        }

        char* copy_line2 = malloc(sizeof(char)*(strlen(copy_line)+1));
        strcpy(copy_line2,copy_line);
        char* rest2;
        
        //pour la premiere cmd on ne redirige pas l'entrée
        char* current_pos = strtok_r(copy_line2,"|", &rest2);
        struct Job newJob;//on cree la structure de job.
        pid_t pgidF=fork();
        switch(pgidF){
            case 0:
                signal_default();
                setpgid(0,0);
                dup2(tab_pipe[0][1], 1);
                for(int i =0; i<occ_pipe; i++){
                    close(tab_pipe[i][1]);
                    close(tab_pipe[i][0]);
                }

                evaluate_ligne(current_pos, previous_path, prompt_buffer, jobs, jobsSize,getpgrp(),true);
                free(copy_line2);
                free(copy_line);
                exit(0);
            case -1:
                perror("fork 1 pipes");
                //on redirige les sorties et entrées telles qu'elles étaient à la base
                dup2(original_stdin, 0);
                dup2(original_stdout, 1);
                dup2(original_stderr, 2);
                return 1;
            default: 
                newJob=createJob(pgidF,ligne,1);//on initialise notre job avec le pid du premier procesus du pipe.
                newJob.isPipe=true;
                jobsList[jobsListSize]=newJob;
                ++jobsListSize;
                ++nb_jobs;
                close(tab_pipe[0][1]);
                current_pos = strtok_r(NULL,"|", &rest2);
                break;
        }
        

        setpgid(pgidF,pgidF);//on definis dans le pere le groupe du procesus de pgidF
        int i = 0; // sert a savoir si c'est la derniere cmd
        while(current_pos!=NULL && i<occ_pipe-1){
            pid_t pidF=fork();
            
            switch(pidF){
                case 0:
                    signal_default();
                    setpgid(0,pgidF);//on place le fils dans le groupe pgidF
                    dup2(tab_pipe[i][0], 0);
                    dup2(tab_pipe[i+1][1], 1);
                    for(int i =0; i<occ_pipe; i++){
                        if(i!=0)close(tab_pipe[i][1]);
                        close(tab_pipe[i][0]);
                    }
                    evaluate_ligne(current_pos, previous_path, prompt_buffer, jobs, jobsSize,pgidF,true);
                    free(copy_line2);
                    free(copy_line);
                    exit(0);
                case -1:
                    perror("fork pipes");
                    //on redirige les sorties et entrées telles qu'elles étaient à la base
                    dup2(original_stdin, 0);
                    dup2(original_stdout, 1);
                    dup2(original_stderr, 2);
                    return 1;
                default:
                    struct Processus newProc=createProc(pidF,current_pos);//on crée un nouveau processus.
                    addProcToJob(&jobsList[jobsListSize-1],newProc);//on place le nouveau processus dans le tableau de processus du job.
                    close(tab_pipe[i][0]);
                    close(tab_pipe[i+1][1]);
                    current_pos = strtok_r(NULL,"|", &rest2);
                    i++;
                    break;
            }
        }

        //derniere cmd on ne redirige pas la sortie
        int pid;
        switch(pid=fork()){
            case 0:
                signal_default();
                setpgid(0,pgidF);
                dup2(tab_pipe[occ_pipe-1][0], 0);
                close(tab_pipe[occ_pipe-1][0]);
                evaluate_ligne(current_pos, previous_path, prompt_buffer, jobs, jobsSize,pgidF,true);
                free(copy_line2);
                free(copy_line);
                exit(0);
            case -1:
                perror("fork 1 pipes");
                //on redirige les sorties et entrées telles qu'elles étaient à la base
                dup2(original_stdin, 0);
                dup2(original_stdout, 1);
                dup2(original_stderr, 2);
                return 1;
            default:
                struct Processus newProc=createProc(pid,current_pos);//on crée un nouveau processus.
                addProcToJob(&jobsList[jobsListSize-1],newProc);//on place le nouveau processus dans le tableau de processus du job.
                close(tab_pipe[occ_pipe-1][0]);
                int status;
                tcsetpgrp(STDIN_FILENO, pgidF);
                //On attend la fin ou l'interruption des processus dans le groupe pgidF
                while ((pid = waitpid(-pgidF, &status, WUNTRACED)) > 0) {
                    //on verifie si un processus a été arrêté
                    if(WIFSTOPPED(status)){
                        majStatusJobs(pgidF, jobsList, 2, jobsListSize);//si le processus à reçu le signal SIGSTP ou SIGSTOP on met le statut du jobs à 2(stopped)
                        printJob(jobs+(jobsListSize-1), 2, &nb_jobs);//on affiche que le procesus à reçu  le signal stopped
                        break;
                    }
                    

                }

                 if(WIFEXITED(status)||WIFSIGNALED(status)){//on récupère la valeur de retour du processus fils
                        //si le processus a été tué
                        int sig=WTERMSIG(status);
                        if(sig==15){
                            fprintf(stderr,"Terminated\n");
                        }
                        last_return_value = WEXITSTATUS(status);
                        majStatusJobs(pgidF,jobsList,-1,jobsListSize);//si le processus a terminé normalement on l'enleve de la liste des jobs
                        --nb_jobs;
                        
                    
                    }
                    
                tcsetpgrp(STDIN_FILENO, getpgrp());
                free(copy_line2);
                free(copy_line);
                //on redirige les sorties et entrées telles qu'elles étaient à la base
                dup2(original_stdin, 0);
                dup2(original_stdout, 1);
                dup2(original_stderr, 2);
                return 0;
        }
        
    }
 
    //on verifie si la commande doit etre effectuée à l'arriere plan 
    //si c'est le cas on met arriere_plan a true et on enleve le &
    bool arriere_plan = false;
    int longueur_ligne = strlen(copy_line);
    if (longueur_ligne > 0 && copy_line[longueur_ligne - 1] == '&') {
        arriere_plan = true;
        copy_line[longueur_ligne-1] = '\0';
    }

    char* mot = strtok_r(copy_line,"\t )", &rest);

    if(mot!=NULL){//on enregistre la cmd avant les redirections
        char* cmd = malloc(sizeof(char)*(strlen(ligne)+1));
        strcpy(cmd,mot);
        mot = strtok_r(NULL,"\t ", &rest);
        while(mot!=NULL && !is_redirect(mot) && strcmp(mot,"<(")!=0 && strcmp(mot,"|")!=0){
            strcat(cmd, " ");
            strcat(cmd, mot);
            mot = strtok_r(NULL,"\t ", &rest);
        }

        while(mot!=NULL && is_redirect(mot)){ // c'est une redirection
            char* type = strdup(mot);
            mot = strtok_r(NULL,"\t ", &rest);
            if(mot==NULL){
                return 1;
            }
            char* fic = strdup(mot);
            if((last_return_value=redirect(fic, type))==1){
                free(copy_line);
                free(cmd);
                free(fic);
                free(type);
                return 1;
            }
            free(fic);
            free(type);
            mot = strtok_r(NULL, "\t ", &rest);
        }
           
        int nbr_mots = nombreMots(cmd);
        if(nbr_mots>0){
            //execution d'une commande en arrière plan
            if(arriere_plan){
                pid_t pid=fork();
                switch(pid){
                    case -1 :
                        perror("fork dans copy_line");
                        free(copy_line);
                        free(cmd);
                        //on redirige les sorties et entrées telles qu'elles étaient à la base
                        dup2(original_stdin, 0);
                        dup2(original_stdout, 1);
                        dup2(original_stderr, 2);
                        return 1;
                    case 0 :
                        setpgid(0,0);//on place le fils dans un groupe de processus
                        evaluate_cmd(cmd, nbr_mots, previous_path, prompt_buffer,jobs,*jobsSize,true,0,isPipe);
                        free(copy_line);
                        free(prompt_buffer);
                        free(ligne);
                        free(cmd);
                        exit(0);
                    default :   
                        //on crée un nouveau jobs avec le pid du processus fils executé en arriere plan
                        struct Job newJob=createJob(pid,cmd,1);
                        jobs[(*jobsSize)]=newJob;
                        ++(*jobsSize);
                        ++nb_jobs;
                        printJob(&newJob, 2, &nb_jobs);
                        //on redirige les sorties et entrées telles qu'elles étaient à la base
                        dup2(original_stdin, 0);
                        dup2(original_stdout, 1);
                        dup2(original_stderr, 2);
                        free(copy_line);
                        free(cmd);
                        return 0;
                }
            }
            evaluate_cmd(cmd, nbr_mots, previous_path, prompt_buffer,jobs,*jobsSize,false,pgid,isPipe);

            free(copy_line);
            free(cmd);
            //on redirige les sorties et entrées telles qu'elles étaient à la base
            dup2(original_stdin, 0);
            dup2(original_stdout, 1);
            dup2(original_stderr, 2);
            if(is_sub_proc){
                // on unlink nos tubes nommés si c'était une substitution de processus
                int occ_substi = nbr_occurrences(ligne," <( "); // le nbr de commandes dans la ligne 
                for(size_t i = 0; i<occ_substi; ++i){
                    char* tube_nomme = malloc(11+nb_chiffres(i));
                    sprintf(tube_nomme, "/tmp/tube%lu", i);
                    unlink(tube_nomme);
                    free(tube_nomme);
                }
            }
            return 0;
        }

        if(is_sub_proc){
            // on unlink nos tubes nommés si c'était une substitution de processus
            int occ_substi = nbr_occurrences(ligne," <( "); // le nbr de commandes dans la ligne 
            for(size_t i = 0; i<occ_substi; ++i){
                char* tube_nomme = malloc(11+nb_chiffres(i));
                sprintf(tube_nomme, "/tmp/tube%lu", i);
                unlink(tube_nomme);
                free(tube_nomme);
            }
        }
        
        free(copy_line);
        free(cmd);
        //on redirige les sorties et entrées telles qu'elles étaient à la base
        dup2(original_stdin, 0);
        dup2(original_stdout, 1);
        dup2(original_stderr, 2);
        return 1;
    }
    free(copy_line);
    //on redirige les sorties et entrées telles qu'elles étaient à la base
    dup2(original_stdin, 0);
    dup2(original_stdout, 1);
    dup2(original_stderr, 2);
    return 1;
}

void evaluate_cmd(char * cmd, int nbr_mots, char* previous_path, char* prompt_buffer,struct Job* jobs,unsigned int nbJobs,bool background,pid_t pgid,bool isPipe){
    char* copy_cmd = malloc(sizeof(char)*(strlen(cmd)+1));
    strcpy(copy_cmd,cmd);
    char* rest;
    //on découpe la ligne en mots
    char* mot = strtok_r(copy_cmd,"\t ", &rest);
    //on cherche quelle commande est appelée
    if(mot!=NULL){
        if(strcmp(mot,"cd")==0){//commande cd
            if(nbr_mots == 2){
                //on enregistre le chemin avant de se deplacer pour cd -
                char* cwd = malloc(PATH_MAX);
                getcwd(cwd, PATH_MAX);

                mot = strtok_r(NULL,"\t ", &rest);
                if(!strcmp(mot, "-")){//cas cd -
                    last_return_value=mon_cd(previous_path);
                }
                else{//cas cd chemin
                    last_return_value=mon_cd(mot);
                }

                strcpy(previous_path,cwd);
                free(cwd);
            }
            else if(nbr_mots==1){//cas cd
                char* cwd = malloc(PATH_MAX);
                getcwd(cwd, PATH_MAX);

                last_return_value=mon_cd("~");

                strcpy(previous_path,cwd);
                free(cwd);
            }
            else{//erreur de cd
                last_return_value=1;
                write(2,"Il y a trop d'arguments pour la commande cd.\n",45);
            }

        }

        else if(strcmp(mot,"pwd")==0){//commande pwd
            last_return_value=pwd();
        }

        else if(strcmp(mot,"exit")==0){//commande exit
            if(nbr_mots==2){//cas exit val
                if(jobsEnd(jobs,nbJobs)){
                    last_return_value=0;
                    mot = strtok_r(NULL,"\t ", &rest);
                    int val =atoi(mot);
                    mon_exit(val);
                }
                else last_return_value=1;
            }
            else {//cas exit
                free(copy_cmd);
                free(cmd);
                free(previous_path);
                free(prompt_buffer);
                mon_exit(last_return_value);
            }
            
        }
        else if(strcmp(mot,"?")==0){
            //on affiche la dernière valeur de retour, mise à jour à chaque commande
            fprintf(stdout, "%d\n", last_return_value); 
            last_return_value = 0;
        }
        else if(strcmp(mot,"jobs")==0){
            if(nbr_mots==1){
                procesusMaj(jobs,nbJobs);
                printJobs(jobs,&nbJobs,&nb_jobs,false);
                last_return_value = 0;
            }
            else if(nbr_mots==2){
                mot = strtok_r(NULL,"\t ", &rest);
                if(strcmp(mot,"-t")==0){
                    procesusMaj(jobs,nbJobs);
                    last_return_value=printJobs(jobs,&nbJobs,&nb_jobs,true);
                }
                else if(mot[0]=='%'){

                    pid_t jobId = atoi(mot+1);
                    procesusMaj(jobs,nbJobs);
                    last_return_value=printJobID(jobs,&nbJobs,&nb_jobs,jobId,false);
                }
                else{
                     fprintf(stderr, "argument incorrect\n");
                     last_return_value=1;
                }

            }
            else if(nbr_mots==3){
                mot = strtok_r(NULL,"\t ", &rest);
                if(strcmp(mot,"-t")==0){
                    mot = strtok_r(NULL,"\t ", &rest);
                    if(mot[0]=='%'){
                        pid_t jobId = atoi(mot+1);
                        procesusMaj(jobs,nbJobs);
                        last_return_value=printJobID(jobs,&nbJobs,&nb_jobs,jobId,true);
                    }
                    else{
                        fprintf(stderr, "argument incorrect\n");
                        last_return_value=1;
                    }
                }
                else{
                    fprintf(stderr, "argument incorrect\n");
                    last_return_value=1;
                }
            }
        }

        else if(strcmp(mot,"kill")==0){
            int sig = 15;//signale de base SIGTERM
            int pid = -1;
            if(nbr_mots==1){
                fprintf(stderr, "kill : usage : kill [-signal] [pid]\n");
                last_return_value = 1;
            }
            //kill sans paramètre (SIGTERM)
            else if (nbr_mots==2){
                mot = strtok_r(NULL, "\t ", &rest);
                //cas ou on appelle kill sur le numéro du job
                if(mot[0]=='%'){
                    int num_job = atoi(mot+1);
                    for(int i=0;i<nbJobs;++i){
                        if(jobs[i].jobsId==num_job){
                            pid = jobs[i].pid;
                            last_return_value = mon_kill(sig, pid, jobs, nbJobs,&nb_jobs);
                        }
                    }
                }
                // cas ou on appelle kill sur le pid du job
                else if(isdigit(mot[0])){
                    pid = atoi(mot);
                    last_return_value = mon_kill(sig, pid, jobs, nbJobs,&nb_jobs);
                }
                else{
                    fprintf(stderr, "kill : usage : kill [-signal] [pid]\n");
                    last_return_value = 1;
                }
            }
            //kill avec paramètre
            else if(nbr_mots==3){
                mot = strtok_r(NULL, "\t ", &rest);
                if(mot[0]!='-'){
                    fprintf(stderr, "kill : usage : kill [-signal] [pid]\n");
                    last_return_value = 1; 
                }
                else{
                    sig = -(atoi(mot));
                    mot = strtok_r(NULL, "\t ", &rest);
                    if(mot[0]=='%'){
                        int num_job = atoi(mot+1);
                        for(int i=0;i<nbJobs;++i){
                            if(jobs[i].jobsId==num_job){
                                pid = jobs[i].pid;
                                last_return_value = mon_kill(sig, pid, jobs, nbJobs,&nb_jobs);
                            }
                        }
                    }
                    else if(isdigit(mot[0])){
                        pid = atoi(mot);
                        last_return_value = mon_kill(sig, pid, jobs, nbJobs,&nb_jobs);
                    }
                    else{
                        fprintf(stderr, "kill : usage : kill [-signal] [pid]\n");
                        last_return_value = 1;
                    }
                }
            }
        }
        else if(strcmp(mot,"bg")==0){
            if(nbr_mots==2){
                mot = strtok_r(NULL, "\t ", &rest);
                if(mot[0]=='%'){
                    int num_job=atoi(mot+1);
                    last_return_value=bg(num_job,jobsList,jobsListSize);
                }
            }
        }
         
    
        else if(strcmp(mot,"fg")==0){
            if(nbr_mots==2){
                mot = strtok_r(NULL, "\t ", &rest);
                if(mot[0]=='%'){
                    int num_job=atoi(mot+1);
                    last_return_value=fg(num_job,jobsList,jobsListSize,&nb_jobs);
                }
                else{
                    fprintf(stderr,"argument incorrect");
                }
            }

        }
        else{//commande externe
            //on alloue le tableau d'arguments et on le remplit
            char** argv = malloc((nbr_mots+1)*sizeof(char*));
            argv[0] =strdup(mot);
            for(int i = 1; i<nbr_mots;++i){
                char* arg = strtok_r(NULL," \t", &rest);
                *(argv+i) = strdup(arg);
            }
            *(argv+nbr_mots)=NULL;

            //execution de la commande externe en arrière plan ou bien si elle est dans un pipe
            if(background||isPipe){
                if(execvp(mot, argv)==-1){//si l'execution ne marche pas on écrit le message d'erreur qui convient
                        last_return_value=1;
                        if(errno == ENOENT){
                            write(2, mot, strlen(mot));
                            write(2, " : Commande inconnue.\n", 22);
                        }
                        else if(errno == EINVAL){
                            perror("Arguments invalides\n");
                        }
                        else{
                            perror("Erreur d'exécution de la commande externe\n");
                        }
                        for(int i = 0; i<nbr_mots;++i){
                            free(*(argv+i));
                        }
                            free(argv);
                            free(copy_cmd);
                            exit(last_return_value);
                        }
            }
            else{
            
            pid_t fils;
            //on crée le processus fils qui executera la commande
            switch(fils=fork()){
                case -1 : //le fork n'a pas marché
                    perror("fork");
                    last_return_value=1;
                    break;

                case 0 ://le code du fils
                    setpgid(0,0);//on place le fils dans un groupe de procesus
                    signal_default();
                    if(execvp(mot, argv)==-1){//si l'execution ne marche pas on écrit le message d'erreur qui convient
                        last_return_value=1;
                        if(errno == ENOENT){
                            write(2, mot, strlen(mot));
                            write(2, " : Commande inconnue.\n", 22);
                        }
                        else if(errno == EINVAL){
                            perror("Arguments invalides\n");
                        }
                        else{
                            perror("Erreur d'exécution de la commande externe\n");
                        }
                        for(int i = 0; i<nbr_mots;++i){
                            free(*(argv+i));
                        }
                            free(argv);
                            free(copy_cmd);
                            exit(last_return_value);
                    }
                        
                default ://le code du père
                    int status;
                    //on crée un job pour la commande externe en avant plan
                    struct Job newJob=createJob(fils,cmd,1);
                    jobsList[jobsListSize]=newJob;
                    ++jobsListSize;
                    ++nb_jobs;
                    tcsetpgrp(STDIN_FILENO, fils);
                    waitpid(fils, &status,WUNTRACED | WCONTINUED);
                    tcsetpgrp(STDIN_FILENO, getpgrp());
                    if(WIFEXITED(status)||WIFSIGNALED(status)){//on récupère la valeur de retour du processus fils
                        //si le processus a été tué
                        int sig=WTERMSIG(status);
                        if(sig==15){
                            fprintf(stderr,"Terminated\n");
                        }
                        last_return_value = WEXITSTATUS(status);
                        majStatusJobs(fils,jobsList,-1,jobsListSize);//si le processus a terminé normalement on l'enleve de la liste des jobs
                        --nb_jobs;
                    }
                    else if(WIFSTOPPED(status)){
                        majStatusJobs(fils, jobsList, 2, jobsListSize);//si le processus à reçu le signal SIGSTP ou SIGSTOP on met le statut du jobs à 2(stopped)
                        printJob(jobs+(jobsListSize-1), 2, &nb_jobs);//on affiche que le procesus à reçu  le signal stopped
                    }
                    break;
                }

                
            }
            //on libère la mémoire allouée pour le tableau
            for(int i = 0; i<nbr_mots;++i){
                free(*(argv+i));
            }
            free(argv);
        }
        while(mot!=NULL){//sert a s'assurer que strtok finit a NULL pour éviter les problèmes de thread
            mot = strtok_r(NULL," \t", &rest);
        }
    }
    free(copy_cmd);
}

char* prompt(){
    char* prompt_buffer = calloc(57,sizeof(char));
    // le chemin du répértoire courant
    char* chemin = calloc(PATH_MAX+1,sizeof(char));
    getcwd(chemin,PATH_MAX);
    char* cheminTronque = chemin_tronque(chemin);
    // on met le chemin (tronqué) dans le prompt 
    // (ainsi que le nb de jobs et les couleurs)
    int taille = strlen(cheminTronque) + nb_chiffres(nb_jobs) + 19 + 4 +6;
    snprintf(prompt_buffer, taille,
            "\001%s\002[%d]\001%s\002%s\001%s\002$ ", 
            PURPLE_TEXT, nb_jobs, CYAN_TEXT, 
            cheminTronque, RESET_COLOR);
    free(chemin);
    free(cheminTronque);
    return prompt_buffer;
}

int main(){
    // pour le readline
    rl_outstream = stderr;
    write(1,"~ Mon terminal ~\n",17);
    //on initialise la list de jobs 
    jobsList=calloc(MAX_JOBS,sizeof(struct Job));
    // On initialise previous_path pour cd -
    char* previous_path = malloc(PATH_MAX);
    getcwd(previous_path, PATH_MAX);
   
    //initialisation de la structure pour la gestion du signal SIGCHLD
    struct sigaction sa;
    // on initialise la structure sa à zéro
    memset(&sa, 0, sizeof(struct sigaction));
    // attribution du gestionnaire de signal pour SIGCHLD
    sa.sa_handler = sigchld_handler;
    //définition des indicateurs pour le comportement du signal
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    // enregistrement du gestionnaire de signal pour SIGCHLD
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {     
        perror("sigaction 1");
        exit(EXIT_FAILURE);
    }

    // ignorer les signaux SIGINT SIGTERM SIGTTIN SIGQUIT SIGTTOU SITSTP
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIG_IGN;

    if
     (sigaction(SIGINT, &action, NULL)==-1
     || sigaction(SIGTERM, &action, NULL)==-1
     || sigaction(SIGTTIN, &action, NULL)==-1
     || sigaction(SIGQUIT, &action, NULL)==-1
     || sigaction(SIGTTOU, &action, NULL)==-1
     || sigaction(SIGTSTP, &action, NULL)==-1)
    {
        perror("sigaction 2");
        exit(EXIT_FAILURE);
    }

    // on ouvre notre terminal
    while(1){
        procesusMaj(jobsList,jobsListSize);    
        //on affiche les jobs done avant d'afficher le prompts on met egalement à jour le nombre de jobs en cours
        printJobsDone(jobsList,&jobsListSize,&nb_jobs);
        char* prompt_buffer = prompt();
        
        // on laisse la main à l'utilisateur
        char* ligne = readline(prompt_buffer);
        add_history(ligne); // pour utiliser les flèches
          
        // CTRL+D
        if(!ligne){
            mon_exit(last_return_value);
        }
        
        // on récupère la ligne tapée par l'utilisateur et on l'évalue
        evaluate_ligne(ligne, previous_path, prompt_buffer,jobsList,&jobsListSize,0,false);
       
        // on libère la mémoire allouée
        free(ligne);
        free(prompt_buffer);
    }
    free(previous_path);
    return 0;
}