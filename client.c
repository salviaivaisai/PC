#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 256


int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr, udp_sock, from_station;
  
    unsigned int sd_len;

    char buffer[BUFLEN];
   

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");

    int sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd == -1)
        perror("Error UDP socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);

    udp_sock.sin_family = AF_INET;
    udp_sock.sin_port = atoi(argv[2]);
    inet_aton(argv[1], &(udp_sock.sin_addr) );

    int ok=0;


    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR connecting");    

    fd_set read_fds;
    fd_set tmp_fds;

    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    FD_SET(sd, &read_fds);

    while(1){

        tmp_fds = read_fds;

        if (select (sockfd+1, &tmp_fds, NULL, NULL, NULL) < 0)
            perror ("select error");

        if (FD_ISSET(0, &tmp_fds)){
            memset (buffer, 0 , BUFLEN);
            fgets (buffer, BUFLEN, stdin);

            // socketul UDP pe unde vine comanda de unlock
            if (strstr(buffer, "unlock") != NULL){
                sendto (sd, &buffer, strlen(buffer), 0, (struct sockaddr*)&udp_sock, sizeof(struct sockaddr_in));
            }
            else if (ok == 1){
                sendto (sd, &buffer, strlen(buffer), 0 , (struct sockaddr*)&udp_sock, sizeof(struct sockaddr_in));
            }
            // trimite restul comenzilor
            else{
                n = send (sockfd, buffer, strlen(buffer), 0);
                if (n < 0)
                    perror("error writing to socket");
            }
        }

        // primeste prin UDP aprobare de a introduce parola
        if (FD_ISSET (sd, &tmp_fds)){
            memset (buffer, 0 , BUFLEN);
            recvfrom (sd, buffer, BUFLEN, 0, (struct sockaddr*)&from_station, &sd_len);
            printf("%s", buffer);
            ok = 1;
        }

        // daca primeste ceva de la server:
        if (FD_ISSET (sockfd, &tmp_fds)){
            memset(buffer, 0, BUFLEN);
            n = recv (sockfd, buffer, BUFLEN, 0);
            if (n < 0){
                perror("did not receive answer from server");
                exit(1);
            }
            else{
                printf("received \"%s\" from server\n", buffer);
            }
        }
    }

    close(sockfd);

    return 0;
}