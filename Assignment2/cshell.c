#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <linux/kernel.h>
#include <sys/syscall.h>

#define __NR_uppercase 449

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

// function to tokenize a command
char **commandTokenize(char *command) {
    int i = 0;
    char *commandTemp = strdup(command);
    char **tokens = malloc(MAX_COMMAND_TOKENS_COUNT * sizeof(char *));
    char *token;

    // tokenize command
    token = strtok(commandTemp, " \t\n=");
    while (token != NULL && i < MAX_COMMAND_TOKENS_COUNT - 1) {
        tokens[i] = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(tokens[i], token);
        i++;
        token = strtok(NULL, " \t\n=");
    }
    tokens[i] = NULL;

    // reallocate tokens
    tokens = realloc(tokens, (i + 1) * sizeof(char *));

    // free memory
    free(commandTemp);

    return tokens;
}

// function to check if a command is a valid environment variable
// -1 => Variable name expected
// -2 => Variable value expected
// 0 => Invalid assignment, print nothing
// 1 => Valid assignment, print nothing
int isValidEnvVarCheck(char *command) {
    int commandLength = strlen(command);
    int isEnvVarDelim = 0;

    // check if a command starts with '$' and has at least 2 characters
    if ((command[0] != '$') || (command[1] == '=')) {
        return -1;
    }

    // print error msg when $var =...
    for (int i = 1; command[i] != '='; i++) {
        if (command[i] == ' ') {
            return -2;
        }
        if (command[i] == '/') {
            return -1;
        }
    }

    // check if a command has only one '=' and no space (or spaces at the end)
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

    // // check if a EnvName has only letters, numbers, and underscores
    // for (int i = 1; command[i] != '='; i++) {
    //     if (!isalpha(command[i]) && (command[i] != '_')) {
    //         return 0;
    //     }
    // }

    return 1;
}

// function to store environment variables
int setEnvVar(char *command, EnvVar *envVars, int envVarCount) {
    
    // check if command is a valid environment variable
    if (isValidEnvVarCheck(command) != 1) {
        return envVarCount;
    }

    char *commandTemp = strdup(command);
    char *envVarName;
    char *envVarValue;

    envVarName = strtok(commandTemp, "=");
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
    envVars[envVarCount].name = malloc((strlen(envVarName) + 1) * sizeof(char));
    envVars[envVarCount].value = malloc((strlen(envVarValue) + 1) * sizeof(char));
    strcpy(envVars[envVarCount].name, envVarName);
    strcpy(envVars[envVarCount].value, envVarValue);
    envVarCount++;

    // free memory
    free(commandTemp);

    return envVarCount;
}

// function to store command log
int setCommandLog(char **tokens, char *command, Command *commandLog, int commandLogCount, int returnVal) {
    char *commandName;
    time_t rawtime;
    time(&rawtime);

    if (tokens[0] == NULL) {
        return commandLogCount;
    }
    
    // store full command if the command fails or is an environment variable
    if (tokens[0][0] == '$') {
        commandName = command;
    } else {
        commandName = tokens[0];
    }

    // store each command with its name, time, and return value
    commandLog[commandLogCount].name = malloc((strlen(commandName) + 1) * sizeof(char));
    strcpy(commandLog[commandLogCount].name, commandName);
    commandLog[commandLogCount].time = *localtime(&rawtime);
    commandLog[commandLogCount].returnVal = returnVal;
    commandLogCount++;

    return commandLogCount;
}

