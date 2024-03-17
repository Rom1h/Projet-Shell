#include "redirections.h"

void print_errors(char * fic){
    switch(errno){
        case EACCES : 
            perror("ERREUR : Permissions insuffisantes.");
            break;
        case EEXIST :
            perror("ERREUR : Le fichier existe déjà.");
            fprintf(stderr, "%s", fic);
            break;
        case EISDIR :
            perror("ERREUR : C'est un répertoire.");
            break;
        case ENOENT :
            perror("ERREUR : Le fichier n'existe pas.");
            break;
        default : 
            perror("ERREUR : Ouverture du fichier impossible.");
            break;
    }
}

int redirect(char* fic, char* type){
    if(is_redirect(type)){
        if(!strcmp(type,"<")){
            int fd = open(fic, O_RDONLY);
            if(fd==-1){
                print_errors(fic);
                return 1;
            }   
            if(dup2(fd,STDIN_FILENO)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
        else if(!strcmp(type,">")){
            int fd = open(fic, O_WRONLY|O_CREAT|O_EXCL, 0666);
            if(fd==-1){
                print_errors(fic);
                return 1;
            }   
            if(dup2(fd,STDOUT_FILENO)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
        else if(!strcmp(type,">|")){
            int fd = open(fic, O_WRONLY|O_TRUNC|O_CREAT, 0666);
            if(fd==-1){
                print_errors(fic);
                return 1;
            }   
            if(dup2(fd,STDOUT_FILENO)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
        else if(!strcmp(type,">>")){
            int fd = open(fic, O_WRONLY|O_APPEND|O_CREAT, 0666);
            if(fd==-1){
                print_errors(fic);
                return 1;
            } 
            if(dup2(fd,STDOUT_FILENO)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
        else if(!strcmp(type,"2>")){
            int fd = open(fic, O_WRONLY|O_CREAT|O_EXCL, 0666);
            if(fd==-1){
                print_errors(fic);
                return 1;
            }   
            if(dup2(fd,2)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
        else if(!strcmp(type,"2>|")){
            int fd = open(fic, O_WRONLY|O_TRUNC|O_CREAT, 0666);
            if(fd==-1){
                print_errors(fic);
                return 1;
            }   
            if(dup2(fd,2)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
        else if(!strcmp(type,"2>>")){
            int fd = open(fic, O_WRONLY|O_APPEND|O_CREAT, 0666);
            if(fd==-1){
                print_errors(fic);
                return 1;
            }
            if(dup2(fd,2)==-1){
                close(fd);
                perror("ERREUR : dup2 (redirect)");
                return 1;
            }
            close(fd);
        }
    }
    else{
        perror("La redirection n'est pas reconnue");
        return 1;
    }
    return 0;
}