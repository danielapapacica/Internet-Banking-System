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

void error(char *msg)
{
    perror(msg);
    exit(1);
}


typedef struct{
	char nume[12];
	char prenume[12];
	long nr_card;
	int pin;
	char parola[8];
	double sold;
	char logat;	// 0 sau 1, pentru a putea sti ce clienti sunt autentificati
	char blocat; // 0 sau 1, pentru a putea sti ce clienti au contul blocat
	char nr_incercari;
} TUser;



int main(int argc, char *argv[])
{

	// citire useri din fisier
	int N;
	FILE* in = fopen(argv[2], "rt");
	fscanf(in, "%d", &N);
	TUser* users = malloc((N+1) * sizeof(TUser));
	for(int i = 1; i <= N; i++){
		fscanf(in, "%s ", users[i].nume);
		fscanf(in, "%s ", users[i].prenume);
		fscanf(in, "%lu ", &(users[i].nr_card));
		fscanf(in, "%d ", &(users[i].pin));
		fscanf(in, "%s ", users[i].parola);
		fscanf(in, "%lf ", &(users[i].sold));
		users[i].logat = 0;
		users[i].blocat = 0;
		users[i].nr_incercari = 0;
	}



     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     int n, i, j;

     fd_set read_fds;	//multimea de citire folosita in select()
     fd_set tmp_fds;	//multime folosita temporar 
     int fdmax;		//valoare maxima file descriptor din multimea read_fds

     if (argc < 2) {
         fprintf(stderr,"Usage : %s port\n", argv[0]);
         exit(1);
     }

     //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     
     portno = atoi(argv[1]);

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd, MAX_CLIENTS);

     //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
     FD_SET(sockfd, &read_fds);
     FD_SET(STDIN_FILENO, &read_fds);
     fdmax = sockfd;




     int users_shortcut[fdmax+1];	// retin sesiunile curente de pe server
     int all_clients[fdmax+1];	// retin sesiunile curente de pe server
     int nr_clients = 0;

     for(i = 0; i <= fdmax; i++) {
     	users_shortcut[i] = 0;
     	all_clients[i] = 0;
     }



     // main loop
	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				 if(i == STDIN_FILENO){		// comanda pentru server
				 	memset(buffer, 0, BUFLEN);
				 	scanf("%s", buffer);

				 	
				 	if(!strcmp(buffer, "quit")){	// quit de la server
				 		memset(buffer, 0, BUFLEN);
				 		memcpy(buffer, "Conexiune inchisa de server", BUFLEN);
				 		for(int j = 0; j <= nr_clients; j++){
				 				int r = send(all_clients[j], buffer , strlen(buffer), 0);	// notificam clientii

				 		}

				 		fclose(in);
     					close(sockfd);
   
     					return 0; 				// inchidem serverul

				 	}
						

				}
				else if (i == sockfd) {



					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}

					all_clients[nr_clients] = newsockfd;	// lista a tuturor conexiunilor cu clientii
					nr_clients++;
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
					
				else {


					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("selectserver: socket %d hung up\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					} 
					
					else { //recv intoarce >0
						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);



						// parsare comanda
						const char delim[2] = " ";
						char *comanda;
						comanda = strtok(buffer, delim);
						int client_out = 0;


						if(!strcmp(comanda, "login")){

							long nr_card = atoi(strtok(NULL, delim));
							int pin = atoi(strtok(NULL, delim));
							int exista_nr_card = 0;

							memset(buffer, 0, BUFLEN);

							for(int j = 1; j <= N; j++){
								if(users[j].nr_card == nr_card){
									exista_nr_card = 1;

									if(users[j].blocat == 1){
										memcpy(buffer, "IBANK>: -5 : Card blocat", BUFLEN);	// card blocat

									}else if(users[j].logat == 1) {
										memcpy(buffer, "IBANK>: -2 : Sesiune deja deschisa", BUFLEN);	// client deja logat

									}else if(users[j].pin != pin) {
										users[j].nr_incercari++;
										if(users[j].nr_incercari == 3){
											users[j].blocat = 1;
											memcpy(buffer, "IBANK>: -5 : Card blocat", BUFLEN);	// card blocat
										}else{
											memcpy(buffer, "IBANK>: -3 : Pin gresit", BUFLEN);	// pin gresit
										}

									}else{	// pin bun

										strcat(buffer, "IBANK>: Welcome ");
										strcat(buffer, users[j].nume);
										strcat(buffer, " ");
										strcat(buffer, users[j].prenume);			

											// pin corect
										users[j].nr_incercari = 0;
										users[j].logat = 1;

										users_shortcut[i] = j;
									}


									break;
								}
							}


							if(exista_nr_card == 0){
								memcpy(buffer, "IBANK>: -4 : Numar card inexistent", BUFLEN);	// numar card inexistent
							}

						}
						else if(!strcmp(comanda, "logout\n")){	// LOGOUT
							memset(buffer, 0, BUFLEN);

							if(users_shortcut[i] == 0){
								memcpy(buffer, "-1 : Clientul nu este autentificat", BUFLEN);	// clientul nu este autentificat
							}else{
								int j = users_shortcut[i];
								users[j].logat = 0;
								users_shortcut[i] = 0;
								memcpy(buffer, "IBANK> Clientul a fost deconectat", BUFLEN);
							}

						}else if(!strcmp(comanda, "listsold\n")){		// LISTSOLD

							if(users_shortcut[i] == 0){
								memcpy(buffer, "-1 : Clientul nu este autentificat", BUFLEN);	// clientul nu este autentificat
							}else{
								sprintf(buffer, "IBANK> %.2f", users[users_shortcut[i]].sold);
							}



						}else if(!strcmp(comanda, "transfer")){			// TRANSFER


							long nr_card = atoi(strtok(NULL, delim));
							double sold = atof(strtok(NULL, delim));
							int exista_nr_card = 0;

							if(users_shortcut[i] == 0){
								memcpy(buffer, "-1 : Clientul nu este autentificat", BUFLEN);	// clientul nu este autentificat
							}else{
								
								for(int j = 1; j <= N; j++){
									if(users[j].nr_card == nr_card){
										exista_nr_card = 1;
										if(users[users_shortcut[i]].sold < sold){
											memcpy(buffer, "IBANK> −8 : Fonduri insuficiente", BUFLEN);		// suma prea mare
										}
										else{

											sprintf(buffer, "Transfer %.2f catre %s %s? [y/n]", sold, users[j].nume, users[j].prenume);
											int r1 = send(i, buffer , strlen(buffer), 0);
											memset(buffer, 0, BUFLEN);

											int r2 = recv(i, buffer, sizeof(buffer), 0);
											if(!strcmp(buffer, "y\n")){
												memset(buffer, 0, BUFLEN);
												memcpy(buffer, "IBANK> Transfer realizat cu succes", BUFLEN);

												users[users_shortcut[i]].sold -= sold;
												users[j].sold += sold;
											}else{
												memset(buffer, 0, BUFLEN);
												memcpy(buffer, "IBANK> -9 : Operatie anulata", BUFLEN);
											}

										}

									}
								}

								if(exista_nr_card == 0){
								memcpy(buffer, "IBANK>: -4 : Numar card inexistent", BUFLEN);	// numar card inexistent
							}

							}


						}else if(!strcmp(comanda, "quit\n")){	// deconectarea clientului
							users[users_shortcut[i]].logat = 0;
							users_shortcut[i] = 0;
							client_out = 1;
						}

						if(!client_out){
							int r = send(i, buffer , strlen(buffer), 0);
						}
					}
				} 
			}
		}

     }

     fclose(in);
     close(sockfd);
   
     return 0; 
}


