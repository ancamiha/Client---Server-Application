#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "functions.h"

#define MAX_CLIENTS 20
#define MAX_LENGTH 1551

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[MAX_LENGTH];

    fd_set read;
    fd_set tmp;

    int fdmax;

    // se golesc multimile de descriptori
    FD_ZERO(&tmp);
    FD_ZERO(&read);

    // verificam daca sunt un numar corect de argumente
	if (argc < 4) {
		usage(argv[0]);
	}

    // verificam daca ID-ul clientului este corect
    if (strlen(argv[1]) > 10) {
        fprintf(stderr, "Client ID must have 10 characters\n");
        exit(0);
    }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

    //se initializeaza campurile din serv_addr
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
    
	FD_SET(sockfd, &read);
    FD_SET(0, &read);
    fdmax = sockfd;

    // trimit id-ul clientului ca mesaj
    n = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(n < 0, "send");

	while (1) {
        tmp = read;
		ret = select(fdmax + 1, &tmp, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp)) {
                if (i == STDIN_FILENO) {
                    // mesaj primit de la stdin
                    memset(buffer, 0, MAX_LENGTH);
                    fgets(buffer, MAX_LENGTH - 1, stdin);

                    // pentru comanda primita exit se deconecteaza
                    if (!strncmp(buffer, "exit", 4)) {
                        FD_CLR(i, &read);
                        FD_CLR(i, &tmp);
                        FD_CLR(sockfd, &read);
                        FD_CLR(sockfd, &tmp);
                        close(sockfd);
                        return 0;
                    }

                    // alta comanda 
                    char* p;
                    char* copy = malloc(sizeof(buffer));
                    strcpy(copy, buffer);
                    // p ia numele comenzii
                    p = strtok(buffer, " ");
                    
                    // se verifica ce comanda s-a primit si se verifica
                    // daca respecta formatul acceptat
                    commands_check(p);

                    // se trimite mesaj la server
                    n = send(sockfd, copy, strlen(copy), 0);
                    DIE(n < 0, "send");
                    free(copy);
                } else {
                    // se receptioneaza mesajul de la socket
                    if (i == sockfd) {
                        // creez variabila locala
						// pentru a stoca mesajul si
						// dimensiunea acestuia 
                        UDP_send buff;
                        
                        memset(&buff, 0, sizeof(UDP_send));
                        n = recv(sockfd, &buff, sizeof(UDP_send), 0);
                        if (n == 0) {
                            // server-ul a primit comanda de exit,
                            // clientii se inchid
                            close(sockfd);
                            return 0;
                        }
                        DIE(n < 0, "recv");

                        // reduc marimea mesajului primit la 
                        // marimea mesajului original
                        buff.message[buff.size] = 
                                    buff.message[strlen(buff.message)];
                        printf("%s\n", buff.message);
                        
                    }
                }
            }
        }
    }
	close(sockfd);
	return 0;
}
