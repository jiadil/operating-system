#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    char shellInfo[4096];
    char shellName[100][100];
    int shellCount = 0;
    char *shellToken;
    int isDuplicate;
    
    // open /etc/shells file
    FILE *shellFile = fopen("/etc/shells", "r");
    while(fgets(shellInfo, sizeof(shellInfo), shellFile)) {
        // get shell name
        shellToken = strrchr(shellInfo, '/') + 1;

        // check if shell name is duplicate
        isDuplicate = 0;
        for (int i = 0; i < shellCount; i++) {
            if (strcmp(shellName[i], shellToken) == 0) {
                isDuplicate = 1;
            }
        }

        // if not duplicate, add to shellName
        if (!isDuplicate) {
            strcpy(shellName[shellCount], shellToken);
            shellCount++;
        }   
    }

    // print shell names
    for (int i = 1; i < shellCount; i++) {
        printf("%s", shellName[i]);
    }

    fclose(shellFile);
    return 0;
}
