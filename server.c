#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS	5
#define BUFLEN 256

/*
structura ajutatoare ce tine datele despre clienti (ordinea citirii din fisier)
*/

typedef struct{
    char client_name[13], client_surname[13];
    char client_cardno[7];
    char client_cardpin[5];
    char client_passwd[17];
    float client_sold;
} client ;


int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    unsigned int clilen;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr, udp_sock;
    int n, i, j;
    int clients_no;
    
    int countcli = 0;
    int clnts[20];       // vector ce tine evidenta clientilor ce se logheaza --> 1 daca da, 0 daca nu
    int k = 0;             // indice de evidenta pt clnts
    int curr = 0;
    
    int pin = 0;          // numara de cate ori s-a introdus pinul gresit
    int blk;                 // indicele clientului blocat

    unsigned int sd_len;
    struct sockaddr_in from_station;

    client *cl;            // vectorul de clienti

     char filename[14];
     FILE *log;

    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds

    if (argc < 2) {
        fprintf(stderr,"Usage : %s port\n", argv[0]);
        exit(1);
    }

    /*
    se deschide fisierul cu baza de date, se citesc datele despre clienti si se tin intr-un 
    vector de clienti <-- cl
    */

    FILE *fl = fopen(argv[2], "r+");
    fscanf(fl, "%d", &clients_no);
    cl = malloc(clients_no * sizeof(client));

    for (j = 0; j < clients_no; j++){
       fscanf(fl, "%s", cl[j].client_name);
       fscanf(fl, "%s", cl[j].client_surname);
       fscanf(fl, "%s", cl[j].client_cardno);
       fscanf(fl, "%s", cl[j].client_cardpin);
       fscanf(fl, "%s", cl[j].client_passwd);
       fscanf(fl, "%f", &cl[j].client_sold);

    }

    fclose(fl);

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR TCP socket");

    int sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd == -1)
        perror("Error UDP socket");

    portno = atoi(argv[1]);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    memset((char*) &udp_sock, 0, sizeof(udp_sock));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	
    serv_addr.sin_port = htons(portno);

    udp_sock.sin_family = AF_INET;
    udp_sock.sin_port = htons(portno);
    udp_sock.sin_addr.s_addr = INADDR_ANY;
    

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    if (bind(sd, (struct sockaddr*)&udp_sock, sizeof(struct sockaddr)) < 0){
        perror("ERROR binding");
        exit(1);
    }

    listen(sockfd, MAX_CLIENTS);

    FD_SET(sockfd, &read_fds);
    FD_SET(sd, &read_fds);
    fdmax = sockfd;

    while(1){
        tmp_fds = read_fds;
        if (select(fdmax+1, &tmp_fds, NULL, NULL, NULL) == -1)
            perror("ERROR in select");

        for ( i = 0; i <= fdmax; i++ ){
            if (FD_ISSET(i, &tmp_fds)){

                if (i == sockfd){  //cand vine o conexiune noua <-- accept
                    clilen = sizeof(cli_addr);

                    if ((newsockfd = accept (sockfd, (struct sockaddr*)&cli_addr, &clilen)) == -1)
                        perror("Not accepted");
                    else{ // adauga noul socket la multimea descr de citire
                        FD_SET(newsockfd, &read_fds);

                        if (newsockfd > fdmax)
                            fdmax = newsockfd;
                    }
                  
                    countcli++;
                    sprintf(filename, "client-%d.log", countcli);
                    log = fopen(filename, "w+");
                    k++;  // tine numarul clientului care se conecteaza la ATM
                }

                // se primesc date pe socketul UDP
                // doar cand se primeste comanda unlock
                // verifica existenta cardului, apoi cere parola daca e cazul
                // primeste parola si deblocheaza
                else if (i == sd){
                    printf("A PRIMIT UDP\n");
                    memset(buffer, 0 ,BUFLEN);
                    recvfrom (sd, buffer, BUFLEN, 0, (struct sockaddr*)&from_station, &sd_len);
                    if (strstr(buffer, "unlock") != NULL){
                        fprintf(log, "%s\n", buffer);
                        char *tok = strtok(buffer, " ");
                        tok = strtok(NULL, " ");

                        for (j = 0; j < clients_no; j++){
                            if (strcmp(tok, cl[j].client_cardno) == 0){
                                printf("UNLOCK> Trimite parola.\n");
                                fprintf(log, "%s\n", "UNLOCK> Trimite parola\n");
                                sendto (sd, "Trimite parola secreta\n", strlen("Trimite parola secreta\n"), 0, (struct sockaddr*)&udp_sock, sizeof(struct sockaddr));
                                blk = j;
                                break;
                            }
                        }
                        if ( j == clients_no ){
                            printf("UNLOCK> -4: Numar card inexistent\n");
                            fprintf(log, "UNLOCK> -4: Numar card inexistent\n");
                        }
                    }
                    else{
                        char *tok = strtok (buffer, " ");
                        if (strcmp (tok, cl[blk].client_cardno) == 0){
                            tok = strtok (NULL, " ");
                            if (strcmp (tok, cl[blk].client_passwd) == 0){
                                printf("UNLOCK>Client deblocat.\n");
                                fprintf(log, "UNLOCK>Client deblocat.\n");
                            }
                            else{
                                printf("UNLOCK> -7: Deblocare esuata\n");
                                fprintf(log, "UNLOCK> -7: Deblocare esuata\n");
                            }
                        }
                    }
                }

                else{  // se primesc date pe un socket <--- recv
                    memset (buffer, 0 ,BUFLEN);

                    if ((n = recv (i, buffer, BUFLEN, 0)) <= 0){

                        if (n == 0)
                            printf("Quitting %d\n", countcli);
                        else
                            perror("Did not receive message");

                        close(i);
                        FD_CLR(i, &read_fds);  
                    }
                    else{ // recv intoarce >0
                    // se primeste mesajul in buffer
                        printf("ATM>");
                        
                        char *p;
                        int ok = 0;

                        // comanda login:
                        if ((p = strstr(buffer, "login")) != NULL){
                        
                            fprintf(log, "\n%s", buffer);
                            char *tok = strtok(p+6, " ");

                            if (clnts[k-1] == 1){
                                printf(" -2: Sesiune deja deschisa\n");
                                fprintf(log, "ATM> -2: Sesiune deja deschisa\n");
                                continue;
                            }
                            
                            if (strcmp(tok, cl[k-1].client_cardno) == 0) 
                                ok = 1;
                            else{
                                ok = 0;
                                printf("-4: Numar card inexistent.\n");
                                fprintf(log, "ATM> -4: Numar card inexistent\n");

                            }

                            tok = strtok(NULL, "\n");
                            if (ok == 1 && strcmp(tok, cl[k-1].client_cardpin) == 0)
                                ok = 1;
                            else if(ok == 0)
                                continue;
                            else{
                                ok = 0;
                                if (pin == 2){  // daca pinul a fost gresit de 3 ori
                                    printf("-5: Card blocat.\n");
                                    fprintf(log, "ATM> -5: Card blocat.\n");
                                }
                                else{
                                    printf(" -3: Pin incorect.\n");
                                    fprintf(log, "ATM> -3: Pin incorect\n");
                                    pin++;
                                }

                            }

                            if (ok == 1){  // clientul s-a conectat, clnts[client_curent] = 1 <-- evidenta
                                printf("Welcome, %s %s\n", cl[k-1].client_name, cl[k-1].client_surname);
                                fprintf(log, "ATM> Welcome, %s %s\n", cl[k-1].client_name, cl[k-1].client_surname);
                                curr = k-1;
                                clnts[k-1] = 1;  // daca clientul curent s-a conectat, pe indicele lui in vector se pune 1
                            }
                        }

                        // comanda logout:
                        else if ((p = strstr(buffer, "logout")) != NULL){
                            fprintf(log, "\n%s", buffer);
                            if (clnts[curr] == 1){  // daca pe client e 1, inseamna ca e conectat si se poate deloga
                                printf("Deconectare de la bancomat.\n");
                                fprintf(log, "ATM>Deconectare de la bancomat.\n");
                                clnts[curr] = 0;
                            }
                            else{  // altfel, inseamna ca nu e conectat
                                printf(" -1: Clientul nu este autentificat.\n");
                                fprintf(log, "ATM> - 1: Clientul nu este autentificat.\n");
                            }
                        }

                        //comanda listsold:
                        else if ((p = strstr(buffer, "listsold")) != NULL){
                            fprintf(log, "\n%s", buffer);
                            if (clnts[curr] != 0){  
                                printf("%0.2f\n", cl[k-1].client_sold);
                                fprintf(log, "ATM> %0.2f\n", cl[k-1].client_sold);
                            }
                        }

                        // comanda getmoney
                        else if ((p = strstr(buffer, "getmoney")) != NULL){
                            fprintf(log, "\n%s", buffer);
                            int suma = atoi(p+9);
                            if (suma % 10 != 0){
                                printf(" -9: Suma nu este muliplu de 10.\n");
                                fprintf(log, "ATM> -9: Suma nu este muliplu de 10.\n");
                            }
                            else if (suma > cl[k-1].client_sold){
                                printf(" -8: Fonduri insuficiente\n");
                                fprintf(log, "ATM> -8: Fonduri insuficiente\n");
                            }
                            else{
                                printf("Suma %d retrasa cu succes.\n", suma);
                                fprintf(log, "ATM> Suma %d retrasa cu succes.\n", suma);
                                cl[k-1].client_sold -= suma;
                            }
                        }

                        //comanda putmoney:
                        else if ((p = strstr(buffer, "putmoney")) != NULL){
                            fprintf(log, "\n%s", buffer);
                            int suma = atoi(p+9);
                            cl[k-1].client_sold += suma;
                            printf("Suma depusa cu succes.\n");
                        }

                        // comanda quit
                        else if ((p = strstr(buffer, "quit")) != NULL){
                               fprintf(log, "\n%s", buffer);
                               fclose(log);
                        }
                    }

                }

            }
            
        }
    }
  
    close(sockfd);

    return 0;
}