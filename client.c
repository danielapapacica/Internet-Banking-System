#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>


#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  
   
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    


    fd_set read_fds;
    fd_set tmp_fds;  
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(0, &read_fds);
    FD_SET(sockfd, &read_fds);



    char file_name[50];

    sprintf(file_name, "client-%d.log", getpid());

    FILE *out = fopen(file_name, "wt");

    while(1){
        //citesc de la tastatura
        tmp_fds = read_fds;
        if (select(sockfd + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
            error("ERROR in select");
    
        if (FD_ISSET(0, &tmp_fds)) {

            memset(buffer, 0 , BUFLEN);
            fgets(buffer, BUFLEN-1, stdin);



            //trimit mesaj la server
            n = send(sockfd,buffer,BUFLEN, 0);
            fprintf(out, "%s", buffer);               // scriu comanda in fisierul de log

            if(!strcmp(buffer, "quit\n"))
                break;

            if (n < 0) 
                 error("ERROR writing to socket");

        } else {
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd, buffer, BUFLEN, 0);


            if(!strcmp(buffer, "Conexiune inchisa de server")){
                printf("%s\n", buffer);             // scriu raspunsul in consola
                break;
            }

            if (n < 0) 
                 error("ERROR writing to socket");
            else{
                fprintf(out, "%s\n", buffer);       // scriu raspunsul in fisierul de log
                printf("%s\n", buffer);             // scriu raspunsul in consola
            }
        }

    }

    fclose(out);
    close(sockfd);
    return 0;
}


