#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

#define SYSCALL_PROCSLEEPING 386

int main(void) {
    char buf[4096];
    long ret;

    printf("Invoking 'procSleeping' system call.\n");
    ret = syscall(SYSCALL_PROCSLEEPING, buf, sizeof(buf));

    if (ret > 0) {
        printf("%s\n", buf);
    }
    else {
        printf("Syscall error %ld\n", ret);
    }

    return 0;
}