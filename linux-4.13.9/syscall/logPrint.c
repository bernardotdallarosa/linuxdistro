#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include "logPrint.h"

#define MSG_BUF_MAX 256

asmlinkage long sys_logPrint(const char __user *msg) {
    char kbuf[MSG_BUF_MAX];
    long copied;

    copied = strncpy_from_user(kbuf, msg, sizeof(kbuf));

    if (copied < 0 || copied == sizeof(kbuf))
        return -1;

    printk(KERN_INFO "logPrint: %s\n", kbuf);

    return 0;
}