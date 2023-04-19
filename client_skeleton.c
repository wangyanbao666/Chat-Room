#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number

static int sockfd;

void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    printf("WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    printf("#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    printf("Or input messages sending to everyone in the chatroom.\n");
}

void recv_server_msg_handler() {
    /********************************/
	/* receive message from the server and desplay on the screen*/
	/**********************************/
	char buffer[MAX];
    int nbytes;

    while (1) {
        bzero(buffer, sizeof(buffer));
        nbytes = recv(sockfd, buffer, sizeof(buffer), 0);
        if (nbytes == -1) {
            perror("recv");
            break;
        } else if (nbytes == 0) {
            printf("Server disconnected.\n");
            break;
        } else {
            printf("%s", buffer);
        }
    }

    // Clean up and exit
    close(sockfd);
    pthread_exit(NULL);

}

int main(){
    int n;
	int nbytes;
	struct sockaddr_in server_addr, client_addr;
	char buffer[MAX];
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(1);
    }


	generate_menu();
	// recieve welcome message to enter the nickname
    bzero(buffer, sizeof(buffer));
    if (nbytes = recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        perror("recv");
    }
    printf("%s", buffer);

	/*************************************/
	/* Input the nickname and send a message to the server */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is the register/login message*/
	/*******************************************/
	char nickname[MAX];
	printf("Enter your desired nickname: ");
	fgets(nickname, MAX, stdin);
	nickname[strcspn(nickname, "\n")] = 0;

	// Concatenate "REGISTER" before the nickname to notify the server
	char register_msg[MAX];
	snprintf(register_msg, sizeof(register_msg), "REGISTER %s", nickname);

	// Send the registration message to the server
	if (send(sockfd, register_msg, strlen(register_msg) + 1, 0) == -1) {
		perror("send");
		exit(1);
	}



    // receive welcome message "welcome xx to joint the chatroom. A new account has been created." (registration case) or "welcome back! The message box contains:..." (login case)
    bzero(buffer, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        perror("recv");
    }
    printf("%s", buffer);

    /*****************************************************/
	/* Create a thread to receive message from the server*/
	/* pthread_t recv_server_msg_thread;*/
	/*****************************************************/
	pthread_t recv_server_msg_thread;
    pthread_create(&recv_server_msg_thread, NULL, (void *)recv_server_msg_handler, NULL);

    
	// chat with the server
	for (;;) {
		bzero(buffer, sizeof(buffer));
		n = 0;
		while ((buffer[n++] = getchar()) != '\n')
			;
		if ((strncmp(buffer, "EXIT", 4)) == 0) {
			printf("Client Exit...\n");
			/********************************************/
			/* Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
			if (send(sockfd, buffer, sizeof(buffer), 0) < 0) {
				perror("send");
			}
			pthread_cancel(recv_server_msg_thread);
            close(sockfd);
            break;
		}
		else if (strncmp(buffer, "WHO", 3) == 0) {
			printf("Getting user list, pls hold on...\n");
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_WHO failed");
				exit(1);
			}
			printf("If you want to send a message to one of the users, pls send with the format: '#username:message'\n");
		}
		else if (strncmp(buffer, "#", 1) == 0) {
			// If the user want to send a direct message to another user, e.g., aa wants to send direct message "Hello" to bb, aa needs to input "#bb:Hello"
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				printf("Sending direct message failed...");
				exit(1);
			}
		}
		else {
			/*************************************/
			/* Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/
			buffer[strcspn(buffer, "\n")] = 0;  // Remove the newline character

			char broadcast_msg[MAX];
			snprintf(broadcast_msg, sizeof(broadcast_msg), "%s: %s", nickname, buffer);

			// Send the formatted message to the server
			if (send(sockfd, broadcast_msg, strlen(broadcast_msg) + 1, 0) == -1) {
				perror("send");
				exit(1);
			}

			
		}
	}
	return 0;
}

