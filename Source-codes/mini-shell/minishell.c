#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include <signal.h>
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h> 

#include "minishell.h"

int current_env_nb = 0, curr_procs_nb = 0, found_path = 0;//, current_process;
char name_ps[LENGTH];

/**************Process status****************/
#define RUNNING 1
#define PAUSED  2
#define STOPPED 3

// Greeting shell during startup
void init_shell()
{
	printf("\n\n\n\n******************"
        "************************");
    	printf("\n\n\n\t****THIS IS OUR MINISHELL****");
    	printf("\n\n\t-USE IT AT YOUR OWN RISK :)-");
	printf("\n\n\n\n*******************"
        "***********************");
    	char* username = getenv("USER"), cwd[1024];
    	printf("\n\n\nUSER is: @%s", username);
    	printf("\n");

	getcwd(cwd, sizeof(cwd)); 
	printf("\nDir: %s", cwd); 
}

void ms_help()
{
    puts("\n***WELCOME TO THE MINISHELL HELP***"
        "\nList of commands supported:"
        "\n>ms_help   -- Print this help."
        "\n>ms_exit   -- Exit the shell."
        "\n>ms_ps     -- Display the list of processes that you have."
	"	         Executed since you launched your session on the shell."	
	"\n>ms_export -- Export an environment variable."
	"\n		 You should specify the name and value of the environment"
	"\n	         variable to export (export ENV_NAME=ENV_VALUE)."
	"\n>ms_env    -- List the environment variables."        
	"\n>ms_kill   -- Send a signal to a process."
	"\n		 You should specify the option and the pid of the process."
	"\n	         (kill -option pid) - Option is either :"
	"\n	         1 - to suspend the process."
	"\n	         2 - to continue a suspended process."
	"\n	         3 - to kill (stop) the process."
        "\n>You can also use any default shell built-in command (provided that you exported the related path)."
        ); 
  
    return;
}

void ms_exit()
{
	printf("\nHope it was a pleasure to use this shell!\nSee you soon...\n");
	exit(0);
	exit(0);
}

void ms_export(char **path_value)
{
	char *tmp = strdup(path_value[1]);

	strcpy(env[current_env_nb].name, strsep(&tmp, "="));
	strcpy(env[current_env_nb].value, tmp);

	current_env_nb++;
}

void ms_ps()
{
	int ps_found=0;
	if(curr_procs_nb == 0)
	{
		printf("\nNo running process!\n");
		return;
	}
	for(int i=0; i<curr_procs_nb; i++)
	{
		if(procs[i].status != STOPPED)
		{
			ps_found = 1;
			break;
		}
	}
	if(ps_found)
	{
		printf(" PID     PPID     STATUS     CMD\n");
		for(int i=0; i<curr_procs_nb; i++)
		{
			if(procs[i].status != STOPPED)
				printf("%d     %d     %d     %s\n", procs[i].pid, procs[i].ppid, procs[i].status, procs[i].name);
		}
	}
	else
		printf("\nNo running process!\n");
}

void update_ps_status(int pid, char *name, int status)
{//if(status == STOPPED) curr_procs_nb--; 
	for(int i=0; i<curr_procs_nb; i++)
	{
		if(procs[i].name == name)
			procs[i].status = status;
		if(procs[i].pid == pid)
			procs[i].status = status;
	}
}

void ms_env()
{
	for(int i=0; i<current_env_nb; i++)
	{
		printf("%s = %s\n", env[i].name, env[i].value);
	}
}

int pid_exist(int pid, char *name)
{
	int exist = 0;

	for(int i=0; i<curr_procs_nb; i++)
	{
		if(procs[i].pid == pid || !strcmp(procs[i].name, name))
		{
			exist = 1;
			break;
		}
	}
	
	return exist;
}

void ms_kill(char **cmd_args)
{
	char *cmd = cmd_args[0], *sig = cmd_args[1]+1;
	int pid, signal;
	
	if(!strcmp(cmd, "fg"))
	{
		kill(current_process,SIGCONT);
	}
	else
	{
		signal = atoi(sig);
		pid = atoi(cmd_args[2]);
		
		if(!pid_exist(pid, "\0"))	
			printf("Process not found\n");

		switch(signal)
		{
			case 1:
				kill(pid,SIGTSTP);
				break;
			case 2:
				kill(pid,SIGCONT);
				break;
			case 3:
				kill(pid,SIGKILL);
				update_ps_status(pid, "\0", STOPPED);
				break;
			default:
				printf("\nSorry, Kill option not implemented in this shell!\n");
				break; 
		}
	}
}

int readInput(char *input)
{
	char *buf;
	
	buf = readline("\n>> ");

	if (strlen(buf) != 0) 
	{  
		strcpy(input, buf); 
		return 0; 
	} 
	else  
		return 1; 
}

int parseInput(char *input, char **cmd_args)
{
	int i;

	for ( i = 0; i < LENGTH; i++ ) 
	{ 
		cmd_args[i] = strsep(&input, " "); 
	  
		if (cmd_args[i] == NULL) 
		    break; 
		if (strlen(cmd_args[i]) == 0) 
		    i--; 
    	} 
	 	
	return i;
}

