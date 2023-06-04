#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_ENV_VAR_COUNT 100
#define MAX_COMMAND_COUNT 100
#define MAX_COMMAND_LENGTH 100
#define MAX_COMMAND_TOKENS_COUNT 10
#define MAX_COMMAND_LOG_COUNT 100

typedef struct {
    char *name;
    char *value;
} EnvVar;

typedef struct {
    char *name;
    struct tm time;
    int returnVal;
} Command;

int setCommandLog(char **tokens, char *command, Command *commandLog, int commandLogCount, int returnVal) {
    time_t rawtime;
    char *commandName;

    if ((returnVal == -1) || (tokens[0][0] == '$')) {
        commandName = command;
    } else {
        commandName = tokens[0];
    }
    
    time(&rawtime);

    commandLog[commandLogCount].name = malloc(strlen(commandName) + 1);
    strcpy(commandLog[commandLogCount].name, commandName);
    commandLog[commandLogCount].time = *localtime(&rawtime);
    commandLog[commandLogCount].returnVal = returnVal;
    commandLogCount++;

    return commandLogCount;
}

//boolean function to check if the command is an environment variable
int isValidEnvVar(char *command) {
    int commandLength = strlen(command);
    int isEnvVarDelim = 0;

    if ((command[0] != '$') || (commandLength < 2)) {
        return 0;
    }

    for (int i = 1; i < commandLength; i++) {
        if (command[i] == '=') { // check if there is only one '='
            if (isEnvVarDelim || (command[i-1] == '$')) {
                return 0;
            }
            isEnvVarDelim = 1;
        } else if (command[i] == ' ') { // check if there is no space
            int isAtEnd = 1;
            for (int j = i; j < commandLength; j++) {
                if (command[j] != ' ') {
                    isAtEnd = 0;
                }
            }
            if (!isAtEnd) {
                return 0;
            }
        }
    }

    return 1;
}

int setEnvVar(char *command, EnvVar *envVars, int envVarCount) {
    
    if (!isValidEnvVar(command)) {
        return envVarCount;
    }

    char *commandCopy = strdup(command);
    char *envVarName;
    char *envVarValue;

    envVarName = strtok(commandCopy, "=");
    envVarValue = strtok(NULL, "=");

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

    free(commandCopy);
    return envVarCount;
}

char **commandTokenize(char *command) {
    int i = 0;
    char *commandCopy = strdup(command);
    char **tokens = malloc(MAX_COMMAND_TOKENS_COUNT * sizeof(char *));
    char *token;

    // tokenize command
    token = strtok(commandCopy, " \t\n=");
    while (token != NULL && i < MAX_COMMAND_TOKENS_COUNT - 1) {
        tokens[i] = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(tokens[i], token);
        i++;
        token = strtok(NULL, " \t\n=");
    }
    tokens[i] = NULL;

    // reallocate tokens
    tokens = realloc(tokens, (i + 1) * sizeof(char *));

    free(commandCopy);
    return tokens;
}

