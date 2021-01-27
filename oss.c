#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#define SHMKEY 849213               //custom key for shared memory
#define SIZE sizeof(int)            //size of each element in a shared memory array
#define TOTALPROCESSLIMIT 12000     //limit for total processes allowed
#define CHILDLIMIT 19               //limit for total child processes allowed at once

/*
 * Project : Operating systems assignment 2
 * Author : Scott Tabaka
 * Date Due : 2/26/2020
 * Course : CMPSCI 4760
 * Purpose : Process numbers checking for primality using fork, exec, and shared memory.  Outputs numbers to an output file.
 */

int shmid1;                 //variable to store id for shared memory segment 1
int shmid2;                 //variable to store id for shared memory segment 2
int *clockSim;              //shared memory pointer to clock simulator array
int *array;                 //shared memory pointer to prime array
int activeChildren = 0;     //variable to keep track of active children
FILE *output;               //variable for file output
int n = 4;                  //variable for total processes allowed
int s = 2;                  //variable for total child processes allowed at once
int b = 2;                  //variable for where to start checking for primes
int increment = 1;          //variable for incrementing b to check additional numbers for primes
char o[255] = "output.log"; //array for storing name of output file to output results to
int hflag=0;                //flag for help option
int opt;                    //variable for checking options
static char usage[100];     //array for storing a usage message
static char error[100];     //array for storing a error message

void printHelp()            //function to print help info
{
    if (hflag == 1)         //will print if help option was selected
    {
        printf("%s\n\n", usage);
        printf("This program will search for prime numbers and output the results to a file.\n\n");
        printf("-h Print a help message and exit.\n");
        printf("-n Indicate the maximum total of child processes oss will ever create.(Default 4,Max %d)\n", TOTALPROCESSLIMIT);
        printf("-s Indicate the number of children allowed to exist in the system at the same time.(Default 2,Max %d)\n", CHILDLIMIT);
        printf("-b Start of the sequence of numbers to be tested for primality(Default 2)\n");
        printf("-i Increment between numbers that will be tested\n");
        printf("-o Output ﬁle name(Default \"output.log\")\n");

        exit(0);
    }
}

void checkForOverflow()         //function to check if an overflow will happen with current options and numbers
{
    long maxIntValue = 2147483647;
    long nLong = (long)n;
    long bLong = (long)b;
    long iLong = (long)increment;

    if(maxIntValue-((nLong*iLong)+bLong) < 0)       //if the integer variable used for prime checking will overflow print message and exit
    {
        fprintf(stderr, "%s Current options selected will cause an overflow condition\n", error);
        exit(EXIT_FAILURE);
    }
}

int isInteger(char* input)          //function to check if char array input is a valid integer or not
{
    int i, len = strlen(input);

    for (i = 0; i < len; ++i)       //loop for checking each character of the input char array
    {
        if (!isdigit(input[i]))     //check if character is a digit or not
        {
            return 0;               //if not a number
        }
    }
    return 1;                       //if all characters were numbers
}

int isValidInput(char* input,int min,int max)       //function to check if user option argument numbers are within the range required
{
    int temp = atoi(input);                         //convert char array into an int

    if(temp < min || temp > max)                    //makes sure input is within the min and max given
    {
        return 0;                                   //if input is not within range
    } else {
        return 1;                                   //if input is within range
    }
}

void validateInput(int min,int max)     //function to make sure option argument is a number and within a given range
{
    if(isInteger(optarg) == 0)          //function call to check if option argument is a number or not
    {
        fprintf(stderr, "%s Argument for option -%c \"%s\" is not a positive integer\n", error,opt,optarg);
        exit(EXIT_FAILURE);
    }

    if(min >= 0 && max >=0)             //will only check if both min and max are positive integers
    {
        if(isValidInput(optarg,min,max) == 0)   //function call to check option argument is within a given range
        {
            fprintf(stderr, "%s Argument for option -%c \"%s\" is out of range\n", error,opt,optarg);
            exit(EXIT_FAILURE);
        }
    }
}

void initializeMessages(char* str)      //function to set text for usage and error messages
{
    strcpy(error,str);
    strcat(error,": Error:");

    strcpy(usage,"Usage: ");
    strcat(usage,str);
    strcat(usage," [-h] | [-n integer] [-s integer] [-b integer] [-i integer] [-o filename]");
}

void incrementClock(int *sec, int *nan)         //function to increment clock simulator
{
    *nan = *nan + 5;                            //increments clock simulator by 5 nano seconds. (See README)
    if(*nan >= 1000000000)                      //if nano seconds are equal or greater than 1 second
    {
        *sec = *sec + 1;                        //increase seconds by 1
        *nan = *nan - 1000000000;               //decrease nano seconds by 1 second
    }
}

