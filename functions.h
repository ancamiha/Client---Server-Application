#ifndef _SERVER_FUNCTIONS_H
#define _SERVER_FUNCTIONS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "struct.h"

#define MAX_CLIENTS 20
#define MAX_LENGTH 1551

// functie apelata atunci cand se primeste o conexiune de la socketul UDP
// converteste textul primit in formatul specificat, data_type
int convert_message(char buffer[MAX_LENGTH], char* to_send,
                    message message_recv) {
    int size;
	int int_n;
	double real_n;
	long long result;
    char string_value[256];
	switch (message_recv.data_type) {
	case 0: ;
		// s-a primit un INT
		// conversie
		uint32_t content_0 = 0;
		memcpy(&content_0, buffer + 52, sizeof(content_0));
		int_n = ntohl(content_0); 
						
		// verific octetul de semn 
		if (message_recv.content[0] == 1) { // numarul este negativ
			result = (-1) * (long long)int_n;
		} else {
			result = (long long)int_n;
		}
							
		// construire mesaj
		strcat(to_send, "INT");
		strcat(to_send, " - ");
		sprintf(string_value, "%lld", result);
		strcat(to_send, string_value);
		size = strlen(to_send);
		break;
	case 1: ;
		// s-a primit un SHORT_REAL
		// conversie
		uint16_t content_1 = 0;
		memcpy(&content_1, buffer + 51, sizeof(content_1));
		real_n = ntohs(content_1); 
		real_n = (double)real_n / 100;

		// verific octetul de semn 
		if (message_recv.content[0] == 1) { // numarul este negativ
			real_n *= (-1);
		}

		// construire mesaj
		strcat(to_send, "SHORT_REAL");
		strcat(to_send, " - ");
		sprintf(string_value, "%0.2f", real_n);
		strcat(to_send, string_value);
		size = strlen(to_send);
		break;
	case 2: ;
		// s-a primit un FLOAT
		// conversie
		uint32_t content_2 = 0;
		uint8_t mod = 0;
		memcpy(&content_2, buffer + 52, sizeof(content_2));
		memcpy(&mod, buffer + 56, sizeof(mod));
		real_n = ntohl(content_2);
		real_n = (double)real_n / (double)(pow(10, mod));

		// verific octetul de semn 
		if (message_recv.content[0] == 1) { // numarul este negativ
			real_n *= (-1);
		}

		// construire mesaj
		strcat(to_send, "FLOAT");
		strcat(to_send, " - ");
		// folosesc %.10g pentru a scapa de trailing zeros
		sprintf(string_value, "%.10g", real_n);
		strcat(to_send, string_value);
		size = strlen(to_send);
		break;
	case 3: ;
		// s-a primit un STRING	
		strcat(to_send, "STRING");
		strcat(to_send, " - ");
		strcat(to_send, message_recv.content);
		size = strlen(to_send);
		break;
		default:
		fprintf(stderr, "This data_type doesn't exist!\n");
			break;
		}
    return size;
}

// functie apelata in caz ca server-ul primeste o comanda de la clientul TCP
// subscribe sau unsubscribe
void subscribe_unsubscribe(char* p, char id[10], client* clients, int k) {
    // primeste comanda de subscribe
    if (!strncmp(p, "subscribe", 10)) {
		// p ia numele topicului
		p = strtok(NULL, " ");
		char name[50];
		strcpy(name, p);

	    int ok = 1; //presupun ca topicul nu exista deja
		// caut in lista de topicuri sa vad daca topicul exista deja
		for (int t = 0; t < clients[k].n; t++) {
			if (strncmp(name, clients[k].topics[t].name, strlen(name)) == 0) {
				fprintf(stderr, "The topic already exists.\n");
				ok = 0;
				break;
			}
		}

        // daca nu s-a gasit topicul
		if (ok == 1) {
			strcpy(clients[k].topics[clients[k].n].name, p);
			// p ia valoarea SF-ului 0/1
			p = strtok(NULL, " ");
			clients[k].topics[clients[k].n].SF = atoi(p);

			// cresc memoria vectorului de topicuri
			clients[k].n += 1;
			clients[k].topics = realloc(clients[k].topics, (clients[k].n + 1) * sizeof(top_caract));
            printf("The client (%s) subscribed %s.\n", id, name);
		}
	} else {
		// primeste comanda de unsubscribe
		if (!strncmp(p, "unsubscribe", 12)) {
			// p ia numele topicului
			p = strtok(NULL, " ");
			char name[50];
			strcpy(name, p);
										
			int ok = 0; // presupun ca topicul  nu exista in lista
			int idx = 0;
			// caut topicul in lista de topicuri
			for (int t = 0; t < clients[k].n; t++) {
				if (strncmp(name, clients[k].topics[t].name, strlen(name) - 1) == 0) {
					ok = 1;
					idx = t;
					break;
				}
		    }

            // daca nu s-a gasit topicul				
			if (ok == 1) {
				// sterg din lista topicul la care s-a dat unsubscribe
				for (int t = idx; t < (clients[k].n) - 1; t++) {
					clients[k].topics[t] = clients[k].topics[t + 1];
				}
				// reduc dimensiunea vectorului
				clients[k].n -= 1;
				name[strlen(name) - 1] = name[strlen(name)];
				printf("The client (%s) unsubscribed %s.\n", id, name);
			} else {
				fprintf(stderr, "The topic doesn't exist.\n");
			}
		}
	}
}

// functie apelata in subscriber.c atunci cand clientul trimite o comanda
// de la tastatura pentru a verifica daca comanda respecta formatul
void commands_check(char* p) {
	if (!strncmp(p, "subscribe", 10)) {
    	// p ia numele topicului
        p = strtok(NULL, " "); 

        if (p == NULL) {
            fprintf(stderr, "The topic is missing.\n");
            exit(0);
        }

        // p ia valoarea lui SF 0/1
        p = strtok(NULL, " ");
        if (p == NULL) {
            fprintf(stderr, "The SF value is missing.\n");
            exit(0);
        }

        int SF = atoi(p);
        if (SF != 0 && SF != 1) {
            fprintf(stderr, "SF value doesn't exist.\n");
            exit(0);
        }
    } else {
        if (!strncmp(p, "unsubscribe", 12)) {
            // p ia numele topicului
            p = strtok(NULL, " "); 
            if (p == NULL) {
                fprintf(stderr, "The topic is missing.\n");
                exit(0);
            } 
        } else {
            fprintf(stderr, "The command doesn't exist.\n");
         	exit(0);
        }
    } 
}
#endif