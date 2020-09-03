/* Kernel Programming
 * #define MODULE
 * #define LINUX
 * #define __KERNEL__
 */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/module.h>/* Needed by all moudle */
#include <linux/moduleparam.h>/* Needed by module param function */
#include <linux/init.h>/* Needed for the macros- module_init\exit */
#include <linux/unistd.h>//allowing acess to the carnel DB
#include <linux/utsname.h>//utsname
#include <linux/errno.h>

static char *program_name = "Bill";
MODULE_PARM(program_name, char*);


MODULE_LICENSE("GPL");

//void** sys_call_table = arch/x86/entry/syscall_64.c;

void** sys_call_table = NULL;


asmlinkage long (*original_syscall)(int pid, int sig);

asmlinkage long our_sys_kill(int pid, int sig){//TODO: complete the function

    struct task_struct* ptr = find_task_by_pid(pid);
    if (ptr == NULL) {
        return original_syscall(pid, sig);
    }
    if (program_name == NULL || ptr->comm == NULL) {
        return original_syscall(pid, sig);
    }
    if (strncmp(program_name, p->comm, 16)==0 && sig == SIGKILL) {
        return -EPERM;
    }
    return original_syscall(pid, sig);

}




void allow_rw(unsigned long addr){//verify what param this function gets.
    unsigned int level;
    pte_t *ptr_e = lookup_address(addr,&level);
    if(ptr_e->pte & ~_PAGE_RW){
        ptr_e->pte |= _PAGE_RW;
    }
}

/*
turns off the R/W flag for addr.
*/
void disallow_rw(unsigned long addr) {
    unsigned int level;
    pte_t *ptr_e = lookup_address(addr,&level);
    ptr_e->pte =ptr_e->pte & ~_PAGE_RW;

}

void plug_our_syscall(void){
    original_syscall=(typeof(sys_kill) *)sys_call_table[__NR_kill];//    original_syscall=(typeof(sys_kill) )sys_call_table[__NR_read];
    allow_rw((unsigned long)sys_call_table);
    sys_call_table[__NR_kill]=our_sys_kill;

}

/*
This function updates the entry of the kill system call in the system call table to point to the original kill system call.
*/
void unplug_our_syscall(void){
    sys_call_table[__NR_kill]=original_syscall;
    disallow_rw((unsigned long)sys_call_table);
}

int kill_init(void){

    sys_call_table= (void*)kallsyms_lookup_name("sys_call_table");
    if(!sys_call_table){
        pr_err("Couldn't look up sys_call_table\n");
        return -1;
    }
    plug_our_syscall();
    return 0;
}


void kill_restore(void){
    program_name=NULL;
    unplug_our_syscall();

}



module_init(kill_init);
module_exit(kill_restore);



