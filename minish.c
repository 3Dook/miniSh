// AUTHOR: DUC DOAN

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
// Strings
#include <string.h>
//USE for signals
#include <signal.h>
//Used for open and write
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*****************************************
GLOBAL VARIABLE TO MANAGE BACK/FORE GROUND LOCK
unable to figure out how to pass additional arguments
to catch and update variables in SIG Catch
- Piazza - instructor did not state any better methods.
*****************************************/
int backgroundLock = 0;

/*********************************************
FUNCTION TO CHECK AND PRINT OUT STATUS OF
EXIT termination.
*********************************************/
void checkExitStatus(int childExitMethod){

    //based on lecture slide 26 
    if(WIFEXITED(childExitMethod)){
        //process exited normally, aka ran to completion
        printf("exit value %d\n", WEXITSTATUS(childExitMethod));
        fflush(stdout);
    }
    else{
        //process was exited by a signal. 
        printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
        fflush(stdout);
    }

}

// ATTEMPT TO DO THIS. SIGNAL HANDLING.
// CTRL C will be disable in main, background, unlock only in foreground
// CTRL Z will lock switch between background and foreground, processes
//lecture slide 109
void catchSIGTSTP(int signo){
//  THIS FUNCTION will be used by CTRL-Z to toggle and switch the global 
// variable so that we can lock the foreground and background processes.
    if(backgroundLock == 0){
        char* message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        fflush(stdout);
        backgroundLock = 1;
    }
    else{
        char* message = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        backgroundLock = 0;
    }
}



/*******************************************************
This function will be used to get input and update key variables 
based on the cmd line of the user. 
*******************************************************/

void getCmdLine(char* inputArry[], int* numArg, int pid, int* backArry, char* inputName, char* outputName){
    
    //we we will first get user input from the cmd line
    // max amount of character is 2048
    char cmdLine[2048];
    int i;
    for(i=0; i < 2048; i++){
        cmdLine[i] = '\0';
    }

    // we every new command we will reset the background to zero
    backArry[0] = 0;

    //send to the user the input prompt get input
    printf(": ");
    fflush(stdout);
    fgets(cmdLine, 2048, stdin);
    //printf("YOU WROTE %s", cmdLine);
    //inputArry[0] = cmdLine;
    //printf("singleChar %c\n", cmdLine[1]);
    
    i = strcspn(cmdLine, "\n");
    //printf("newline count is %d\n", i);

    int start, tempCount, inputTracker;
    int final = i;
    start = 0;
    tempCount = 0;
    inputTracker = 0;
    char temp[2048];
    //memset(temp, '\0', sizeof(temp));
    //flags to properly log and catch if we need to add to out or in names
    int inFlag, outFlag;
    inFlag = 0;
    outFlag = 0;

    // note to self, this will iterate and transfer only useful character into temp.
    // then will load temp completely into our command argument array.
    while(start != final){
        memset(temp, '\0', sizeof(temp));
        while(cmdLine[start] != '\n'){
            //printf("HELLO - %c\n", cmdLine[start]);

            if(cmdLine[start] == ' '){
                start = start + 1;
                //move on to the next spot
                break;
            }
            else if(cmdLine[start] == '$' && cmdLine[start+1] == '$'){
                // we need to expand the process ID,
                // we add start twice because we already check the next one to be $.
                 start = start + 1;
                 start = start + 1;
                 char strBuff[256];
                 sprintf(strBuff, "%d", pid);
                 //printf("THE $$ expanded to %s", strBuff);
                 int k;
                 for(k=0; k < strlen(strBuff); k++){
                     temp[tempCount] = strBuff[k];
                     tempCount = tempCount + 1;
                 }
            }
            else{
                temp[tempCount] = cmdLine[start];
                start = start + 1;
                tempCount = tempCount + 1;  
            }
           
        }
        //printf("NOW ENTERING %s\n", temp);

        // weird bugs
        if(strcmp(temp, "&") == 0){
            backArry[0] = 1;
            inputTracker = inputTracker - 1;
        }
        // we will not add the <> into the inputArry but will add the other values respectively
        else if(strcmp(temp, "<") == 0){
            inFlag = 1;
            inputTracker = inputTracker - 1;
        }
        else if(strcmp(temp, ">") == 0){
            outFlag = 1;
            inputTracker = inputTracker - 1;
        }
        else{

            if(inFlag == 1){
                //*inputName = strdup(temp);
                strcpy(inputName, temp);
                inFlag = 0;
            }
            else if(outFlag == 1){
                //*outputName = strdup(temp);
                strcpy(outputName, strdup(temp));
                inFlag = 0;
            }
            else{
                inputArry[inputTracker] = strdup(temp);
            }
        }
        //memcpy(inputArry[inputTracker], temp[tempCount], tempCount);
        //printf("array now has %s\n", inputArry[inputTracker]);
        // reset of temp
        tempCount=0;
        inputTracker = inputTracker + 1;
    }

    
    //return number of arg into the number of arguments
    *numArg = inputTracker;
    //printf("THIS IS INPUT - %s AND THIS IS OUTPUT %s\n", inputName, outputName);
}

