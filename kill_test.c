#include "kill_wrapper.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define MIN_LEN 6
#define MAX_LEN 16

#define FAIL_ERRVAL(testcase, exp, act) \
    printf("Error in test %s, expected error %s, and got %s\n", #testcase, name_error(exp), name_error(act));

#define FAIL_RETVAL(testcase) \
    printf("Error in test %s, expected retval < 0, got >= 0\n", #testcase);

#define CONST_NAME_CASE(name) \
        case name: \
            return #name

/*
    This tests file can test 7 cases:
    1. The pid doesn't exist (errno = ESRCH)
    2. The pid exists and isn't equal to program_name's pid:
        a. The signal is invalid (errno = EINVAL)
        b. The signal is valid and is not SIGKILL (SUCCESS)
        c. The signal is valid and is SIGKILL (SUCCESS)
    3. The pid is of program_name:
        a. The signal is invalid (errno = EINVAL)
        b. The signal is valid and is not SIGKILL (SUCCESS)
        c. The signal is valid and is SIGKILL (errno = EPERM)
    
    These cases are also used to assert that the kill syscall is
    correct before the module is inserted and after the module 
    is removed.
    It also tests the actual signal sending effects.

*/

char program_name[MAX_LEN + 1];

void rand_name(char* name, int min, int max);
char* name_error(int error);

void sighandler(int sig);

void testcase1();
void testcase2_3(bool is_module_in, bool is_prog_name);

int main(int argc, char** argv)
{
    signal(SIGFPE, sighandler);
    signal(SIGUSR1, sighandler);

    char buffer[1024];

    if(argc == 1)
    {
        printf("Welcome to the Kill Bill module test program!\n");

        srand(time(NULL)); // Initialize random.
        rand_name(program_name, MIN_LEN, MAX_LEN + 1);

        // create the executables.
        system("gcc -o test kill_test.c");
        sprintf(buffer, "gcc -o %s kill_test.c", program_name);
        system(buffer);

        // before module is in:
        testcase1();
        testcase2_3(false, false);
        testcase2_3(false, true);

        // insmod
        sprintf(buffer, "sudo insmod ./intercept.ko program_name=%s", program_name);
        printf("running: %s\n", buffer);
        system(buffer);
        // module is in:
        testcase1();
        testcase2_3(true, false);
        testcase2_3(true, true);
        printf("running: sudo rmmod intercept.ko\n");
        system("sudo rmmod intercept.ko");

        // after module is out:
        testcase1();
        testcase2_3(false, false);
        testcase2_3(false, true);

        system("sudo rm test");
        sprintf(buffer, "sudo rm %s", program_name);
        system(buffer);

        printf("KIll Bill test program is finished.\n");
    }
    else if(strcmp("sig_receiver", argv[1]) == 0)
    {
        while(1) ;
    }
    else
    {
        printf("Error: Invalid parameters.\n");
    }

    return 0;
}

void rand_name(char* name, int min, int max)
{
    int len = rand() % (max - min) + min;

    for (int i = 0; i < len; i++)
    {
        name[i] = rand() % ('z' - 'a') + 'a';
    }

    name[len] = 0;
}

char* name_error(int error)
{
    static char def[32];

    switch(error)
    {
        CONST_NAME_CASE(ESRCH);
        CONST_NAME_CASE(EINVAL);
        CONST_NAME_CASE(EPERM);
        default:
            sprintf(def, "%d (NAME NOT FOUND)", error);
            return def;
    }
}

void sighandler(int sig)
{
    if(sig == SIGFPE)
    {
        char tosend = 42;
        write(fileno(stdout), &tosend, 1);
    }
    else if(sig == SIGUSR1)
    {
        exit(1);
    }
}

void testcase1()
{
    if(kill(0xfeeffeef, SIGUSR1) < 0)
    {
        if(errno != ESRCH)
        {
            FAIL_ERRVAL("pid doesn't exist", ESRCH, errno);
        }
    }
    else
    {
        FAIL_RETVAL("pid doesn't exist");
    }
}

void testcase2_3(bool is_module_in, bool is_prog_name)
{
    int pid;
    int pipes[2];

    pipe(pipes);
    pid = fork();


    if(pid > 0)
    {
        sleep(2);

        char torecv;

        close(pipes[1]);

        // case 2/3a
        if(kill(pid, 342) < 0)
        {
            if(errno != EINVAL)
            {
                FAIL_ERRVAL("signal not exist", EINVAL, errno);
            }
        }
        else
        {
            FAIL_RETVAL("signal not exist");
        }

        // case 2/3b
        if(kill(pid, SIGFPE) < 0)
        {
            perror("Error in calling kill (test case 2b)\n");
        }
        read(pipes[0], &torecv, 1);
        if(torecv != 42)
        {
            printf("Error in signal sending (test case 2b)\n");
        }

        // case 2/3c
        if(kill(pid, SIGKILL) < 0)
        {
            if(is_module_in && is_prog_name)
            {
                if(errno != EPERM)
                {
                    FAIL_ERRVAL("sending sigkill", EPERM, errno);
                }
            }
            else
            {
                perror("Error in test case \"sending sigkill\", received");
            }
        }
        else
        {
            if(is_module_in && is_prog_name)
            {
                printf("Test case sending sigkill: module didn't protect\n");
            }
        }

        sleep(1);

        int stat;
        if(waitpid(pid, &stat, WNOHANG) == 0)
        {
            if(is_module_in)
            {
                if(!is_prog_name)
                {
                    printf("Test case sending sigkill: module didn't send sigkill when it should've\n");
                }
                else
                {
                    // case of protection, sending sigusr1.
                    if(kill(pid, SIGUSR1) < 0)
                        perror("Unexpected kill error");
                    if(waitpid(pid, &stat, 0) < 0)
                        perror("unexpected waitpid error");
                }

            }
            else
            {
                printf("Test case sending sigkill: signal wasn't failed without module - cleanup failed (maybe in previous tests)\n");
            }
        }
    }
    else if(pid == 0)
    {

        dup2(pipes[1], fileno(stdout));
        close(pipes[0]); close(pipes[1]);

        if(is_prog_name)
        {
            char* argv[3];
            argv[0] = program_name;
            argv[1] = "sig_receiver";
            argv[2] = 0;
            execv(program_name, argv);
            perror("error in execv");
        }
        else
        {
            char* argv[3];
            argv[0] = "test";
            argv[1] = "sig_receiver";
            argv[2] = 0;
            execv("test", argv);
            perror("error in execv");
        }
    }
}