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


/*
 * first mission:
 * 1) find the address of the system call table by using an internal private kernel function
 *
 * 2) having the adress of the system call table we would want to change the entry of the sys_kill() to
 * point to out own-wriitten function, but there is one more problem!
 */

/*
turns on the R/W flag for addr.

* second mission:
* the kernel protects itsself by turning off the R/W flag in(virtual) pages that contain static read-only structurs
* like our beloved system call table,therefore we will not be able to upadte it.
* modify the R\W permission by implementing allow_rw function,dont forget to disallow the R\W permissions when removing
* the module (i.e calling rrmod)
void allow_rw(unsigned long addr){
 */

/*
This function updates the entry of the kill system call in the system call table to point to our_syscall.
* 1)when our module is loaded(insmod),it will hijack the system call by pointing the ystem call table entry
* occupied by the original sys_kill pointer to our self-written function.
* 2)we will also have to save the original function pointer to be able to restore it later, when our module is unloaded
//(rmmod)
 */

/*
This function is called when loading the module (i.e insmod <module_name>)
* shelly might want to save programs other the "Bill" in the future,therefore the module will
 * depend on a command-line string argument called "program_name".
 *
 * example for a possible loading command for our module could be :
 * " insmod intercept.ko program_name"some_other_program" "
 *  program_name is optinal where the default value is set to "Bill"
 *
 *  Note: we want to block the signal SIGKILL and allow other signal to be sent to "Bill"
 */