//run the command, and potentially spawn and fork child processess.
void runExecute(char* inputArry[], int numArg, int* backArry, char* inputName, char* outputName, int* childExitMethod, struct sigaction signalACT){
    pid_t spawnpid = -5;

    // will update maybe later
    //int childExitMethod = -5;

    // use this for file handling
    int sourceFD, targetFD, result;
    fflush(stdout);


    spawnpid = fork();
    switch(spawnpid){
        case -1:
            perror("ERROR HULL BREACH!\n");
            fflush(stdout);
            exit(1);
            break;
        case 0:
            //child process;

            // to get it to terminal only when run on the foreground, we will use the background
            //Switch to control the method.
            if(backArry[0] == 0){
                // NO BACKGROUND.
                // reset lec 104
                // The foreground child process must handle its own termination.
                // note that exec will remove any special handlers and thus, we'll keep it to the default.
                //printf("FOREGROUND MODE ON - CTRL - C reactivated");
                //fflush(stdout);
                signalACT.sa_handler = SIG_DFL;
                sigaction(SIGINT, &signalACT, NULL);
            }

            //set up out and in file streams 
            // from lecture 3.4
            if(strlen(inputName) > 0){
                // open the input file name,
                // reassign
                // add a trigger close
                //printf("HAPPENING\n");
                sourceFD = open(inputName, O_RDONLY);
                if(sourceFD == -1){
                    perror("cannot open file for input");
                    exit(1);
                }
                result = dup2(sourceFD, 0);
                if(result == -1){
                    perror("cannot assign file for input");
                    exit(2);
                }
                fcntl(sourceFD, F_SETFD, FD_CLOEXEC);

            }

            if(strlen(outputName) > 0){
                // open the input file name,
                // reassign
                // add a trigger close
                //printf("HAPPENING\n");
                targetFD = open(outputName,  O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(targetFD == -1){
                    perror("cannot open file for output");
                    exit(1);
                }
                result = dup2(targetFD, 1);
                if(result == -1){
                    perror("cannot assign file for output");
                    exit(2);
                }
                fcntl(targetFD, F_SETFD, FD_CLOEXEC);

            }

            //https://linux.die.net/man/3/execvp
            // only returns -1 if error, if success then 0, hence why we will run an if statement to check
            //printf("%s\n", inputArry[0]);
            // THIS IS FROM 3.4
            if(execvp(inputArry[0], inputArry)){
                //in case of bad files, and flush it out to make sure no issues.
                printf("%s: no such file or directory\n", inputArry[0]);
                fflush(stdout);
                exit(1);
            };
 
            break;
        default:
            //https://linux.die.net/man/2/waitpid
            // we will run a process in the background if there is an & and if we are allowed to with allowedbackground - 
            //if(backArry[0] && backArry[1]){
            // wasn't able to edit the CTRL-Z without using a global variable.
            // hence why we are using the global variable
            if(backArry[0] && !backgroundLock){
                waitpid(spawnpid, childExitMethod, WNOHANG);
                //print out the background ID
                printf("background pid is %d\n", spawnpid);
                fflush(stdout);
            }
            else{
                //wait for child - AKA FOREGROUND
                waitpid(spawnpid, childExitMethod, 0);
            }
    }
    // CHECK FOR ONGOING BACKGROUND CHILDREN
    // WILL OCCUR BY THE PARENT, ONCE DONE.
    //https://www.geeksforgeeks.org/exit-status-child-process-linux/
    while((spawnpid = waitpid(-1, childExitMethod, WNOHANG)) > 0){
        printf("background pid %d is done: ", spawnpid);
        checkExitStatus(*childExitMethod);
        fflush(stdout);
    }


}


void main(){

    //inputArry of 512 arguments
    char* inputArry[512];
    //input and output files
    char inputName[256] = "";
    char outputName[256] = "";

    //memset(inputArry, '\0', sizeof(inputArry));
    //Number of arguments in the array,
    int numArg;
    // pid for the process to expand on
    int pid = getpid();
    // status and flags
    int exitStatus = 0;
    // background flag;
    // first spot means to run command in background
    // second spot means if we can run the command in the background.
    // unable to figure out how to fix and update array with signal handlers without
    //using a global variable, it will be left there for now.
    int backgroundArray[] = {0, 1};


    /***********************************************************************
    SIGNAL HANDLERS - CTRL - C DISABLE in main, we can can ignore it, and 
    renable them in foreground child. THIS DOES NOT TERMINATE THE BACKGROUND.
    THEY WILL HANDLE THEMSELVES or exti when shell exit.
    CTRL - Z, add a catch method to toggle the background.
    We will set Z in the begining and leave it just like that.
    GIVEN FROM slide 109 and Lecture.
    ***********************************************************************/
    //CTRL
    struct sigaction SIGINT_action = {0};
    //IGNORE - LEC 109
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_handler = SIG_IGN;
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);
    // reset lec 104
    //SIGINT_action.sa_handler = SIG_DFL;
    //sigaction(SIGINT, &SIGINT_action, NULL);

    //CTRL-Z - immediately set it to the handler.
    struct sigaction SIGTSTP_action = {0};
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while(1){
        //pre and reset names and variables
        strncpy(inputName, "", sizeof(inputName));
        strncpy(outputName, "", sizeof(outputName));
        fflush(stdout);
        memset(inputArry, '\0', sizeof(inputArry));
        getCmdLine(inputArry, &numArg, pid, backgroundArray, inputName, outputName);
        //printf("WHILE IN MAIN THIS IS TO KEEP TRACK THIS IS INPUT - (%s){%d} AND THIS IS OUTPUT (%s){%d}\n", inputName, strlen(inputName), outputName, strlen(outputName));

/*
        printf("checking array for target, at number %d\n", isThere(inputArry, ">", numArg));
        if(isThere(inputArry, ">", numArg) > -1){
            printf("TRUE there is an >\n");
        }
*/
        //printf("number of argument %d\n", numArg);
        // FOR DEBUGGING PURPOSES
        //printf(inputArry[0]);
        /*
        printf("CONTENT OF INPUTARRAY ARE with %d arguments..\n", numArg);
        int i;
        for(i = 0; i < numArg; i++){
            printf("inputARRAY[%d] - %s\n", i, inputArry[i]);
        }

*/
        
        //Blank Line and comment line check
        if(numArg == 0 || strcmp(inputArry[0], "#") == 0 || strcmp(inputArry[0], "") == 0 ){
            continue;
        }
        // Exit statement
        else if (strcmp(inputArry[0], "exit") == 0){
            exit(0);
        }
        // CD statement
        else if (strcmp(inputArry[0], "cd") == 0){
            //char cwd[256];
            //getcwd(cwd, sizeof(cwd));
            //printf("you are currently in %s\n", cwd);
            
            //https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/
            if(numArg == 1){
                chdir(getenv("HOME"));
            }
            else{
                // this will move around in the directory based on where you are currently in.
                // chDir will return 1
                int check = chdir(inputArry[1]);
                if(check == -1){
                    //error
                    printf("INVALID DIRECTORY\n");
                }

            }
        }
        //STATUS
        else if (strcmp(inputArry[0], "status") == 0){
            //call function to check the most recent exitStatus of a foreground background.
            // if it hasn't had one it will stay with the default set one we made during initalization.
            checkExitStatus(exitStatus);

        }
        else{
            // for now because, to ensure i am doing this right.
            runExecute(inputArry, numArg, backgroundArray, inputName, outputName, &exitStatus, SIGINT_action);
        } 

    }

}

