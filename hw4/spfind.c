/*******************************************************************************
 * Name        : spfind.c
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 3/16/2021
 * Description : Recursively finds directories and files with matching permissions
 *               then pipes that information to sort then back to the parent process.
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

bool perm_valid(char* perms) {
    bool valid = true;
    if(strlen(perms) != 9) {
        valid = false;
        return valid;
    }
    for(int x=0; x<9; x++) {
        if(x == 0 || x == 3 || x == 6) {
            if(perms[x] != 'r' && perms[x] != '-') {
                valid = false;
            }
        } else if(x == 1 || x == 4 || x == 7) {
            if(perms[x] != 'w' && perms[x] != '-') {
                valid = false;
            }
        } else if(x == 2 || x == 5 || x == 8) {
            if(perms[x] != 'x' && perms[x] != '-') {
                valid = false;
            }
        }
    }

    return valid;
}

int main(int argc, char *argv[]) {
    int dir_f = 0;
    int perm_f = 0;

    char* dir_name;
    char* perms;

    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, ":d:p:h")) != -1) {
        switch (c){
            case 'd':
                dir_f = 1;
                dir_name = optarg; 
                break;
            case 'p':
                perm_f = 1;
                perms = optarg;
                break;
            case 'h':
                printf("Usage: ./spfind -d <directory> -p <permissions string> [-h]\n");
                return EXIT_SUCCESS;
            case '?':
                printf("Error: Unknown option '-%c' received.\n", optopt);
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if(argc == 1) {
        printf("Usage: ./spfind -d <directory> -p <permissions string> [-h]\n");
        return EXIT_FAILURE;
    }

    if(dir_f != 1) {
        printf("Error: Required argument -d <directory> not found.\n");
        return EXIT_FAILURE;
    }

    if(perm_f != 1) {
        printf("Error: Required argument -p <permissions string> not found.\n");
        return EXIT_FAILURE;
    }
    
    if(!perm_valid(perms)) {
        printf("Error: Permissions string '%s' is invalid.\n", perms);
        return EXIT_FAILURE;
    }

    struct stat statbuf;
    if (stat(dir_name, &statbuf) < 0) {
        printf("Error: Cannot stat '%s'. No such file or directory.\n", dir_name);
        return EXIT_FAILURE;
    }

    int pfind_to_sort[2], sort_to_parent[2];
    pipe(pfind_to_sort);
    pipe(sort_to_parent);
    

    pid_t pid[2];
    if ((pid[0] = fork()) == 0) {
        // pfind
        close(pfind_to_sort[0]);
        dup2(pfind_to_sort[1], STDOUT_FILENO);

        // Close all unrelated file descriptors.
        close(sort_to_parent[0]);
        close(sort_to_parent[1]);

        if (execlp("./pfind", "./pfind", "-d", dir_name, "-p", perms, NULL) == -1) {
            fprintf(stderr, "Error: pfind failed.\n");
            return EXIT_FAILURE;
        }
    }

    if ((pid[2] = fork()) == 0) {
        // sort
        close(pfind_to_sort[1]);
        dup2(pfind_to_sort[0], STDIN_FILENO);
        close(sort_to_parent[0]);
        dup2(sort_to_parent[1], STDOUT_FILENO);

        if (execlp("sort", "sort", NULL) == -1) {
            fprintf(stderr, "Error: sort failed.\n");
            return EXIT_FAILURE;
        }

    }


    close(sort_to_parent[1]);
    dup2(sort_to_parent[0], STDIN_FILENO);
    // Close all unrelated file descriptors.
    close(pfind_to_sort[0]);
    close(pfind_to_sort[1]);

    char buffer[128];
    int matches = 0;
    ssize_t count = 1;
    while(count != 0) {
        count = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (count == -1) {
            perror("read()");
            exit(EXIT_FAILURE);
        }
        buffer[count] = '\0';
        for(int x=0; x<=count; x++) {
            if(buffer[x] == '\n') {
                matches++;
            }
        }
        printf("%s", buffer);
    }

    close(sort_to_parent[0]);
    
    int statusp;
    int statuss;
    wait(&statusp);
    wait(&statuss);
    int es1;
    int es2;

    if ( WIFEXITED(statusp) ) {
        es1 = WEXITSTATUS(statusp);
    }
    if ( WIFEXITED(statuss) ) {
        es2 = WEXITSTATUS(statuss);
    }

    if(es1 != 1 && es2 != 1) {
        printf("Total matches: %d\n", matches);
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
