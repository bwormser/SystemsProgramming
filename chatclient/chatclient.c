/*******************************************************************************
 * Name        : makefile
 * Author      : Brian Wormser, Michael Karsen
 * Date        : 5/7/2021
 * Description : Creates a chat client.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() {
    int ret = get_string(outbuf, MAX_MSG_LEN);
    if(ret == TOO_LONG) {
        fprintf(stderr, "Sorry, limit your message to %d characters.\n", MAX_MSG_LEN);
        return EXIT_SUCCESS;
    }
    if (send(client_socket, outbuf, strlen(outbuf), 0) < 0) {
        fprintf(stderr, "Error: Failed to send message to server. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_client_socket() {
    int bytes_recvd;
    if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0)) < 0) {
        fprintf(stderr, "Warning: Failed to receive incoming message. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    } else if(bytes_recvd == 0) {
        fprintf(stderr, "\nConnection to server has been lost.\n");
        return EXIT_FAILURE;
    }

    inbuf[bytes_recvd] = '\0';
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    int bytes_recvd, ip_conversion;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int retval = EXIT_SUCCESS;

    memset(&server_addr, 0, addrlen);

    char* servIP;
    char* portstr;
    int var = 0;
    int* port = &var;

    if(argc != 3) {
        printf("Usage: %s <server IP> <port>\n", argv[0]);
        retval = EXIT_FAILURE;
    }  else {
        servIP = argv[1];
        portstr = argv[2];
    }

    if((ip_conversion = inet_pton(AF_INET, servIP, &server_addr.sin_addr)) == 0) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    if(!parse_int(portstr, port, "port number")) {
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    if(*port < 1024 || *port > 65535) {
        fprintf(stderr, "Error: Port must be in range [1024, 65535].\n");
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    while(true) {
        printf("Enter your username: ");
        fflush(stdout);

        int res = get_string(username, MAX_NAME_LEN);

        if(res == TOO_LONG) {
            printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
        } else if (res == OK) {
            break;
        }
    }

    printf("Hello, %s. Let's try to connect to the server.\n", username);

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(*port);                 

    if (connect(client_socket, (struct sockaddr *)&server_addr, addrlen) < 0) {
        fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN - 1, 0)) < 0) {
        fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    } else if(bytes_recvd == 0) {
        fprintf(stderr, "All connections are busy. Try again later.\n");
        retval = EXIT_FAILURE;
        goto EXIT;
    } else {
        printf("\n%s\n\n", inbuf);
    }

    memcpy(outbuf, username, sizeof(username));

    if (send(client_socket, outbuf, strlen(outbuf), 0) < 0) {
        fprintf(stderr, "Error: Failed to send username to server. %s.\n", strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    printf("[%s]: ", username);
    fflush(stdout);

    fd_set sockset;
    int max_socket = client_socket;
    while (true) {

        FD_ZERO(&sockset);
        FD_SET(0, &sockset);
        FD_SET(client_socket, &sockset);
        max_socket = client_socket;
       
        if (select(max_socket + 1, &sockset, NULL, NULL, NULL) < 0 && errno != EINTR) {
            fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
            retval = EXIT_FAILURE;
            goto EXIT;
        }

        if (FD_ISSET(client_socket, &sockset)) {
            if (handle_client_socket() == EXIT_FAILURE) {
                retval = EXIT_FAILURE;            
                goto EXIT;
            }

            if(strcmp(inbuf, "bye") == 0) {
                printf("\nServer initiated shutdown.\n");
                goto EXIT;
            } else {
                printf("\n%s\n", inbuf);
            }

        }

        if (FD_ISSET(0, &sockset)){
            if (handle_stdin() == EXIT_FAILURE) {
                retval = EXIT_FAILURE;            
                goto EXIT;
            }

            if(strcmp(outbuf, "bye") == 0) {
                printf("Goodbye.\n");
                goto EXIT;
            }
        }

        printf("[%s]: ", username);
        fflush(stdout);
    }

    EXIT:
    if (fcntl(client_socket, F_GETFD) >= 0) {
        close(client_socket);
    }    
    return retval;
}