#include <ucontext.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

/*
 * Contrairement à -scheduler-pthread qui démarre de vrais threads, cette version fonctionne avec uniquement un seul thread.
 *
 * Usage: gcc -o mini-scheduler-context mini-scheduler-context.c
 * ./mini-scheduler-context
 */


#define NUMCONTEXTS 2              /* how many contexts to make */
#define STACKSIZE 4096              /* stack size */

ucontext_t contexts[NUMCONTEXTS];
int curcontext = 0;
ucontext_t *cur_context;

ucontext_t scheduler_context;
void *scheduler_stack;


void scheduler();
void timer_handler(int j, siginfo_t *si, void *old_context);
void setup_signals(void);
void *thread0();
void *thread1();
void mkcontext(ucontext_t *uc,  void *function);

int main(){
	int i;
	struct itimerval it;

	scheduler_stack = malloc(STACKSIZE);
	if (scheduler_stack == NULL) {
		perror("Problème avec malloc");
		exit(1);
	}
	/*
	 * Création des contextes pour nos deux threads
	 */
	mkcontext(&contexts[0], thread0);
	mkcontext(&contexts[1], thread1);
	/*
	 * Positionnement des handlers
	 */
	setup_signals();
	/*
	 * Configuration du timer
	 */
	it.it_interval.tv_sec = 5;
	it.it_interval.tv_usec = 0;
	it.it_value = it.it_interval;
	if (setitimer(ITIMER_REAL, &it, NULL) )//à chaque timer l'OS envoie le signal SIGALRM 
		perror("Problème de setitiimer");

	/*
	 * On force le démarrage du premier thread
	 */
	cur_context = &contexts[0];
	setcontext(&contexts[0]);

	return 0;
}

/*
 * Scheduler: round-robin
 */
void scheduler(){
	printf("Scheduling out thread %d\n", curcontext);
	curcontext = (curcontext + 1) % NUMCONTEXTS; /* round robin */
	cur_context = &contexts[curcontext];
	printf("scheduling in thread %d\n", curcontext);
	/*
	 * On swap le nouveau contexte et on l'exécute
	 */
	setcontext(cur_context);
}

/*
 * Le handler du timer, permet d'appeler le scheduler à chaque intervalle de temps.
 */
void timer_handler(int j, siginfo_t *si, void *old_context){
	/*
	 * On crée un contexte pour le scheduler
	 */
	getcontext(&scheduler_context);
	scheduler_context.uc_stack.ss_sp = scheduler_stack;
	scheduler_context.uc_stack.ss_size = STACKSIZE;
	scheduler_context.uc_stack.ss_flags = 0;
	sigemptyset(&scheduler_context.uc_sigmask);
	makecontext(&scheduler_context, scheduler, 1);
	/*
	 * On sauvegarde le contexte actuel et on va directement dans le scheduler
	 */
	swapcontext(cur_context,&scheduler_context);
}

/*
 * Positionnement du handler pour SIGALRM, pour scheduler périodiquement le scheduler.
 */
void setup_signals(void){
	struct sigaction act;
	sigset_t set;
	/*
	 * Noter ici l'utilisation de sa_sigaction au lieu de sa_handler.
	 * sa_sigaction permet de faire un handler complexe, qui prend plus de paramètres qu'un handler sa_handler.
	 * Dans notre cas, on veut que le handler prenne en paramètre le context qui a été dégagé par le scheduler (voir timer_handler)
	 */
	act.sa_sigaction = timer_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	if(sigaction(SIGALRM, &act, NULL) != 0) {
		perror("Problème du positionnement du handler de SIGALRM.");
	}
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
 * On construit un nouveau context, à partir du context courant.
 */
void mkcontext(ucontext_t *uc,  void *function){
	void * stack;

	getcontext(uc);
	stack = malloc(STACKSIZE);
	if (stack == NULL) {
		perror("Problème avec malloc");
		exit(1);
	}
	uc->uc_stack.ss_sp = stack;
	uc->uc_stack.ss_size = STACKSIZE;
	uc->uc_stack.ss_flags = 0;
	if (sigemptyset(&uc->uc_sigmask) < 0){
		perror("Problem avec sigemptyset");
		exit(1);
	}
	//On crée le nouveau context, qui exécutera la fonction function, avec 1-1 (i.e. zéro) argument
	makecontext(uc, function, 1);
}

