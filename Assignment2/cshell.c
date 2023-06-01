#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ENV_VAR_COUNT 100
#define MAX_COMMAND_LENGTH 100
#define MAX_COMMAND_TOKENS_COUNT 10

typedef struct {
    char *name;
    char *value;
} EnvVar;

int setEnvVar(char *command, EnvVar *envVars, int envVarCount) {
    if (command[0] != '$') {
        return envVarCount;
    }

    char *envVarName;
    char *envVarValue;
    char *envVarDelim;

    printf("command: %s\n", command);

    envVarDelim = strchr(command, '=');
    envVarName = strtok(command, "=");
    envVarValue = strtok(NULL, "=");
    if (envVarDelim) {
        if (
            (isspace(*(envVarDelim - 1)) || isspace(*(envVarDelim + 1))) ||
            (envVarName == NULL || envVarValue == NULL)
        ) {
            printf("Variable value expected1\n");
            return envVarCount;
        }
    } else {
        printf("Variable value expected2\n");
        return envVarCount;
    }




    // printf("envVarName: %s\n", envVarName);
    // printf("envVarValue: %s\n", envVarValue);




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

void commandTokenize(char *command, char *tokens[]) {
    int i = 0;

    // tokenize command
    tokens[i] = strtok(command, " \t\n=");
    while (tokens[i] != NULL && i < MAX_COMMAND_TOKENS_COUNT - 1) {
        i++;
        tokens[i] = strtok(NULL, " \t\n=");
    }
    tokens[i] = NULL;
}

int commandExecute(char *tokens[]) {
    int status = 0;

    // char *tokens[MAX_COMMAND_TOKENS_COUNT];
    // int i = 0;

    if (strcmp(tokens[0], "exit") == 0) {
        printf("Bye!\n");
        exit(0);

        printf("test");
        
        // free
    } else {
        pid_t child_pid = fork();

        if (child_pid < 0) {
            exit(1);
        } else if (child_pid == 0) {
            execvp(tokens[0], tokens);
            // perror("execvp");
            exit(1);
        } else {
            // wait(&status);
            // return(status);
            waitpid(child_pid, &status, 0);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    char command[MAX_COMMAND_LENGTH];
    char *tokens[MAX_COMMAND_TOKENS_COUNT];
    EnvVar envVars[MAX_ENV_VAR_COUNT];
    int envVarCount = 0;

    while(1) {
        printf("cshell$ ");
        
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        command[strcspn(command, "\n")] = 0; // remove trailing newline
        // printf("command: %s\n", command);

        
        
        envVarCount = setEnvVar(command, envVars, envVarCount);
        // printf("envVarCount: %d\n", envVarCount);
        // for(int i = 0; i < envVarCount; i++) {
        //     printf("envVarName: %s\n", envVars[i].name);
        //     printf("envVarValue: %s\n", envVars[i].value);
        // }
        
        commandTokenize(command, tokens); // tokenize command
        commandExecute(tokens);
    }

    return 0;
}
