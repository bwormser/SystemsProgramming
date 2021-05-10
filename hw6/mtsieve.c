/*******************************************************************************
 * Name        : mtsieve.c
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 4/23/2021
 * Description : Runs threads to find all primes from a to b.
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
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <sys/sysinfo.h>

typedef struct arg_struct {
    int start;
    int end;
} thread_args;

int total_count = 0;
pthread_mutex_t lock;

int countDigit(long long num){
    int count = 0;
    while(num != 0){
        num = num / 10;
        ++count;
    }
    return count;
}

void *threadSieve(void *ptr) {

    thread_args* arg = (thread_args*) ptr;
    int start = arg->start;
    int end = arg-> end;

    if(pthread_mutex_lock(&lock) != 0) {
        fprintf(stderr, "Warning: Cannot lock mutex. %s.\n", strerror(errno));
    }

    int root = sqrt(end);

    bool prime[root + 1];
    memset(prime, true, sizeof(prime));
    

    for (int p = 2; p * p <= root; p++) {
        if (prime[p] == true) {
            for (int i = p * p; i <= root; i += p) {
                prime[i] = false;
            }
        }
    }

    int count = 0;
    for (int p = 2; p <= root; p++) {
        if (prime[p]) {
            count++;
        }
    }

    int low_primes[count];
    int pos = 0;
    for (int p = 2; p <= root; p++) {
        if (prime[p]) {
            low_primes[pos] = p;
            pos++;
        }
    }

    bool* high_primes = malloc((end - start + 1) * sizeof(bool));

    for(int x=0; x<(end - start + 1); x++) {
        high_primes[x] = true;
    }

    int i;
    for(int x=0; x<pos; x++) {
        int p = low_primes[x];
        i = ceil((double)start/p) * p - start;
        if(start <= p) {
            i = i + p;
        }
        int y = i;
        int increment = 1;
        while(y<(end - start + 1)) {
            if((y+start)%p == 0) {
                high_primes[y] = false;
                increment = p;
            }

            y += increment;
        }
    }

    for(int x=0; x<(end - start + 1); x++) {
        if(high_primes[x]) {
            int tempThreeCount = 0;
            int curr = x+start;
            for(int i = 0; i < countDigit(x+start); i++){
                if(curr % 10 == 3){
                    ++tempThreeCount;
                }
                curr = floor((double)curr / 10);
                if(tempThreeCount >= 2){
                    ++total_count;
                    break;
                }
            }
        }
    }
    free(high_primes);
    if(pthread_mutex_unlock(&lock) != 0){
        fprintf(stderr, "Warning: Cannot unlock mutex. %s.\n", strerror(errno));
    }

    pthread_exit(NULL);
}

bool checkInt(int* result, char* num, char arg) {
    int value = atoi(num);
    long long value_l = atoll(num);

    if(value != value_l) {
        printf("Error: Integer overflow for parameter '-%c'.\n", arg);
        return false;;
    } else if(value == 0) {
        printf("Error: Invalid input '%s' received for parameter '-%c'.\n", num, arg);
        return false;
    }

    *result = value;
    return true;
}

int main(int argc, char* argv[]) {

    if(argc == 1) {
        printf("Usage: ./mtsieve -s <starting value> -e <ending value> -t <num threads>\n");
        return EXIT_FAILURE;
    }

    bool start_b = false;
    bool end_b = false;
    bool threads_b = false;

    int start;
    int end;
    int threads;

    int c;
    opterr = 0;
    int* result = &c;
    while((c = getopt (argc, argv, "s:e:t:")) != -1) {
        switch(c) {
            case 's':
                if(checkInt(result, optarg, c)) {
                    start = *result; 
                    start_b = true;
                    break;
                } else {
                    return EXIT_FAILURE;
                }
            case 'e':
                if(checkInt(result, optarg, c)) {
                    end = *result; 
                    end_b = true;
                    break;
                } else {
                    return EXIT_FAILURE;
                }
            case 't':
                if(checkInt(result, optarg, c)) {
                    threads = *result;
                    threads_b = true;
                    break;
                } else {
                    return EXIT_FAILURE;
                }
            case '?':
                if(optopt == 'e' || optopt == 's' || optopt == 't') {
                    fprintf(stderr, "Error: Option -%c requires an argument.\n", optopt);
                } else if(isprint(optopt)) {
                    fprintf(stderr, "Error: Unknown option '-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Error: Unknown option character '\\x%x'.\n", optopt);
                }
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if(optind < argc) {
        printf("Error: Non-option argument '%s' supplied.\n", argv[optind]);
        return EXIT_FAILURE;
    } else if(!start_b) {
        printf("Error: Required argument <starting value> is missing.\n");
        return EXIT_FAILURE;
    } else if(start < 2) {
        printf("Error: Starting value must be >= 2.\n");
        return  EXIT_FAILURE;
    } else if(!end_b) {
        printf("Error: Required argument <ending value> is missing.\n");
        return EXIT_FAILURE;
    } else if(end < 2) {
        printf("Error: Ending value must be >= 2.\n");
        return  EXIT_FAILURE;
    } else if(end < start) {
        printf("Error: Ending value must be >= starting value.\n");
        return EXIT_FAILURE;
    } else if(!threads_b) {
        printf("Error: Required argument <num threads> is missing.\n");
        return EXIT_FAILURE;
    } else if(threads < 1) {
        printf("Error: Number of threads cannot be less than 1.\n");
        return EXIT_FAILURE;
    } else if(threads > (2 * get_nprocs())) {
        printf("Error: Number of threads cannot exceed twice the number of processors(%d).\n", (2 * get_nprocs()));
        return EXIT_FAILURE;
    }

    int retval;
    if ((retval = pthread_mutex_init(&lock, NULL)) != 0) {
        fprintf(stderr, "Error: Cannot create mutex. %s.\n", strerror(retval));
        return EXIT_FAILURE;
    }

    int testNums = end - start + 1;
    if(testNums < threads) {
        threads = testNums;
    }

    printf("Finding all prime numbers between %d and %d.\n", start, end);
    printf("%d segments:\n", threads);
    
    pthread_t runThreads[threads];
    thread_args targs[threads];

    int div = testNums / threads;
    int rem = testNums % threads;

    int subStart = start;
    int subEnd = start - 1;
    for(int x=0; x<threads; x++) {
        if(x < rem) {
            subEnd += div + 1;
        } else {
            subEnd += div;
        }

        printf("   [%d, %d]\n", subStart, subEnd);

        targs[x].start = subStart;
        targs[x].end = subEnd;
        // printf("pre\n");
        if ((retval = pthread_create(&runThreads[x], NULL, threadSieve, &targs[x])) != 0) {
            fprintf(stderr, "Error: Cannot create thread %d. %s.\n", x, strerror(retval));
            return EXIT_FAILURE;
        }
        // printf("post\n");
        subStart = subEnd + 1;
    }

    for (int i=0; i<threads; i++) {
        if (pthread_join(runThreads[i], NULL) != 0) {
            fprintf(stderr, "Warning: Thread %d did not join properly.\n", i + 1);
        }
    }

    if ((retval = pthread_mutex_destroy(&lock)) != 0) {
        fprintf(stderr, "Error: Cannot destroy mutex. %s.\n", strerror(retval));
        return EXIT_FAILURE;
    }

    printf("Total primes between %d and %d with two or more '3' digits: %d\n", start, end, total_count);

    return EXIT_SUCCESS;
}