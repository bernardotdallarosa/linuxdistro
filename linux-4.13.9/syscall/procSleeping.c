#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include "procSleeping.h"

#define SLEEP_BUF_MAX 4096

asmlinkage long sys_procSleeping(const char __user *buf, int size) {
    struct task_struct *proces;
    char *kbuf;
    char line[256];
    int used = 0;
    int linelen;
    int bufsz;
    int ret;

    kbuf = kmalloc(SLEEP_BUF_MAX, GFP_KERNEL);
    if (!kbuf)
        return -1;

    for_each_process(proces) {
        if (proces->state == TASK_INTERRUPTIBLE || proces->state == TASK_UNINTERRUPTIBLE) {

            snprintf(line, sizeof(line), "Process: %s\n PID_Number: %ld\n Process State: %ld\n",
                    proces->comm,
                    (long)task_pid_nr(proces),
                    (long)proces->state);

            linelen = strlen(line);

            if (used + linelen + 1 > SLEEP_BUF_MAX)
                break;

            memcpy(kbuf + used, line, linelen);
            used += linelen;
        }
    }

    kbuf[used] = '\0';
    bufsz = used + 1;

    if (bufsz > size) {
        kfree(kbuf);
        return -1;
    }

    ret = copy_to_user((void *)buf, (void *)kbuf, bufsz);
    kfree(kbuf);

    return bufsz - ret;
}