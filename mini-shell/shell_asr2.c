#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h> 

#include "minishell.h"

int main()
{
	char input[INPUT_LENGTH], *cmd_args[LENGTH];
	int nb_args;//, rc, wstatus;

	init_shell();
	
	memset(env, 0, sizeof(env));
	memset(input, 0, sizeof(input));
	memset(cmd_args, 0, sizeof(cmd_args));
	memset(procs,0,sizeof(procs));
	
	signal(SIGINT, sigint_hdle);
	signal(SIGTSTP, sigtstp_hdle);
	signal(SIGCONT,sigcont_hdle);
	signal(SIGCHLD, sigchld_hdle);

	while(1)
	{
		if(readInput(input))
			continue;//continue to loop

		nb_args = parseInput(input, cmd_args);
		
		execute(cmd_args, nb_args);			
	}
}
