Enache Anca- Mihaela
324CD

   Aceasta tema a avut ca scop realizarea unei aplicatii ccare respecta modelul
client_server, din aceasta cauza am ales sa folosesc laboratorul 8 ca punct de 
inceput. Implementarea am facut-o in C folosind structuri si alocare dinamica,
in 5 fisiere diferite: server.c, subscriber.c, functions.h, struct.h si 
helpers.h(luat din laboratorul 8).

	server.c
   Pentru inceput, am adaugat pe langa socket-ul TCP pe care l-am folosit in 
laboratorul 8, un socket UDP, le-am initializat campurile, asociat adrese(bind)
si am setat socketul TCP ca fiind pasiv cu functia listen(). Am adaugat file
descriptorii pentru socketi si cel pentru STDIN in multimea "read".
   Server-ul va functiona cat timp acesta nu primeste comanda "exit" de la STDIN.
Daca aceasta comanda este primita socketii din fisierul server se inchid si dau
return 0, astfel incat atunci cand clientul primeste 0 la recv se va inchide si
acesta.

   Daca se realizeaza o conexiune cu server-ul UDP(i == sock_udp), voi receptiona
mesajul in buffer. Incep prin a-mi aloca memorie pentru un vector de char-uri in
care voi construi pas cu pas mesajul ce va fi trimis spre client. Introduc in 
acest vector informatii despre IP si despre port(folosind functia sprintf pentru
a trece valoarea intreaga intoarsa de htons in string). De asemenea in structura
message_recv(struct message, definita in fisierul struct.h), retin topicul, tipul
de date, continutul si lungimea acestuia. Apelez functia conver_message(definita 
si implementata in fisierul functions.h) ce are ca scop convertirea continutul
primit in functie de tipul de date anuntat. Aceasta conversie se realizeaza 
intr-un switch cu 4 cazuri(int, short_real, float, string). Pentru fiecare caz
tin cont de informatiile scrise in "Tabela 2:Tipuri de date" din enuntul temei
si extrag din buffer continutul primit, pentru primul caz(0) folosesc functia 
"ntohl" impreuna cu un if ce determina semnul numarului primit, pentru urmatoarele
2 cazuri (1,2) folosesc functia "ntohs" impreuna cu un if ce determina semnul.
La finalul fiecarui caz termin de construit mesajul si salvez dimensiunea acestuia
in variabila size. Aceasta variabila va fi returnata de functia in fisier server.c.
   Avand mesajul convertit, pargurg lista de clienti si verific daca vreunul din
topicurile la care acestia sunt abonati corespunde cu topicul mesajului primit 
de la clientul UDP. 
- Daca acesta corespunde si clientul este online, creez o 
variabila locala "buff" (de tip UDP_send, definit in struct.h), in care introduc
mesajul si dimensiunea acestuia si o trimit. 
- Daca acesta corespunde, insa clientul este offline, verific daca SF-ul este 
setat pe 1, daca da salvez id-ul clientului, mesajul, dimensiunea acestuia 
si o variabila setata pe false ce imi va spune daca mesajul a fost sau nu deja
trimis intr-un tabel( declarat ca un vector de structuri table_SF*) ce isi 
actualizeaza dimensiunea si memoria(realloc) dupa fiecare element nou adaugat.

   Daca se primeste o cerere de conexiune de la un client TCP (i == sock_tcp), 
o accept, dezactivez algoritmul Neagle si receptionez mesajul intr-un buffer. 
Incep prin a presupune ca este vorba de un client nou. 
-Daca acesta este nou(dupa cautarea in lista), il adaug in vectorul de structuri 
"clients"(de tip struct client*) si initializez campurile fd, client_id, online
n(numarul de topicuri la care este abonat) si aloc un camp pentru  vectorul de 
topicuri(de tip struct top_caract*), printez mesajul "New client..." in server si
cresc memoria vectorului "clients".
-Daca acesta nu este la prima conectare, reactualizez campul fd, online si verific
daca acesta are vreun topic la care este abonat cu campul SF setat pe 1 pentru ai
trimite mesajele din buffer, daca acestea exista. Trimiterea se face in acelasi mod
ca in cazul trimiterii din conexiunea UDP, doar ca datele sunt luate de data asta
din tabelul de tip table_SF*. Lista de topicuri la care s-a inscris clientul
ramane nemodificata cat timp server-ul ramane deschis.

   Daca se primeste o comanda de la clientul TCP, receptionez mesajul in buffer. 
Caut dupa "i" in lista de clienti clientul ce a trimis comanda. 
-Daca clientul se deconecteaza (n==0), campul online se seteaza pe false, fd-ul 
ia -1, printez un mesaj in server si inchid conexiunea cu clientul. 
-Daca clientul trimite comanda alta comanda, folosesc un char pointer impreuna
cu strtok pentru a lua numele comenzii si apelez functia subscribe_unsubscribe
(definita si implementata in functions.h)
   In functie verific ce comanda s-a primit. Daca s-a primit un subscribe, 
folosesc un strtok pentru a extrage numele topicului si valoarea SF ului. Verific
daca clientul nu este deja abonat la acest topic dacanu este introduc topicul
in lista de topicuri a clientului impreuna cu valoarea SF-ului si cresc dimensiunea
si memoria vectorului de topicuri. Daca s-a primit un unsubscribe, extrag cu strtok
numele topicului, verific daca exista in lista de topicuri daca exista il sterg.
   
   La sfarsitul fisierului elimerez memoria si inchid socket-urile.

	subscribers.c
   Ma folosesc de laboratorul 8 aducand schimbari in while. Unde verific daca se
primeste comanda de la STDIN sau se primeste mesaj de la socket.
   Daca se primeste comanda de la tastatura verific daca s-a primit "exit". Daca
s-a primit inchid socket-ul, scot file descriptorii din multimile read si tmp si
returnez 0. Daca se primeste alta comanda folosesc un char pointer si strtok pentru
a lua numele comenzii si apelez functia commands_check(definitia si implementata in
functions.h) pentru a verifica daca se respecta formatul cerut, in caz ca nu se 
respecta se afiseaza un mesaj si clientul se inchide. Daca comanda primita este
corecta aceasta este trimisa catre server.
   Daca se primeste mesaj de la socket(clientul UDP), creez o variabila locala
buff de tip UDP_send, in care voi receptiona mesajul primit. Daca functia recv
intoarce 0 inseamna ca server-ul a primit comanda exit si ca clientii trebuie sa
se inchida. Dupa ce am receptionat mesajul, reduc dimensiunea continutului la
dimensiunea primita de la server si il printez pe ecran.


   