/*******************************************************************************
 * Name        : sort.c
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 3/2/2021
 * Description : Uses quicksort to sort a file of either ints, doubles, or
 *               strings.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quicksort.h"

#define MAX_STRLEN     64 // Not including '\0'
#define MAX_ELEMENTS 1024

typedef enum {
    STRING,
    INT,
    DOUBLE
} elem_t;

/**
 * Reads data from filename into an already allocated 2D array of chars.
 * Exits the entire program if the file cannot be opened.
 */
size_t read_data(char *filename, char **data) {
    // Open the file.
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open '%s'. %s.\n", filename,
                strerror(errno));
        free(data);
        exit(EXIT_FAILURE);
    }

    // Read in the data.
    size_t index = 0;
    char str[MAX_STRLEN + 2];
    char *eoln;
    while (fgets(str, MAX_STRLEN + 2, fp) != NULL) {
        eoln = strchr(str, '\n');
        if (eoln == NULL) {
            str[MAX_STRLEN] = '\0';
        } else {
            *eoln = '\0';
        }
        // Ignore blank lines.
        if (strlen(str) != 0) {
            data[index] = (char *)malloc((MAX_STRLEN + 1) * sizeof(char));
            strcpy(data[index++], str);
        }
    }

    // Close the file before returning from the function.
    fclose(fp);

    return index;
}

/**
 * Basic structure of sort.c:
 *
 * Parses args with getopt.
 * Opens input file for reading.
 * Allocates space in a char** for at least MAX_ELEMENTS strings to be stored,
 * where MAX_ELEMENTS is 1024.
 * Reads in the file
 * - For each line, allocates space in each index of the char** to store the
 *   line.
 * Closes the file, after reading in all the lines.
 * Calls quicksort based on type (int, double, string) supplied on the command
 * line.
 * Frees all data.
 * Ensures there are no memory leaks with valgrind. 
 */

void help_msg() {
    printf("Usage: ./sort [-i|-d] filename\n");
    printf("   -i: Specifies the file contains ints.\n");
    printf("   -d: Specifies the file contains doubles.\n");
    printf("   filename: The file to sort.\n");
    printf("   No flags defaults to sorting strings.\n");
}

void print_iarr(int* array, size_t len) {
    for(int i = 0; i < len; i++) {
        printf("%d\n", *(array + i));
    }
}

void print_darr(double* array, size_t len) {
    for(int i = 0; i < len; i++) {
        printf("%f\n", *(array + i));
    }
}

void print_sarr(char** array, size_t len) {
    for(int i = 0; i < len; i++) {
        printf("%s\n", *(array + i));
    }
}

int main(int argc, char **argv) {
    
    int num = 0;
    int doub = 0;
    int st = 1;

    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, "id")) != -1) {
        switch (c){
            case 'i':
                num = 1;
                st = 0;
                break;
            case 'd':
                doub = 1;
                st = 0;
                break;
            case '?':
                printf("Error: Unknown option '-%c' received.\n", optopt);
                help_msg();
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if((num + doub + st) > 1) {
        printf("Error: Too many flags specified.\n");
        return EXIT_FAILURE;
    }
    
    if(argc == 1) {
        help_msg();
        return EXIT_FAILURE;
    } else if(argc-optind > 1) {
        printf("Error: Too many files specified.\n");
        return EXIT_FAILURE;
    } else if (argc-optind == 0) {
        printf("Error: No input file specified.\n");
        return EXIT_FAILURE;
    }
    

    FILE *file;
    char** array;
    size_t len;
    if ((file = fopen(argv[optind], "r"))){
        fclose(file);
        array = calloc(MAX_ELEMENTS, sizeof(char*));
        len = read_data(argv[optind], array);
    } else {
        printf("Error: Cannot open '%s'. No such file or directory.\n", argv[optind]);
        return EXIT_FAILURE;
    }
    
    st = 1;
    if(num == 1){
        int* int_array = (int*)calloc(len, sizeof(int));
        for(int i=0; i<len; i++) {
            *(int_array+i) = atoi(*(array+i));
        }
        quicksort(int_array, len, sizeof(int), int_cmp);
        print_iarr(int_array, len);
        free(int_array);
    } else if(doub == 1){
        double* double_array = (double*)calloc(len, sizeof(double));
        for(int i=0; i<len; i++) {
            *(double_array+i) = atof(*(array+i));
        }
        quicksort(double_array, len, sizeof(double), dbl_cmp);
        print_darr(double_array, len);
        free(double_array);
    } else if(st == 1){
        quicksort(array, len, sizeof(char*), str_cmp);
        print_sarr(array, len);
    }

    for(int x=0; x<len; x++) {
        free(*(array+x));
    }
    free(array);
    
    
    return EXIT_SUCCESS;
}
