// #includes omitted!
// Please read the man pages for the functions.
#include "secret_headers.h"

void getAndPrintGroup(gid_t grpNum)
{
    struct group *grp = getgrgid(grpNum);

    if (grp) {
        printf("The group ID %u -> %s\n", grpNum, grp->gr_name);
    } else {
        printf("No group name for %u found\n", grpNum);
    }
}

void getAndPrintUserName(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);

    if (pw) {
        printf("The user ID %u -> %s\n", uid, pw->pw_name);
    } else {
        perror("Hmm not found???");
        printf("No name found for %u\n", uid);
    }
}

int main()
{
    getAndPrintGroup(1001);
    getAndPrintGroup(514378);
    getAndPrintGroup(103);
    getAndPrintGroup(1000);

    getAndPrintUserName(59894);
    getAndPrintUserName(23524);
    getAndPrintUserName(20746);
    getAndPrintUserName(5970);
    getAndPrintUserName(10485);

    return 0;
}
