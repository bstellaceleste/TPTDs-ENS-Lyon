/*Our minishell library : minishell.h */
#define NB_CMDS 10
#define NB_PROCESS 10
#define NB_ENV_VAR 10

#define INPUT_LENGTH 100
#define LENGTH 50

#define NINTERN 1

int current_process;

struct process {
	int pid;
	int ppid; //pid of the parent
	int status;
	char name[LENGTH];
};

struct env_var {
	char name[LENGTH];
	char value[LENGTH];
};


/**** Global variables ****/
struct env_var env[NB_ENV_VAR];
struct process procs[NB_PROCESS];

/********************************/
void init_shell();

void ms_help();

void ms_exit();

void ms_ps();

void ms_export(char **path_value); 

void ms_env();

void ms_kill(char **cmd_args);//can have in a function void func(const char *tab[])
/*******************************/
int readInput(char *input);

int parseInput(char *input, char **cmd_args);

void execute(char **cmd_args, int nb_args);
/******************************************/

void sigchld_hdle(int sigop);

void sigint_hdle(int sigop);//ctrl+c

void sigtstp_hdle(int sigop);//ctrl+z

void sigtstp_hdle_shell(int sigop);//ctrl+z

void sigttin_hdle(int sigop);//bg

void sigcont_hdle(int sigop);//fg

/*	
	if(!strcmp(path,"Not found"))
	{
		printf("\nWrong command\n");
		ms_help();
		return;
	}
else 
		{	printf("ccc\n");			
			printf("ttt%s\n", path);//path = "Not found";
			break;
		}*/


