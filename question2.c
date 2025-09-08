#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>         // used to make system call
#include <sys/syscall.h>    // used to make system call
#include <sys/types.h>
#include <sys/wait.h>

#define MY_SYSCALL_NUM 449

void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(MY_SYSCALL_NUM, addr);

    if (paddr == 0) {
        printf("failure: cannot get physical address\n");
        return NULL;
    } else {
        printf("The virtual address %p ---> The physical address: %p \n", addr, paddr);
    }

    return paddr;
}

int a[2000000]; 

int main()
{ 
    int      loc_a;
    void     *phy_add;  

    a[0] = 10;

    phy_add=my_get_physical_addresses(&a[0]);
    printf("global element a[0]:\n");  
    printf("Offest of logical address:[%p]   Physical address:[%p]\n", &a[0], phy_add);              
    printf("========================================================================\n");

    a[1999999] = 20;

    phy_add=my_get_physical_addresses(&a[1999999]);
    printf("global element a[1999999]:\n");  
    printf("Offest of logical address:[%p]   Physical address:[%p]\n", &a[1999999], phy_add);              
    printf("========================================================================\n"); 
}