int commandExecute(char **tokens, char *command, EnvVar *envVars, int envVarCount, Command *commandLog, int commandLogCount) {
    int returnVal = 0;

    // exit
    if (strcmp(tokens[0], "exit") == 0) {
        if (tokens[1] != NULL) {
            printf("Error: Too many Arguments detected\n");
            returnVal = -1;
            return returnVal;
        }

        printf("Bye!\n");
        
        for (int i = 0; i < envVarCount; i++) {
            free(envVars[i].name);
            free(envVars[i].value);
        }
        for (int i = 0; i < commandLogCount; i++) {
            free(commandLog[i].name);
        }
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);

        exit(0);
    }

    // print
    else if (strcmp(tokens[0], "print") == 0) {
        // create a string to store the command
        char printCommand[MAX_COMMAND_LENGTH] = "";

        // show error message if no arguments are given
        if (tokens[1] == NULL) {
            printf("Please enter a valid argument!");
            returnVal = -1;
        }
        // print arguments
        else {
            for (int i = 1; tokens[i] != NULL; i++) {
                if (tokens[i][0] == '$') {
                    for (int j = 0; j < envVarCount; j++) {
                        if (strcmp(tokens[i], envVars[j].name) == 0) {
                            // add to printCommand with space
                            strcat(printCommand, envVars[j].value);
                            strcat(printCommand, " ");
                            goto end;
                        }
                    }
                    // replace printCommand with error message
                    printf("Error: No Environment Variable %s found.\n", tokens[i]);
                    returnVal = -1;
                    return returnVal;
                } else {
                    strcat(printCommand, tokens[i]);
                    strcat(printCommand, " ");
                }
            }
        }
        
    end:
        printf("%s\n", printCommand);
        return returnVal;
    }

    // log
    else if (strcmp(tokens[0], "log") == 0) {
        if (tokens[1] != NULL) {
            printf("Error: Too many Arguments detected\n");
            returnVal = -1;
            return returnVal;
        }

        for (int i = 0; i < commandLogCount; i++) {
            printf("%s", asctime(&commandLog[i].time));
            printf(" %s ", commandLog[i].name);
            printf("%d\n", commandLog[i].returnVal);
        }

        return returnVal;
    }

    // theme
    else if (strcmp(tokens[0], "theme") == 0) {
        if (tokens[1]) {
            if (strcmp(tokens[1], "red") == 0) {
                printf("\033[0;31m");
            } else if (strcmp(tokens[1], "green") == 0) {
                printf("\033[0;32m");
            } else if (strcmp(tokens[1], "blue") == 0) {
                printf("\033[0;34m");
            } else {
                printf("unsupported theme\n");
                returnVal = -1;
            }
        } else {
            printf("unsupported theme\n");
            returnVal = -1;
        }
        
        return returnVal;
    }

    else if (tokens[0][0] == '$') {
        if (!isValidEnvVar(command)) {
            printf("Variable value expected\n");
            returnVal = -1;
        }
    }


    // fork
    else {
        pid_t child_pid = fork();
        
        if (child_pid >= 0) {
            if (child_pid == 0) {
                if (execvp(tokens[0], tokens) == -1) {
                    printf("Missing keyword or command, or permission problem\n");
                    exit(1);
                }
            } else {
                int status;
                wait(&status);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    returnVal = -1;
                }
            }
        }

        return returnVal;
    }

    return returnVal;
}

int main(int argc, char *argv[]) {
    char command[MAX_COMMAND_LENGTH];
    char **tokens;
    
    EnvVar envVars[MAX_ENV_VAR_COUNT];
    int envVarCount = 0;

    Command commandLog[MAX_COMMAND_LOG_COUNT];
    int commandLogCount = 0;
    int returnVal;

    // script mode
    if (argv[1]) {
        FILE *script = fopen(argv[1], "r");
        if (script == NULL) {
            printf("Unable to read script file: %s\n", argv[1]);
            exit(1);
        }

        // create an array to store the commands from file
        char commandsFromFile[MAX_COMMAND_COUNT][MAX_COMMAND_LENGTH];
        int commandCountFromFile = 0;
        while (fgets(command, MAX_COMMAND_LENGTH, script) != NULL) {
            command[strcspn(command, "\n")] = 0; // remove trailing newline
            strcpy(commandsFromFile[commandCountFromFile], command);
            commandCountFromFile++;
        }

        // run
        for (int i = 0; i < commandCountFromFile; i++) {
            envVarCount = setEnvVar(commandsFromFile[i], envVars, envVarCount);

            tokens = commandTokenize(commandsFromFile[i]);

            returnVal = commandExecute(tokens, commandsFromFile[i], envVars, envVarCount, commandLog, commandLogCount);

            commandLogCount = setCommandLog(tokens, commandsFromFile[i], commandLog, commandLogCount, returnVal);

            for (int i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
        }

        // free memory
        for (int i = 0; i < envVarCount; i++) {
            free(envVars[i].name);
            free(envVars[i].value);
        }
        for (int i = 0; i < commandLogCount; i++) {
            free(commandLog[i].name);
        }
        fclose(script);

        // exit cleanly after executing in script mode
        strcpy(command, "exit");

        envVarCount = setEnvVar(command, envVars, envVarCount);

        tokens = commandTokenize(command);
        returnVal = commandExecute(tokens, command, envVars, envVarCount, commandLog, commandLogCount);

        commandLogCount = setCommandLog(tokens, command, commandLog, commandLogCount, returnVal);
        // return 0;
    }

    // interactive mode
    else {
        while(1) {
            printf("cshell$ ");
            
            fgets(command, MAX_COMMAND_LENGTH, stdin);
            command[strcspn(command, "\n")] = 0; // remove trailing newline

            envVarCount = setEnvVar(command, envVars, envVarCount);

            tokens = commandTokenize(command);
            returnVal = commandExecute(tokens, command, envVars, envVarCount, commandLog, commandLogCount);

            commandLogCount = setCommandLog(tokens, command, commandLog, commandLogCount, returnVal);

            for (int i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
        }
    }

    // free memory
    for (int i = 0; i < envVarCount; i++) {
        free(envVars[i].name);
        free(envVars[i].value);
    }
    for (int i = 0; i < commandLogCount; i++) {
        free(commandLog[i].name);
    }

    return 0;
}
