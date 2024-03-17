# Interpréteur de commandes (Shell) - jsh

## Objectif
Le but du projet est de programmer un interpréteur de commandes (shell) interactif qui reprend quelques fonctionnalités classiques des shells usuels, incluant le job control pour la gestion des tâches lancées depuis le shell.

## Fonctionnalités

### Notion de job
- **Gestion de tâches multiples :** Permet le contrôle de l'exécution simultanée de plusieurs tâches et leur bascule entre l'avant- et l'arrière-plan.
- **Identification et surveillance :** Chaque groupe de processus crée constitue un job avec un numéro unique pour une gestion et surveillance efficaces.
- **Contrôle des jobs :** Les jobs peuvent être lancés en arrière-plan ou en avant-plan et sont gérés avec des commandes spécifiques pour lister, reprendre, ou arrêter ces jobs.

### Commandes externes
- **Exécution des commandes système :** jsh peut exécuter toutes les commandes externes disponibles dans le `PATH` de l'environnement.

### Commandes internes
- `pwd` : Affiche le répertoire de travail courant.
- `cd [ref | -]` : Change le répertoire de travail.
- `?` : Affiche la valeur de retour de la dernière commande exécutée.
- `exit [val]` : Termine le processus jsh.
- `jobs [-t] [%job]` : Liste les jobs avec des détails selon les options.
- `bg %job` : Relance un job en arrière-plan.
- `fg %job` : Relance un job en avant-plan.
- `kill [-sig] %job | pid` : Envoie un signal à un job ou processus.

### Gestion de la ligne de commande
- **Mode interactif :** jsh fonctionne en mode interactif, analysant et exécutant les commandes entrées par l'utilisateur.

### Formatage du prompt
- **Personnalisation :** Le prompt est configurable et peut refléter l'état des jobs et le répertoire courant.

### Redirections et combinaisons par tube
- **Redirection des flots :** jsh gère les redirections standards (`<`, `>`, `2>`, `>>`, etc.) ainsi que les combinaisons par tube (`|`).

### Gestion des signaux
- **Traitement des signaux :** jsh gère spécifiquement les signaux pour ne pas être interrompu inopinément.

## Utilisation
- make
-  ./jsh

