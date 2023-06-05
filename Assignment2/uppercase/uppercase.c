#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE1(uppercase, char *, str)
{
    int i = 0;
    char c;
    while (str[i] != '\0')
    {
        c = str[i];
        if (c >= 'a' && c <= 'z')
        {
            str[i] = c - 32;
        }
        i++;
    }
    return 0;
}