int InternCmd(char *cmd)
{
	char *MS_CMD[NB_CMDS];
	int i=0;

	memset(MS_CMD,0,sizeof(MS_CMD));

	MS_CMD[0] = "ms_help";
	MS_CMD[1] = "ms_exit";
	MS_CMD[2] = "ms_env";
	MS_CMD[3] = "ms_export";
	MS_CMD[4] = "ms_ps";
	MS_CMD[5] = "ms_kill";
	MS_CMD[6] = "fg";

	for (i = 0; MS_CMD[i] != NULL && i < NB_CMDS; i++) 
	{ 
		if (!strcmp(cmd, MS_CMD[i])) 
		    break;
    	} 	

	if(i == NB_CMDS || MS_CMD[i] == NULL)
		return -NINTERN;
	else
		return i;
}

void execIntern(int num_cmd, char **cmd_args)
{
	char *kil[2]={"kill", "-2"};
	switch(num_cmd)
	{
		case 0:
			ms_help();
			break;
		case 1:
			ms_exit();
			break;
		case 2:
			ms_env();
			break;
		case 3:
			ms_export(cmd_args);
			break;
		case 4:
			ms_ps();
			break;
		case 5:
			ms_kill(cmd_args);
			break;
		case 6:
			ms_kill(kil);
			break;
		default:
			ms_help();
			break;
	}
}

void find_path(char *cmd, char *path)
{
	found_path=0;

	if(strcmp(cmd,"ms_export") && current_env_nb == 0) 
	{
		printf("\nYou have not exported any path!\n"); 
		ms_help();
		return;
	}

	char *value, possible_paths[LENGTH]; 
	
	for(int i=0; env[i].name != NULL && i<current_env_nb; i++)
	{
		if( !strcmp(env[i].name, "PATH") )
		{
			value = strdup(env[i].value);

			for(int i=0; i<LENGTH && value != NULL; i++)
			{
				memset(possible_paths,0,sizeof(possible_paths));
				strcpy(possible_paths, strsep(&value, ":"));

				if(possible_paths != NULL )
				{
				  	strcat(possible_paths, "/");
					strcat(possible_paths, cmd);

					if( access(possible_paths, F_OK) == 0)
					{
						strcpy(path, possible_paths);
						found_path = 1; return;
					}
				}
				
		    	}	

			if(found_path)
				break;
		}
	}

	 if(!found_path) 
		printf("Environment not exported!\n");
}

void sigchld_hdle(int sigop)
{
	int status, rc;
	rc = waitpid(-1, &status, WNOHANG);
	
	if(rc>0)
	{
		
		if(WIFEXITED(status)) 
			update_ps_status(rc, name_ps, STOPPED);
		else if(WIFSIGNALED(status)) 	
			update_ps_status(rc, name_ps, STOPPED);
		else if(WIFSTOPPED(status)) 
		{		
			update_ps_status(rc, name_ps, PAUSED);
			//printf("WIFSTOPPED.\n");//,WSTOPSIG(status));
		}
		else if(WIFCONTINUED(status)) 
			update_ps_status(rc, "\0", RUNNING);
	}
}

void sigint_hdle(int sigop)
{

	update_ps_status(current_process, name_ps, STOPPED);
	kill(current_process, SIGINT);
}

void sigtstp_hdle(int sigop)
{
	update_ps_status(current_process, name_ps, PAUSED);
	kill(current_process, SIGTSTP);

}

void sigttin_hdle(int sigop)
{

	kill(current_process, SIGTTIN);
}

void sigcont_hdle(int sigop)
{

	update_ps_status(current_process, name_ps, RUNNING);
	kill(current_process, SIGCONT);
}

void execNintern(char **cmd_args, int nb_args)
{
	char path[LENGTH];	
	int pid, nohup=0;

	memset(path,0,sizeof(path));
	memset(name_ps,0,sizeof(name_ps));

	strcpy(name_ps, cmd_args[0]);

	if( !strcmp(cmd_args[nb_args - 1], "&") ){
		cmd_args[nb_args - 1] = 0; 
		nohup = 1;
	}

	find_path(cmd_args[0], path); 

	pid = fork();

	if (pid == -1) 
	{ 
		printf("\nFailed forking child..\n"); 
		return; 
	} 
	else if (pid == 0) 
	{ 
		if (execvp(path, cmd_args) < 0)
			printf("\nCould not execute command..\n"); 		
	} 
	else 
	{
		procs[curr_procs_nb].pid = pid;
		procs[curr_procs_nb].ppid = getpid();
		procs[curr_procs_nb].status = RUNNING;
		strcpy(procs[curr_procs_nb].name, cmd_args[0]);
		curr_procs_nb++;

		current_process = pid;

		if( nohup )
		{			
		}
		else
		{ //waitpid(pid,&wstatus,0);
			//printf("test\n");
		}
	} 
}


void execute(char **cmd_args, int nb_args)
{
	int is_intern = InternCmd(cmd_args[0]);
	
	if(is_intern != -NINTERN)
		execIntern(is_intern, cmd_args);
	else
		execNintern(cmd_args, nb_args);	
}




