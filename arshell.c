#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_HISTORY_SIZE 10 /*history list size*/

static char historyList [MAX_HISTORY_SIZE][MAX_LINE];
static int historyListCount = 0;
static int runningCount = 0;

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

/* the signal handler function */
void handle_SIGTSTP() {

	//print 10 most recent commands
	write(STDOUT_FILENO, "\n", 1);
	
	int numCommandsToPrint = (historyListCount < MAX_HISTORY_SIZE) ? historyListCount : MAX_HISTORY_SIZE;
	int holdRunningCount = runningCount;
	
	for (int i = numCommandsToPrint - 1; i >= 0; i--) {
		char index[20];
		sprintf(index, "[%d]", holdRunningCount--);
		write(STDOUT_FILENO, index, strlen(index));
		write(STDOUT_FILENO, historyList[holdRunningCount % MAX_HISTORY_SIZE], strlen(historyList[holdRunningCount % MAX_HISTORY_SIZE]));
		write(STDOUT_FILENO, "\n", 1);
	}
	write(STDOUT_FILENO, "\n", 1);
}

void addCommandToHistoryList (char *args[]) {

	//consturct the command from the args array
	char command[MAX_LINE];
	command[0] = '\0';
	
	for (int i = 0; args[i] != NULL; i++) {
		strcat (command, args[i]);
		strcat (command, " ");
	}
	
	//remove trailing space
	if (strlen(command) > 0) {
		command[strlen(command) - 1] = '\0';
	}
	
	
	if (historyListCount == MAX_HISTORY_SIZE) {
		for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) 
			strcpy(historyList[i], historyList[i+1]);
			
		strcpy(historyList[MAX_HISTORY_SIZE - 1], command);
	}
	else {
		strcpy(historyList[historyListCount], command);
		historyListCount++;
	}
	runningCount++;
}
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
   
    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) {
        switch (inputBuffer[i]){
          case ' ':
          case '\t' :               /* argument separators */
            if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;
          case '\n':                 /* should be the final char examined */
            if (start != -1){
                    args[ct] = &inputBuffer[start];    
                ct++;
            }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
            break;
          default :             /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&'){
                *background  = 1;
                start = -1;
                inputBuffer[i] = '\0';
            }
          }
     }   
     args[ct] = NULL; /* just in case the input line was > 80 */
}


int main(void)
{
    char inputBuffer[MAX_LINE];  /* buffer to hold the command entered */
    int background;              /* equals 1 if a command is followed by '&' */
    int count = 0;		 /* used for the number of prompts */
    pid_t fork_rv;		 /* used for return value from fork*/
    char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */
    
    
    /*Print greeting message*/
    printf("Welcome to arshell. My pid is: %d\n", getpid());
    
    /* set up the signal handler */
    struct sigaction handler;
    handler.sa_handler = handle_SIGTSTP;
    handler.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &handler, NULL);
    
    while (1){            /* Program terminates normally inside setup */
       background = 0;
       
       /* Shell prompt w/ number of prompts */
       printf("arshell[%d]: ", ++count);
       fflush(stdout);

       
       setup(inputBuffer,args,&background);       /* get next command*/
       
       //check if user wants to repeat command
       if (strcmp(args[0],"r") == 0) {
		
        	//hold number of command in history list to repeat (defaulted to most recent)
        	int numCommand = historyListCount - 1;
        	 
        	//check if user wants to repeat a certain command
        	if (args[1] != NULL) {
			//convert number to repeat to an integer subtract one to accomodate for indexing
			numCommand = atoi(args[1]) - 1;
		}
		//check its valid	
		if (numCommand >= 0 && numCommand <= historyListCount-1) {
		
			//print command that will be repeated:
			printf("arshell[%d]: %s\n", count, historyList[numCommand]);
			
			//make a copy of the historyList command
			char historyCopy [MAX_LINE];
			strcpy(historyCopy, historyList[numCommand]);
			
			//replace args with the command from historyList
			char *token = strtok(historyCopy, " ");
			
			int i;
			for(i = 0; token != NULL; i++) {
				args[i] = token;
				token = strtok(NULL, " ");
			}
			args[i] = NULL;
		}
        }
       //INTERNAL COMMANDS
       //check if yell command
       if (strcmp(args[0],"yell") == 0) {
       		
       		//iterate though the args after yell up until theres no more args
       		for (int i = 1; args[i] != NULL; i++) {
       			//iterate through the string character by character up until the null terminator
       			for (int j = 0; args[i][j] != '\0'; j++) {
       				args[i][j] = toupper(args[i][j]);
       			}
       			
       			//print the uppercased string now
       			printf("%s ", args[i]);
       		}
       		//add a new line to make it look good
       		printf("\n");
       }
       //check if exit command
       else if (strcmp(args[0],"exit") == 0) {
       		
       		//invoke the ps command using the system() function call
       		system("ps -p $$ -o pid,ppid,%cpu,%mem,etime,user,command");
       		
       		//exit the shell
       		exit(0);
       			
       }
       //else fork hehe
       else {
	       //FORKING
	       //fork a new process
	       fork_rv = fork();
       			
	       //check for errors
	       if (fork_rv == -1)
	       		perror("Fork has failed.\n");
	       //execution from child process
	       else if (fork_rv == 0)
       			//execute
	       		execvp(args[0], args);
	       //waits
	       else {
	       		//if background == 0, the parent will wait
	       		if (background == 0) {
	       			//print message indicating pid of child and if it's running in the foreground
	       			printf("[Child PID = %d, background = False]\n", fork_rv);
	       			
	       			
	       			//wait for child process to terminate
	       			waitpid(fork_rv, NULL, 0);
	       			
	       			//print message that child process is complete
	       			printf("Child process complete.\n\n");
	       		}
	       		//otherwise returns to the setup() function
	       		else
	       			//print message indicating pid of child and if it's running in the background
	       			printf("[Child PID = %d, background = True]\n", fork_rv);
	       	}
	  }
	  
	//add command to history list
	addCommandToHistoryList(args);
    }
}
