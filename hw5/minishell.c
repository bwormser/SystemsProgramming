/*******************************************************************************
 * Name        : minishell.c
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 4/13/2021
 * Description : Functions as a minishell
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"
size_t bufsize = 2048;

volatile sig_atomic_t interrupted = false;

void catch_signal(int sig) {
    interrupted = true;
}

int fix_line(char** tokens, char* buffer) {
    int args = 0;
    char delim[] = " \n\0";

    if(read(STDIN_FILENO, buffer, bufsize) == -1) {
        if(interrupted) {
            return -1;
        } else {
            fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
            return -2;
        }
    }

    char *ptr = strtok(buffer, delim);

    tokens[args] = ptr;
    args++;

	while(ptr != NULL) {
		ptr = strtok(NULL, delim);
        tokens[args] = ptr;
        args++;
	}
    
    return args;
}

void change_dir(char** tokens) {
    int succ;
    if(tokens[1] == NULL || !(strcmp(tokens[1], "~"))) {
        uid_t user = getuid();
        struct passwd* pass = getpwuid(user);
        if(pass == 0) {
            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
        }
        char* home = pass->pw_dir;
        succ = chdir(home);
        if(succ == -1) {
            fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", home, strerror(errno));
        }
    } else {
        succ = chdir(tokens[1]);
        if(succ == -1) {
            fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", tokens[1], strerror(errno));
        }
    }
}

int main(int argc, char *argv[]) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = catch_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int args;
    char** tokens = NULL;
    char* buffer = NULL;

    while(true) {
        if(!interrupted) {
            char wkdir[PATH_MAX];
            if (getcwd(wkdir, sizeof(wkdir)) == NULL) {
                fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            }
            printf("[%s%s%s]$ ", BRIGHTBLUE, wkdir, DEFAULT);
            fflush(stdout);
        }
        if(!interrupted) {
            tokens = calloc(bufsize, sizeof(char*));
            if(tokens == NULL) {
                fprintf(stderr, "Error: calloc() failed. %s.\n", strerror(errno));
                interrupted = false;
                continue;
            }
            buffer = calloc(bufsize, sizeof(char));
            if(buffer == NULL) {
                fprintf(stderr, "Error: calloc() failed. %s.\n", strerror(errno));
                interrupted = false;
                free(tokens);
                tokens = NULL;
                continue;
            }
            args = fix_line(tokens, buffer);
            if(args == -1) {
                free(buffer);
                buffer = NULL;
                free(tokens);
                tokens = NULL;
                interrupted = false;
                printf("\n");
                continue;
            } else if(args == -2) {
                free(buffer);
                buffer = NULL;
                free(tokens);
                tokens = NULL;
                printf("\n");
                return EXIT_FAILURE;
            }
        }
        if(!interrupted) {
            if(args > 1) {
                if(!(strcmp(tokens[0], "cd"))) {
                    if(args == 3 || args == 2) {
                        change_dir(tokens);
                    } else {
                        fprintf(stderr, "Error: Too many arguments to cd.\n");
                    }
                } else if(!(strcmp(tokens[0], "exit"))) {
                    free(buffer);
                    buffer = NULL;
                    free(tokens);
                    tokens = NULL;
                    return EXIT_SUCCESS;
                } else {
                    int status;
                    pid_t pid;
                    if ((pid = fork()) < 0) {
                        fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
                        free(buffer);
                        buffer = NULL;
                        free(tokens);
                        tokens = NULL;
                        interrupted = false;
                        continue;
                    } else if (pid > 0) {
                        // We're in the parent.
                        // pid is the process id of the child.
                        do {
                            // Wait for the child to complete and get the status of how it
                            // terminated.
                            pid_t w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
                            if (w == -1) {
                                if(!interrupted) {
                                    fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                                }
                                break;
                            }
                        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                    } else {
                        // We're in the child.
                        if (execvp(tokens[0], tokens) == -1) {
                            fprintf(stderr, "Error: execv() failed. %s.\n", strerror(errno));
                            free(buffer);
                            buffer = NULL;
                            free(tokens);
                            tokens = NULL;
                            return EXIT_FAILURE;
                        }
                    }
                }
            }
        }
        if(interrupted) {
            printf("\n");
            interrupted = false;
        }
        if(buffer != NULL) {
            free(buffer);
            buffer = NULL;
        }
        if(tokens != NULL) {
            free(tokens);
            tokens = NULL;
        }
    }

    return EXIT_SUCCESS;
}