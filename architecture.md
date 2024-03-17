Notre programme est séparé dans trois fichiers différents :

        - commandes.c : ce fichier contient toutes les commandes internes 
                        de notre shell et certaines fonctions auxiliaire utilisées pour 
                        l'implémentation de ces commandes.

        - jobs.c : Pour l’implémentation des jobs nous utilisons une structure Job qui va stocker :

                    -le pid du job
                    -sa commande
                    -son statut (Running/Stopped/Killed..)
                    -son id
                    -un tableau de processus contenant tous les processus de ce jobs (pid/commande)

                    Nous avons également une structure Processus qui va nous permettre de stocker les informations sur la commande et le pid du processus. Le fichier jobs.c va s’occuper de gérer l’affichage et la création des jobs et processus. Pour l’affichage des jobs nous avons une méthode pour afficher tous les jobs qui ont terminé ou bien qui ont reçu un signal pour les kill.
                    Pour la fonction jobs nous parcourons simplement la liste de jobs placée en paramètre et on affiche chaque job.

                    Pour jobs -t on va :
                        -afficher pour chaque job tous les processus qui y sont rattachés
                        -pour chaque processus on va afficher son arborescence c’est à dire tous ses fils en parcourant le répertoire /proc et en vérifiant dans fichier statut si le ppid correspond au pid du processus courant

                    Implémentation des jobs pour les pipe :
                    Lorsque nous exécutons une commande avec un pipe on va définir son pid par le premier processus qu’on va exécuter (celui qui est le chef du groupe) puis ensuite pour chaque commande du pipe on va créer un nouveau processus qu’on va ajouter au job.
                    Notre JSH va stocker tous les jobs dans un tableau.

        - redirection.c : ce fichier contient une fonction servant à implémenter 
                          la majorité redirections des entrées et sorties de commandes

        -jsh.c : ce fichier est le centre de notre projet, c'est la où se 
                 trouve le main, où l'on s'occupe de découper et de traiter la ligne
                 entrée, de réaliser les redirections et de lancer les commandes qui 
                 conviennent. Les deux fonctions les plus importantes de ce fichier 
                 sont evaluate_cmd et evaluate_ligne. La première sert à executer une
                 commande simple dont les redirections (simples, pipes, substutions...)
                 et l'arrière plan ont déja été traités par la fonction evaluate_ligne.
                 La fonction evaluate_cmd est uniquement appelée par evaluate_ligne 
                 tandis que cette dernière est appelée dans le main.