// function to execute commands
int commandExecute(char **tokens, char *command, EnvVar *envVars, int envVarCount, Command *commandLog, int commandLogCount, int *currentThemeColor) {
    int returnVal = 0;

    if (tokens[0] == NULL) {
        return returnVal;
    }
    
    // built-in command
    if (strcmp(tokens[0], "exit") == 0) { // exit
        // show error message if too many arguments are given
        if (tokens[1] != NULL) {
            printf("Error: Too many Arguments detected\n");
            returnVal = -1;
            return returnVal;
        }

        printf("Bye!\n");
        
        // free memory
        if (envVars != NULL) {
            for (int i = 0; i < envVarCount; i++) {
                free(envVars[i].name);
                free(envVars[i].value);
            }
        }
        if (commandLog != NULL) {
            for (int i = 0; i < commandLogCount; i++) {
                free(commandLog[i].name);
            }
        }
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);

        exit(0);
    } else if (strcmp(tokens[0], "print") == 0) { // print
        // create a string to store the command
        char printCommand[MAX_COMMAND_LENGTH] = "";
        int isValidEnvVar = 0;

        if (tokens[1] == NULL) { // show error message if no arguments are given
            printf("Please enter a valid argument!");
            returnVal = -1;
        } else { // store the command to printCommand
            for (int i = 1; tokens[i] != NULL; i++) {
                if (tokens[i][0] == '$') { // if the argument is an environment variable
                    isValidEnvVar = 0;

                    // if the environment variable exists, add to printCommand with space
                    for (int j = 0; j < envVarCount; j++) {
                        if (strcmp(tokens[i], envVars[j].name) == 0) {
                            // add to printCommand with space
                            strcat(printCommand, envVars[j].value);
                            strcat(printCommand, " ");
                            isValidEnvVar = 1;
                        }
                    }

                    // if the environment variable does not exist, show error message
                    if (!isValidEnvVar) {
                        printf("Error: No Environment Variable %s found.\n", tokens[i]);
                        returnVal = -1;
                        return returnVal;
                    }
                } else { // if the argument is not an environment variable, add to printCommand with space
                    strcat(printCommand, tokens[i]);
                    strcat(printCommand, " ");
                }
            }
        }
        printf("%s\n", printCommand);

        return returnVal;
    } else if (strcmp(tokens[0], "log") == 0) { // log
        // show error message if too many arguments are given
        if (tokens[1] != NULL) {
            printf("Error: Too many Arguments detected\n");
            returnVal = -1;
            return returnVal;
        }

        // print the command log
        for (int i = 0; i < commandLogCount; i++) {
            printf("%s", asctime(&commandLog[i].time));
            printf(" %s ", commandLog[i].name);
            printf("%d\n", commandLog[i].returnVal);
        }

        return returnVal;
    } else if (strcmp(tokens[0], "theme") == 0) { // theme
        // if has more than two tokens, show error message
        if (tokens[2] != NULL) {
            printf("Error: Too many Arguments detected\n");
            returnVal = -1;
            return returnVal;
        }
        
        if (tokens[1]) {
            if (strcmp(tokens[1], "red") == 0) {
                *currentThemeColor = 1;
                printf("\033[0;31m");
            } else if (strcmp(tokens[1], "green") == 0) {
                *currentThemeColor = 2;
                printf("\033[0;32m");
            } else if (strcmp(tokens[1], "blue") == 0) {
                *currentThemeColor = 3;
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
    } else if (tokens[0][0] == '$') { // environment variable
        int isValid = isValidEnvVarCheck(command);
        if (isValid != 1) {
            returnVal = -1;
            if (isValid == -1) {
                printf("Variable name expected\n");  
            } else if (isValid == -2) {
                printf("Variable value expected\n");  
            }
        }

    }
    
    // BONUS: uppercase
    else if (strcmp(tokens[0], "uppercase") == 0) {
        if (tokens[1] == NULL) { // show error message if no arguments are given
            printf("Please enter a valid argument!");
            returnVal = -1;
        } else { // convert the argument to uppercase
            for (int i = 1; tokens[i] != NULL; i++) {
                syscall(__NR_uppercase, tokens[i]);
                printf("%s ", tokens[i]);
            }
        }
        printf("\n");
        return returnVal;
    }

    // non-built-in command
    else {
        // replace environment variables with their values
        for (int i = 1; tokens[i] != NULL; i++) {
            if (tokens[i][0] == '$') { // if the argument is an environment variable
                // if the environment variable exists, add to printCommand with space
                for (int j = 0; j < envVarCount; j++) {
                    if (strcmp(tokens[i], envVars[j].name) == 0) {
                        // add to printCommand with space
                        tokens[i] = realloc(tokens[i], strlen(envVars[j].value) + 1);
                        strcpy(tokens[i], envVars[j].value);
                    }
                }
            }
        }

        pid_t child_pid = fork();

        if (child_pid >= 0) {
            if (child_pid == 0) {
                // return error message if execvp fails (error commands))
                if (execvp(tokens[0], tokens) == -1) {
                    printf("Missing keyword or command, or permission problem\n");
                    exit(1);
                }
            } else {
                int status;
                wait(&status);
                // return value is -1 for error commands
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

    int currentThemeColorCode = 0;
    int *currentThemeColor = &currentThemeColorCode;

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

        // execute each command from file
        for (int i = 0; i < commandCountFromFile; i++) {
            envVarCount = setEnvVar(commandsFromFile[i], envVars, envVarCount);

            tokens = commandTokenize(commandsFromFile[i]);

            returnVal = commandExecute(tokens, commandsFromFile[i], envVars, envVarCount, commandLog, commandLogCount, currentThemeColor);

            commandLogCount = setCommandLog(tokens, commandsFromFile[i], commandLog, commandLogCount, returnVal);

            for (int i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
        }

        fclose(script);

        // exit cleanly and clean memory after executing in script mode
        strcpy(command, "exit");

        envVarCount = setEnvVar(command, envVars, envVarCount);

        tokens = commandTokenize(command);
        returnVal = commandExecute(tokens, command, envVars, envVarCount, commandLog, commandLogCount, currentThemeColor);

        commandLogCount = setCommandLog(tokens, command, commandLog, commandLogCount, returnVal);
    }

    // interactive mode
    else {
        while(1) {
            printf("cshell$ ");

            printf("\033[0m");

            fgets(command, MAX_COMMAND_LENGTH, stdin);
            command[strcspn(command, "\n")] = 0; // remove trailing newline

            if (*currentThemeColor == 1) {
                printf("\033[0;31m");
            } else if (*currentThemeColor == 2) {
                printf("\033[0;32m");
            } else if (*currentThemeColor == 3) {
                printf("\033[0;34m");
            }

            fflush(stdout);

            envVarCount = setEnvVar(command, envVars, envVarCount);

            tokens = commandTokenize(command);
            returnVal = commandExecute(tokens, command, envVars, envVarCount, commandLog, commandLogCount, currentThemeColor);

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
