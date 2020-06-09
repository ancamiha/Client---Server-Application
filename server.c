#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <math.h>
#include "functions.h"
#include "helpers.h"
#include "struct.h"


// returneaza maximul dintre 2 numere
int max(int a, int b) {
	if (a >= b) {
		return a;
	}
	return b;
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	// lista de clienti
	int cl_size = 0;
	client* clients = calloc(1, sizeof(client));
	// structura pentru mesaj
	message message_recv;
	// lista de topicuri
	int top_size = 0;
	table_SF* table = calloc(1, sizeof(table_SF));

	int sock_udp, sock_tcp, newsockfd, port_number;
	char buffer[MAX_LENGTH];
	struct sockaddr_in serv_addr_udp, cli_addr, serv_addr_tcp;
	int n, i, ret;
	socklen_t clilen_tcp, clilen_udp;

	fd_set read;	// multimea de citire folosita in select()
	fd_set tmp;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read
	int flag_delay = 1;

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read)
	// si multimea temporara (tmp)
	FD_ZERO(&read);
	FD_ZERO(&tmp);

	//se creeaza socketul pentru clientii UDP
	sock_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "socket");

	//se creeaza socketul pentru clientii TCP
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "socket");

	port_number = atoi(argv[1]);
	DIE(port_number == 0, "atoi");

	memset((char *) &serv_addr_udp, 0, sizeof(serv_addr_udp));
	//se initializeaza campurile din serv_addr_udp
	serv_addr_udp.sin_family = AF_INET;
	serv_addr_udp.sin_port = htons(port_number);
	serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

	memset((char *) &serv_addr_tcp, 0, sizeof(serv_addr_udp));
	//se initializeaza campurile din serv_addr_tcp
	serv_addr_tcp.sin_family = AF_INET;
	serv_addr_tcp.sin_port = htons(port_number);
	serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

	//socketului i se asociaza o adresa
	ret = bind(sock_udp, (struct sockaddr *) &serv_addr_udp, 
			   sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	ret = bind(sock_tcp, (struct sockaddr *) &serv_addr_tcp, 
			   sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	//listen pentru oricat de multi clienti
	ret = listen(sock_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// in multimea read
	FD_SET(sock_udp, &read);
	FD_SET(sock_tcp, &read);
	FD_SET(0, &read);
	fdmax = max(sock_udp, sock_tcp);

	// cat timp nu se primeste comanda "exit" server-ul va rula
	while (1) {
		tmp = read; 	
		ret = select(fdmax + 1, &tmp, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp)) {
				if (i == STDIN_FILENO) { // comanda de la tastatura
					fgets(buffer, MAX_LENGTH - 1, stdin);
					if (!strncmp(buffer, "exit", 4)) {
						// comanda exit a fost primita
						close(sock_tcp);
						close(sock_udp);      
						return 0;
					} else {
						fprintf(stderr, "This command doesn't exist!\n");
					}
				} else {
					//conexiune socket UDP
					if (i == sock_udp) { 
						memset(buffer, 0, MAX_LENGTH);
						clilen_udp = sizeof(struct sockaddr);
						n = recvfrom(sock_udp, buffer, MAX_LENGTH, 0,
							(struct sockaddr *) &serv_addr_udp, &clilen_udp);
						DIE(n < 0, "recv");
						
						// construiesc inceputul mesajului de trimis
						char string_value[256]; // folosesc acest vector in sprintf
						char* to_send = malloc(2000 * sizeof(char));
						strcpy(to_send, inet_ntoa(serv_addr_udp.sin_addr));
						strcat(to_send,":");
						sprintf(string_value, "%u", htons(serv_addr_udp.sin_port));
						strcat(to_send, string_value);

						//parsarea
						// retin topicul
						strncpy(message_recv.topic, buffer, 49);
						// retin tipul de date 0, 1, 2 sau 3
						message_recv.data_type = (int8_t)buffer[50];
						// retin continutul
						strncpy(message_recv.content, buffer + 51, 1500);
						// retin lungimea
						message_recv.length = strlen(message_recv.content);
					
						// introdduc topicul in mesaj
						strcat(to_send, " - ");
						strcat(to_send, message_recv.topic);
						strcat(to_send, " - ");

						int size;
						// convertesc mesajul primit
						size = convert_message(buffer, to_send, message_recv);

						// parcurg lista de clienti
						for (int j = 0; j < cl_size; j++) {
							client c = clients[j];
							// parcurg lista de topicuri a clientului j
							for (int k = 0; k < c.n; k++) {
								// daca clientul este online
								if (!strcmp(c.topics[k].name, message_recv.topic)
									&& c.online) {
									// creez variabila locala
									// pentru a stoca mesajul si
									// dimensiunea acestuia
									UDP_send buff;
												
									// stochez in structura
									strcpy(buff.message, to_send);
									buff.size = size;
									// trimit
									n = send(c.fd, &buff, sizeof(UDP_send), 0);
									DIE(n < 0, "send");
								} else {
									// daca clientul este offline si SF == 1
									if (!strcmp(c.topics[k].name, message_recv.topic)
										&& !c.online && c.topics[k].SF == 1) {
											strcpy(table[top_size].id, c.client_id);
											strcpy(table[top_size].message, to_send);
											table[top_size].size = size;
											table[top_size].printed = false;

											// cresc marimea vectorului
											top_size++;
											table = realloc(table,
												(top_size + 1) * sizeof(table_SF));
									}
								}
							}
						}
						// eliberez memoria
						free(to_send);
					} else {
						// s-a primit o noua cerere de conexiune 
						// de la un client TCP
						if (i == sock_tcp) {
						// a venit o cerere de conexiune pe socketul inactiv
						// (cel cu listen), pe care serverul o accepta
							clilen_tcp = sizeof(cli_addr);
							newsockfd = accept(sock_tcp, 
								(struct sockaddr *) &cli_addr, &clilen_tcp);
							DIE(newsockfd < 0, "accept");

							// dezactive algoritmului Neagle
							setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag_delay, sizeof(int));

							// se adauga noul socket intors de accept()
							// la multimea descriptorilor de citire
							FD_SET(newsockfd, &read);
							if (newsockfd > fdmax) { 
								fdmax = newsockfd;
							}
							
							memset(buffer, 0, MAX_LENGTH);
							n = recv(newsockfd, buffer, MAX_LENGTH - 1, 0);
							DIE(n < 0, "recv");

							// presupun ca este vorba despre un client nou
							int ok = 1; 
							int idx = 0;
							// parcurg lista de clienti
							for (int j = 0; j < cl_size; j++) {
								if(!strncmp(buffer, clients[j].client_id,
									strlen(buffer))) {
									// clientul exista deja
									ok = 0;
									idx = j;
									break;
								}
							}
							if (ok == 1) {
								// salvez detaliile despre noul client in lista
								clients[cl_size].fd = newsockfd;
								strcpy(clients[cl_size].client_id, buffer);
								clients[cl_size].online = true;
								// pregatesc lista de topicuri
								clients[cl_size].n = 0;
								clients[cl_size].topics = calloc(1,
													sizeof(top_caract));

								printf("New client (%s) connected from %s:%d.\n", 
									clients[cl_size].client_id,
									inet_ntoa(cli_addr.sin_addr),
									ntohs(cli_addr.sin_port)); 

								// cresc memoria vectorului de structuri
								cl_size++;
								clients = realloc(clients, (cl_size + 1)
													* sizeof(client));
							} else {
								// clientul nu este la prima conectare
								// actualizez campul fd si campul online
								clients[idx].fd = newsockfd;
								clients[idx].online = true;
								printf("Client (%s) reconnected from %s:%d.\n",
									clients[idx].client_id, 
									inet_ntoa(cli_addr.sin_addr),
									ntohs(cli_addr.sin_port)); 

								client c = clients[idx];
								for (int j = 0; j < c.n && c.online; j++) {
									// daca clientul este online si SF == 1
									if (c.topics[j].SF == 1) { 
										for (int t = 0; t < top_size; t++) {
											if (!strcmp(c.client_id, table[t].id) 
												&& !table[t].printed) {
												// creez variabila locala
												// pentru a stoca mesajul si
												// dimensiunea acestuia
												UDP_send buff;
												
												// stochez in structura
												strcpy(buff.message,
														table[t].message);
												buff.size = table[t].size;
												table[t].printed = true;
												// trimit
												n = send(c.fd, &buff, 
														sizeof(UDP_send), 0);
												DIE(n < 0, "send");
											}
										}
									}
								}
							}
						} else {
							// comanda primita de la clientul TCP
							memset(buffer, 0, MAX_LENGTH);
							n = recv(i, buffer, MAX_LENGTH - 1, 0);
							DIE(n < 0, "recv");
							char id[10];
							int k;
							for (int j = 0; j < cl_size; j++) {
								// caut in lista de clienti
								// clientul cu fd == i
								if (clients[j].fd == i && clients[j].online) {
									k = j;
									break;
								}
							}
							// daca un client se deconecteaza
							if (n == 0) {
								strcpy(id, clients[k].client_id);
								// actualizez campurile din structura
								clients[k].online = false;
								clients[k].fd = -1;
								printf("Client (%s) disconnected.\n", id);
								close(i);
								FD_CLR(i, &read);
							} else {
								// clientul a trimis comanda de subscribe
								// sau de unsubscribe
								client cli = clients[k];
								strcpy(id, cli.client_id);

								char* p;
								// p ia numele comenzii
								p = strtok(buffer, " ");
								// primeste comanda de subscribe sau de unsubscribe
								subscribe_unsubscribe(p, id, clients, k);
							}
						}
					}
				}
			}
		}
	}
	// eliberez memoria
	for (int i = 0; i < cl_size; i++) {
		free(clients[i].topics);
	}
	free(clients);
	free(table);
	// inchid socketii
	close(sock_udp);
	close(sock_tcp);
	return 0;
}
