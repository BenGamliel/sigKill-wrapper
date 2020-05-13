//
// Created by admin on 12/31/2019.
//

#ifndef OS3_KILL_WRAPPER_H
#define OS3_KILL_WRAPPER_H


#include <stdio.h>
//#include <linux/module.h>
//#include <dlfcn.h>
#include <sys/types.h>
#include <errno.h>

#define SYS_KILL (62)

int kill(pid_t pid,int sig) {
    printf("welcome to our kill wrapper!\n");
    int __res;
    __asm__(
    "syscall;"//instead of "int $0x80;"
    : "=a"(__res)//2 move the user parameters to the registers
    : "0" SYS_KILL,//3place the syscall number in rax
    "D" (pid), "S"(sig)
    : "memory");//4execute syscall instruction

    //5handle the syscall's return value
    if ((__res) < 0) {
        errno = (-__res);
        return -1;
    }
    return __res;

}

#endif //OS3_KILL_WRAPPER_H
