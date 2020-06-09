#ifndef _STRUCT_H
#define _STRUCT_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// structura pentru formatul mesajului primit de la clientul UDP
typedef struct {
    char topic[50];
    unsigned char data_type;
    char content[1500];
    int length;
} message;

// structura pentru caracteristicile unui topic nume si SF
typedef struct {
    char name[50];
    int SF;
} top_caract;

// structura pentru client
typedef struct {
    char client_id[10];
    bool online;
    int fd; 
    int n; // numarul de topicuri la care este abonat
    top_caract* topics;
} client;

// structura pentru mesajele SF
typedef struct {
    char id[10];
    char message[2000];
    int size;
    bool printed;
} table_SF;

// structura folosita la trimiterea mesajelor din tabelul SF
typedef struct {
    char message[2000];
    int size;
} UDP_send;
#endif