#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#define SHMKEY 849213           //custom key for shared memory
#define SIZE sizeof(int)        //size of each element in a shared memory array
#define MILLISEC 1000000        //value of 1 millisec in nano seconds

int num1;                       //variable for identifying process number
int num2;                       //variable for number to be checked for primality
int *clockSim;                  //shared memory pointer to clock simulator array
int *array;                     //shared memory pointer to prime array
int storedSec;                  //variable for storing starting seconds
int storedNan;                  //variable for storing starting nano seconds

void compareTime()          //function to check if process has gone over 1 millisecond of simulated time
{
    int currentSec = clockSim[0];
    int currentNan = clockSim[1];
    if ((currentSec - storedSec) > 0)                                       //check for available seconds to calculate milliseconds
    {
        currentSec--;
        currentNan = currentNan + 1000000000;
    }

    if(((currentSec - storedSec) == 0) && ((currentNan - storedNan) > MILLISEC))    //if over 1 millisecond of simulated time
    {
        array[num1] = -1;                                                   //sets the element of its process number to -1 because it timed out
        shmdt(clockSim);                                                    //detach clock simulator from shared memory
        shmdt(array);                                                       //detach array of primes from shared memory
        exit(0);
    }
}

int checkPrimality(int n)       //function to check a given integer for primality
{
    int i, isPrime = 1;

    for (i = 2; i <= n / 2; ++i)    //loop to check if given number is divisible by another
    {
        compareTime();          //function call to check if program is over 1 millisecond and terminate if it is
        if (n % i == 0)         //if not a prime number
        {
            isPrime = 0;        //set prime flag to false
            break;
        }
    }
    if (isPrime == 1)
    {
        return n;               //if number is a prime print itself
    } else {
        return -n;              //if number is not a prime print itself negative
    }
}

void storeTime()                //function to store the simulated clock start time
{
    storedSec = clockSim[0];    //stores simulated seconds
    storedNan = clockSim[1];    //stores simulated nano seconds
}


int main (int argc, char *argv[])
{
    int shmid1 = shmget (SHMKEY, (3*SIZE)+1, 0600 | IPC_CREAT);     //creates shared memory id for clock simulator
    if (shmid1 == -1)
    {
        perror("Shared memory:");                                   //if shared memory does not exist print error message and exit
        return 1;
    }

    clockSim = (int *)shmat(shmid1, 0, 0);                          //attaches to the shared memory clock simulator
    int n = clockSim[2];

    storeTime();

    int shmid2 = shmget(SHMKEY+1, (n*SIZE)+1, 0600 | IPC_CREAT);    //creates shared memory id for array of primes
    if (shmid2 == -1)
    {
        perror("Shared memory2:");                                  //if shared memory does not exist print error message and exit
        return 1;
    }

    array = (int *)shmat(shmid2, 0, 0);                             //attaches to the shared memory array of primes

    num1 = atoi(argv[0]);                                           //converts process number argument into int and copies into variable
    num2 = atoi(argv[1]);                                           //converts number to be checked for primality argument into int and copies into variable

    if(array[num1] != 0)                                            //checks if shared memory is not 0
    {
        fprintf(stderr, "Worker: 0 not found in shared memory.\n"); //error message if shared memory is not 0
        exit(1);
    }
    else
    {
        array[num1] = checkPrimality(num2);                         //calls function to find largest prime number and copies it into shared memory
    }

    shmdt(clockSim);                                                //detach clock simulator from shared memory
    shmdt(array);                                                   //detach array of primes from shared memory
    return 0;
}