void printClock(char *str,int num)          //function to print clock simulator to output file
{
    if(num >= 0)
    {
        fprintf(output, "%-23s %-7d %d:%d%s", str, num, clockSim[0], clockSim[1], "\n");    //if second argument is 0 or greater
    } else {
        fprintf(output, "%-31s %d:%d%s", str, clockSim[0], clockSim[1], "\n");              //if second argument is less than 0
    }
}

void printArray()           //function to print shared memory array of primes/nonprimes/unknown
{
    fprintf(output, "%-12s%s", "***PRIMES***", "\n");
    fprintf(output, "%-12s%s%s", "Process #", "Results", "\n");

    int i;
    for(i=0;i<n;i++)                                        //loop for printing only prime numbers
    {
        if (array[i] > 0)
        {
            fprintf(output, "%-14d%d%s", i, array[i], "\n");
        }
    }
    fprintf(output, "%s", "\n");
    fprintf(output, "%-12s%s", "***NON-PRIMES***", "\n");
    fprintf(output, "%-12s%s%s", "Process #", "Results", "\n");
    for(i=0;i<n;i++)                                        //loop for printing only non-prime numbers
    {
        if (array[i] < -1)
        {
            fprintf(output, "%-14d%d%s", i, array[i], "\n");
        }
    }
    fprintf(output, "%s", "\n");
    fprintf(output, "%-12s%s", "***NOT PROCESSED***", "\n");
    fprintf(output, "%-12s%s%s", "Process #", "Results", "\n");
    for(i=0;i<n;i++)                                        //loop for printing everything that was not processed(0 for not processed/-1 for timeout in child)
    {
        if (array[i] == -1 || array[i] == 0)
        {
            fprintf(output, "%-14d%d%s", i, array[i], "\n");
        }
    }
}

void handle_sigint()                    //signal handler for interupt signal(Ctrl-C)
{
    printf("\nProgram aborted by user --> %s\n",o);                     //displays end of program message to user
    while (waitpid(-1, NULL, WNOHANG) > 0);                             //wait until all children have terminated
    while (activeChildren > 0);

    printClock("Program aborted by user - End of Program",-1);          //prints the end of program time in output file
    fprintf(output, "%s", "\n");                                        //prints a new line in output file
    printArray();                                                       //function call to print entire contents of array of primes to output file

    shmdt(clockSim);                                                    //detach clock simulator from shared memory
    shmdt(array);                                                       //detach array of primes from shared memory
    shmctl(shmid1,IPC_RMID,NULL);                                       //remove clock simulator shared memory segment
    shmctl(shmid2,IPC_RMID,NULL);                                       //remove clock simulator shared memory segment
    fclose(output);                                                     //close output file
    kill(getpid(),SIGTERM);                                             //kill current process
    exit(1);
}

void handle_sigalarm()          //signal handler for alarm signal(for time out condition)
{
    printf("\nTimeout - Ending program --> %s\n",o);                    //displays end of program message to user
    while (waitpid(-1, NULL, WNOHANG) > 0);                             //wait until all children have terminated
    while (activeChildren > 0);

    printClock("Timeout - End of Program",-1);                          //prints the end of program time in output file
    fprintf(output, "%s", "\n");                                        //prints a new line in output file
    printArray();                                                       //function call to print entire contents of array of primes to output file

    shmdt(clockSim);                                                    //detach clock simulator from shared memory
    shmdt(array);                                                       //detach array of primes from shared memory
    shmctl(shmid1,IPC_RMID,NULL);                                       //remove clock simulator shared memory segment
    shmctl(shmid2,IPC_RMID,NULL);                                       //remove clock simulator shared memory segment
    fclose(output);                                                     //close output file
    kill(getpid(),SIGTERM);                                             //kill current process
    exit(1);
}

void handle_sigchild()      //signal handler for child termination
{
    while (waitpid(-1, NULL, WNOHANG) > 0)          //handle each child that has terminated
    {
        activeChildren--;                           //decrement number of active children
        printClock("Process terminated",-1);        //print process terminated time in output file
    }
}


