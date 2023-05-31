#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct {
    char *name;
    char *value;
} EnvVar;

int setEnvVar(EnvVar *envVars, char *line, int envVarCount) {
    char *envVarName;
    char *envVarValue;
    char *envVarDelim;

    envVarDelim = strchr(line, '=');
    envVarName = strtok(line, "=");
    envVarValue = strtok(NULL, "=");
    if (
        (isspace(*(envVarDelim - 1)) || isspace(*(envVarDelim + 1))) ||
        (envVarName == NULL || envVarValue == NULL)
    ) {
        printf("Variable value expected\n");
        return envVarCount;
    }



    printf("envVarName: %s\n", envVarName);
    printf("envVarValue: %s\n", envVarValue);




    // check if environment variable already exists
    for (int i = 0; i < envVarCount; i++) {
        if (strcmp(envVars[i].name, envVarName) == 0) {
            envVars[i].value = realloc(envVars[i].value, strlen(envVarValue) + 1);
            strcpy(envVars[i].value, envVarValue);
            return envVarCount;
        }
    }

    // add new environment variable
    envVars[envVarCount].name = malloc(strlen(envVarName) + 1);
    envVars[envVarCount].value = malloc(strlen(envVarValue) + 1);
    strcpy(envVars[envVarCount].name, envVarName);
    strcpy(envVars[envVarCount].value, envVarValue);
    envVarCount++;

    return envVarCount;
}

int main(int argc, char *argv[]) {
    // EnvVar envVars[100];

    printf("cshell$ ");

    char line[100];
    char *token;
    EnvVar envVars[100];
    int envVarCount = 0;
    
    fgets(line, 100, stdin);
    

    // parse input line
    printf("line: %s\n", line);
    

    envVarCount = setEnvVar(envVars, line, envVarCount);






    // printf("Hello there (pid:%d)\n", (int) getpid());
    // int fc = fork();
    // if (fc < 0) {
    //     printf("Fork failed\n");
    //     exit(1);
    // } else if (fc == 0) {
    //     printf("I am the child (pid:%d)\n", (int) getpid());
    // } else {
    //     printf("I am the parent of %d (pid:%d)\n", fc, (int) getpid());
    // }

    return 0;

}