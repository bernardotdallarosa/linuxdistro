#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYSCALL_LOGPRINT 387

int main(int argc, char **argv) {
    long ret;

    if (argc < 2) {
        printf("Uso: %s <mensagem>\n", argv[0]);
        return 1;
    }

    printf("Invoking 'logPrint' system call.\n");
    ret = syscall(SYSCALL_LOGPRINT, argv[1]);

    if (ret == 0) {
        printf("Sucesso. Veja a mensagem com 'dmesg | tail'.\n");
    } else {
        printf("Syscall error, ret=%ld\n", ret);
    }

    return 0;
}