int main (int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);          //initialization of signals and which handler will be used for each
    signal(SIGALRM, handle_sigalarm);
    alarm(2);                               //will send alarm signal after 2 seconds
    signal(SIGCHLD, handle_sigchild);

    output = fopen(o, "w");                 //opens output file for writing(if file not present it will create file)
    initializeMessages(argv[0]);            //function call to set error and usage message text
    opterr = 0;                             //disables some system error messages(using custom error messages so this is not needed)

    while ((opt = getopt(argc, argv, "hn:s:b:i:o:")) != -1)		//loop for checking option selections
    {
        switch (opt)
        {
            case 'h':                                   //option h
                hflag = 1;
                break;
            case 'n':                                   //option n
                validateInput(1,TOTALPROCESSLIMIT);
                n = atoi(optarg);
                break;
            case 's':                                   //option s
                validateInput(1,CHILDLIMIT);
                s = atoi(optarg);
                break;
            case 'b':                                   //option b
                validateInput(-1,-1);
                b = atoi(optarg);
                break;
            case 'i':                                   //option i
                validateInput(-1,-1);
                increment = atoi(optarg);
                break;
            case 'o':                                   //option o
                strcpy(o, optarg);
                break;
            case '?':                                   //check for arguments
                if (optopt == 'n' || optopt == 's' || optopt == 'b' || optopt == 'i' || optopt == 'o')
                {
                    fprintf(stderr, "%s Option -%c requires an argument.\n", error, optopt);
                } else {
                    fprintf(stderr, "%s Unknown option character '-%c'\n", error, optopt);
                }
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "%s\n", usage);
                exit(EXIT_FAILURE);
        }
    }

    checkForOverflow();         //function call to check for overflow condition
    printHelp();                //function call to print help message

    printf("Program has started processing primes\n");

    shmid1 = shmget(SHMKEY, (3*SIZE)+1, 0600 | IPC_CREAT);     //creates shared memory id for clock simulator
    if (shmid1 == -1)
    {
        perror("Shared memory1:");                           //if shared memory does not exist print error message and exit
        return 1;
    }

    clockSim = (int *)shmat(shmid1, 0, 0);                  //attaches to the shared memory clock simulator

    shmid2 = shmget(SHMKEY+1, (n*SIZE)+1, 0600 | IPC_CREAT);     //creates shared memory id for array of primes
    if (shmid2 == -1)
    {
        perror("Shared memory2:");                           //if shared memory does not exist print error message and exit
        return 1;
    }

    array = (int *)shmat(shmid2, 0, 0);                     //attaches to the shared memory array of primes

    clockSim[0] = 0;                                        //initialize clock simulator seconds to 0
    clockSim[1] = 0;                                        //initialize clock simulator nano seconds to 0
    clockSim[2] = n;                                        //uses the clock simulator array to store the size needed for shared memory array of primes
    int* c0 = &clockSim[0];                                 //creates a pointer to the clock simulator to be used in the increment clock function
    int* c1 = &clockSim[1];

    int i;
    for(i=0; i<n; i++)                                      //initializes all elements of the array of primes to 0
    {
        array[i] = 0;
    }

    int process=0;                                          //variable for identifying which process is being handled

    while(process < n)                                      //while current process number is less than the total number of processes
    {
        if(activeChildren < s)                              //if number of active children is less than the max number of allowed child processes at once
        {
            activeChildren++;                                           //increments the number of active children
            printClock("Launching child process", process);             //prints process start time in output file
            int pid = fork();                                           //fork call
            if (pid == 0)                                               //child process
            {
                char str[20];
                snprintf(str, sizeof(str), "%d", process);              //copies process number into a char array
                char str2[20];
                snprintf(str2, sizeof(str2), "%d", b);                  //copies current number to be checked for primality into a char array
                execl("prime", str, str2, NULL);                        //exec call with 2 arguments(process number,number to be checked for primality)
            }
            process++;                                                  //increment process id
            b = (b + increment);                                        //increments number to be checked for primality with increment
        }
        incrementClock(c0,c1);                                          //function call to increment clock
    }

    while(activeChildren > 0);                                          //program will wait here to make sure all active child processes have been terminated

    printClock("End of Program",-1);                                    //prints the end of program time in output file
    fprintf(output, "%s", "\n");                                        //prints a new line in output file
    printArray();                                                       //function call to print entire contents of array of primes to output file

    shmdt(clockSim);                                                    //detach clock simulator from shared memory
    shmdt(array);                                                       //detach array of primes from shared memory
    shmctl(shmid1,IPC_RMID,NULL);                                       //remove clock simulator shared memory segment
    shmctl(shmid2,IPC_RMID,NULL);                                       //remove clock simulator shared memory segment
    fclose(output);                                                     //close output file

    printf("Program completed successfully --> %s\n",o);                         //completed message for user

    return 0;
}