#include <stdio.h>
#include <string.h>

int main() {
    char inputUsername[100];
    char userInfo[4096];
    char *currentUsername;
    char *userIDToken;
    
    // get input username from user
    printf("%s", "Enter username: ");
    scanf("%s", inputUsername);
    
    // open /etc/passwd file
    FILE *userFile = fopen("/etc/passwd", "r");
    if (userFile) {
        while(fgets(userInfo, sizeof(userInfo), userFile)) {
            // get username
            currentUsername = strtok(userInfo, ":");

            // if username matches input username, print user ID
            if (strcmp(currentUsername, inputUsername) == 0) {
                // skip password
                strtok(NULL, ":");
                // get user ID
                userIDToken = strtok(NULL, ":");
                // print user ID
                printf("%s\n", userIDToken);

                fclose(userFile);
                return 0;
            }
        }
    }

    // if no user ID found, print error message
    printf("%s", "no such user found\n");
    
    fclose(userFile);
    return 0;
}
