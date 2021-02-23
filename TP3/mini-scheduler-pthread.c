#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

/*
 * Dans cet exemple, on crée des threads avec pthread.
 * Ces threads seront gérés par l'OS, donc le scheduler de ce dernier.
 * Ceci signifit que nous ne maîtrisons pas le scheduling de ces threads.
 * Le scheduler de l'OS va donner l'illusion de parallélisme.
 * Nous souhaitons implanter un scheduler en user space, qui va alterner l'exécution des threads.
 * Nous avons deux threads à scheduler.
 * Un troisième thread joue le rôle de scheduler.
 * Le scheduler laisse 10 premières secondes au scheduler de l'OS pour montrer que les threads sont bien exécuter en "parallèle".
 * Ensuite notre scheduler fait en sorte que toutes les 5 secondes, il endort un thread et reveil un autre.
 *
 * Pour la réalisation:
 * -Penser à sigaction et les signaux SIGUSR1 et SIGUSR2
 * -Penser à sigsuspend
 * -Penser évidemment à pthread_create et pthread_join
 *
 *
 * Usage: gcc -o mini-scheduler-pthread mini-scheduler-pthread.c -lpthread
 * ./mini-scheduler-pthread
 */

pthread_t tcbs[2],sched_tcb;

void *thread0();
void *thread1();
void *schedule();
void handle_signal_sigusr1(int signal);
void handle_signal_sigusr2(int signal);


int main() {
	int pid,rc,wstatus;
	struct sigaction sa_usr1,sa_usr2;
	void *status;

	//Handler pour la suspension d'un thread
	sa_usr1.sa_handler = &handle_signal_sigusr1;
	sa_usr1.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
		perror("Error: cannot handle SIGUSR1"); // Should not happen
	}
	//Handler pour débloquer un thread suspendu
	sa_usr2.sa_handler = &handle_signal_sigusr2;
	sa_usr2.sa_flags = SA_RESTART;
	if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1) {
		perror("Error: cannot handle SIGUSR2"); // Should not happen
	}
	//Création des threads
	if (pthread_create(&sched_tcb, NULL, schedule, NULL) != 0) {
		perror("pthread_create Scheduler");
		exit(1);
	}
	if (pthread_create(&tcbs[0], NULL, thread0, NULL) != 0) {
		perror("pthread_create thread-1");
		exit(1);
	}
	if (pthread_create(&tcbs[1], NULL, thread1, NULL) != 0) {
		perror("pthread_create thread-1");
		exit(1);
	}
	//Les joins
	if (pthread_join(sched_tcb, &status) != 0) {
		perror("pthread_join - Scheduler"); exit(1);
	}
	if (pthread_join(tcbs[0], &status) != 0) {
		perror("pthread_join - Thread-1"); exit(1);
	}
	if (pthread_join(tcbs[1], &status) != 0) {
		perror("pthread_join - Thread-2"); exit(1);
	}
}

/*
 * Permet de suspendre un thread
 */
void handle_signal_sigusr1(int signal) {
	sigset_t mask;
	sigemptyset (&mask);
	//printf("Caught SIGUSR1\n");
	sigsuspend(&mask);
}

/*
 * Utilisé pour débloquer un thread endormi sur sigsuspend
 */
void handle_signal_sigusr2(int signal) {
	//printf("Caught SIGUSR2\n");
}

/*
 * Le thread 0
 */
void *thread0(){
	while(1){
		printf("--------------Thread-0\n");
		sleep(2);
	}
}

/*
 * Le thread 1
 */
void *thread1(){
	while(1){
		printf("Thread-1---------------\n");
		sleep(2);
	}
}

/*
 * Le scheduler
 */
void *schedule(){
	int i=0,toSchedule;
	/*
	 * Pendant les 10 premières secondes, on va voir que tous les threads s'exécutent, une sorte de parallélisme.
	 * Ceci est dû au fait que le scheduler de l'OS voit ces threads et les schedule
	 */
	sleep(10);
	/*
	 * Maintenant on va faire en sorte que notre scheduler gère séquenciellement les threads. Comme si on n'avait qu'un seul processeur.
	 */
	while(1){
		toSchedule=(i++%2);
		printf("Scheduling out thread %d\n",toSchedule);
		printf("Scheduling in thread %d\n",(toSchedule+1)%2);
		/*
		 * On bloque le thread tcbs[toSchedule] en lui envoyant SIGUSR1.
		 * Le handler de ce dernier effectue un sigsuspend pour bloquer le thread.
		 */
		pthread_kill(tcbs[toSchedule],SIGUSR1);
		/*
		 * Débloquer le thread précédemment bloqué.
		 * En effet, il est bloqué avec sigsuspend.
		 * Ce dernier suspend le processus et le relache à la reception de n'importe
		 * quel signal pour lequel un handler avec été défini.
		 * Ici on envoit le signal SIGUSR2
		 */
		pthread_kill(tcbs[(toSchedule+1)%2],SIGUSR2);//Permet de dépbloquer le thread précédemment bloqué. En eff
		sleep(5);
	}
}
