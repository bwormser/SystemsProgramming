/*******************************************************************************
 * Name        : pfind.c
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 3/16/2021
 * Description : Recursively finds directories and files with matching permissions.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

int perms[] = {S_IRUSR, S_IWUSR, S_IXUSR,
               S_IRGRP, S_IWGRP, S_IXGRP,
               S_IROTH, S_IWOTH, S_IXOTH};

char* permission_string(struct stat *statbuf) {
    char* permString = (char *)malloc(11 * sizeof(char));
    int permission_valid;
    char* temp = permString;
    *temp = '-';
    temp++;

    // this is to print the file type bit, as displayed in `$ ls -l`.
    for (int i = 0; i < 9; i += 3) {
        permission_valid = statbuf->st_mode & perms[i];
        if (permission_valid) { 
            *temp = 'r';
            temp++;   
        } else {
            *temp = '-';
            temp++;    
        }    
        permission_valid = statbuf->st_mode & perms[i+1];
        if (permission_valid) {
            *temp = 'w';
            temp++;      
        } else {
            *temp = '-';
            temp++;    
        }    
        permission_valid = statbuf->st_mode & perms[i+2];
        if (permission_valid) {
            *temp = 'x';
            temp++;       
        } else {
            *temp = '-';
            temp++;    
        }
    }
    *temp = '\0';
    return permString;
}

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

int to_bin(char* perms) {
    char* bin_str = perms;

    for(int x=0; x<strlen(perms); x++) {
        if(perms[x] == '-') {
            bin_str[x] = '0';
        } else {
            bin_str[x] = '1';
        }
    }

    int bin = atoi(bin_str);
    return bin;
}

void parse_full(char* name, int match) {
    char path[PATH_MAX];
    if (realpath(name, path) == NULL) {
        fprintf(stderr, "Error: Cannot get full path of file '%s'. %s.\n",
                name, strerror(errno));
        return;
    }

    DIR *dir;
    if ((dir = opendir(path)) == NULL) {
        fprintf(stderr, "Error: Cannot open directory '%s'. %s.\n",
                path, strerror(errno));
        return;
    }

    struct dirent *entry;
    struct stat sb;
    char full_filename[PATH_MAX+1];
    size_t pathlen = 0;

    // Set the initial character to the NULL byte.
    // If the path is root '/', you can now take the strlen of a properly
    // terminated empty string.
    full_filename[0] = '\0';
    if (strcmp(path, "/")) {
        // If path is not the root - '/', then ...

        // If there is no NULL byte among the first n bytes of path,
        // the full_filename will not be terminated. So, copy up to and
        // including PATH_MAX characters.
        strncpy(full_filename, path, PATH_MAX);
    }
    // Add + 1 for the trailing '/' that we're going to add.
    pathlen = strlen(full_filename) + 1;
    full_filename[pathlen - 1] = '/';
    full_filename[pathlen] = '\0';

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue; 
        }
        strncpy(full_filename + pathlen, entry->d_name, PATH_MAX - pathlen);
        if (lstat(full_filename, &sb) < 0) {
            fprintf(stderr, "Error: Cannot stat file '%s'. %s.\n",
                    full_filename, strerror(errno));
            continue;
        } else {
            char* file_perms_str = permission_string(&sb);
            int file_perms = to_bin(file_perms_str);
            if(file_perms == match) {
                printf("%s\n", full_filename);
            }
            free(file_perms_str);
        }
        if (entry->d_type == DT_DIR) {
            parse_full(full_filename, match);
        }
    }

    closedir(dir);
}

int main(int argc, char **argv) {
    
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
                printf("Usage: ./pfind -d <directory> -p <permissions string> [-h]\n");
                return EXIT_SUCCESS;
            case '?':
                printf("Error: Unknown option '-%c' received.\n", optopt);
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if(argc == 1) {
        printf("Usage: ./pfind -d <directory> -p <permissions string> [-h]\n");
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

    int bin_perms = to_bin(perms);

    parse_full(dir_name, bin_perms);

    return EXIT_SUCCESS;
}