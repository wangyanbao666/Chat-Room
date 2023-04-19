#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t listOfUsers[MAX_USERS] = {0}; // list of users
  

/* Add user to userList */
void user_add(user_info_t *user){
	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	/***************************/
	/* add the user to the list */
	/**************************/
    listOfUsers[users_count++] = *user;
}

/* Determine whether the user has been registered  */
int isNewUser(char* name) {
	int i;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/

	for (i = 0; i < users_count; i++) {
		if (strcmp(listOfUsers[i].username, name) == 0) {
			return 0; // User already exists
		}
    }

    return 1; // User is new
}

/* Get user name from userList */
char * get_username(int ss){
	int i;
	static char uname[MAX];
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	for (i = 0; i < users_count; i++) {
        if (listOfUsers[i].sockfd == ss) {
            strcpy(uname, listOfUsers[i].username);
            printf("get user name: %s\n", uname);
            return uname;
        }
    }

	return NULL;
}

/* Get user sockfd by name */
int get_sockfd(char *name){
	int i;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for (i = 0; i < users_count; i++) {
        if (strcmp(listOfUsers[i].username, name) == 0) {
            return listOfUsers[i].sockfd;
        }
    }

    return -1;
}

int get_state(char *name){
	int i;
	for (i = 0; i < users_count; i++) {
        if (strcmp(listOfUsers[i].username, name) == 0) {
            return listOfUsers[i].state;
        }
    }
    return -1;
}

void change_user_state(char *name, int flag){
	int i;
	for (i = 0; i < users_count; i++) {
        if (strcmp(listOfUsers[i].username, name) == 0) {
			if (flag == 1){
            	listOfUsers[i].state = 1;
			}
			else {
				listOfUsers[i].state = 0;
			}
			break;
        }
    }
}

// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}



int main(){
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof * pfds * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, u, rv;

    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
    // socket create and verification
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	
	
	bzero(&server_addr, sizeof(server_addr));
	
	// asign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	
	// Binding newly created socket to given IP and verification
	if ((bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
		printf("Socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener
	
	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);

		if (poll_count == -1) {
				perror("poll");
				exit(1);
		}

		// run through the existing connections looking for data to read
        	for(i = 0; i < fd_count; i++) {
            	  if (pfds[i].revents & POLLIN) { // we got one!!
                    if (pfds[i].fd == listener) {
                      /**************************/
					  /* we are the listener and we need to handle new connections from clients */
					  /****************************/
						addr_size = sizeof(client_addr);
						newfd = accept(listener, (struct sockaddr*)&client_addr, &addr_size);
						
						if (newfd == -1) {
							perror("accept");
						} else {
							add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
							printf("pollserver: new connection from %s on socket %d\n",inet_ntoa(client_addr.sin_addr),newfd);
						} 

						// send welcome message
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.\n");
						if (send(newfd, buffer, sizeof(buffer), 0) == -1)
							perror("send");
                      }
                    
					else {
                        // handle data from a client
						bzero(buffer, sizeof(buffer));
                        if ((nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0)) <= 0) {
                          // got error or connection closed by client
                          if (nbytes == 0) {
                            // connection closed
                            printf("pollserver: socket %d hung up\n", pfds[i].fd);
							bzero(buffer, sizeof(buffer));
							strcpy(buffer, get_username(pfds[i].fd));
							strcat(buffer, " has left the chatroom\n");
							for (int j = 1; j < fd_count; j++) {
								if (pfds[j].fd != pfds[i].fd) {
									send(pfds[j].fd, buffer, strlen(buffer), 0);
								}
							}
							change_user_state(get_username(pfds[i].fd),0);

                          } else {
                            perror("recv");
                          }
						  close(pfds[i].fd); // Bye!
						  del_from_pfds(pfds, i, &fd_count);
                        } else {
                            // we got some data from a client
							if (strncmp(buffer, "REGISTER", 8)==0){
								printf("Got register/login message\n");
								/********************************/
								/* Get the user name and add the user to the userlist*/
								/**********************************/
								char name[MAX];
    							sscanf(buffer, "REGISTER %s", name);
								user_info_t newUser;
								newUser.sockfd = newfd;
								strncpy(newUser.username,name,MAX);
								newUser.state = 1; 
								if (isNewUser(name) == 1) {
									/********************************/
									/* it is a new user and we need to handle the registration*/
									/**********************************/
									user_add(&newUser);
									/********************************/
									/* create message box (e.g., a text file) for the new user */
									/**********************************/
									char filename[MAX];
									snprintf(filename, sizeof(filename), "%s.txt", name);
									FILE *fp = fopen(filename, "w");

									if (fp == NULL) {
										perror("Error opening the file");
									} else {
										fclose(fp);
									}

									printf("a new user created\n");

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome ");
									strcat(buffer, name);
									strcat(buffer, " to join the chat room!\n");
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/

									for (int k = 1; k < fd_count; k++) {
										if (pfds[k].fd != pfds[i].fd && pfds[k].fd != listener) {
											send(pfds[k].fd, buffer, strlen(buffer), 0);
										}
									}

									/*****************************/
									/* send registration success message to the new user*/
									/*****************************/
									char success_message[] = "Registration successful! Welcome to the chat room.\n";
        							send(pfds[i].fd, success_message, strlen(success_message), 0);
								}
								else {
									/********************************/
									/* it's an existing user and we need to handle the login. Note the state of user,*/
									/**********************************/
									change_user_state(name, 1);

									/********************************/
									/* send the offline messages to the user and empty the message box*/
									/**********************************/
									char filename[MAX];
									snprintf(filename, sizeof(filename), "%s.txt", name);

									FILE *fp = fopen(filename, "r");
									if (fp != NULL) {
										char offline_msg[MAX];
										while (fgets(offline_msg, MAX, fp) != NULL) {
											send(newfd, offline_msg, strlen(offline_msg), 0);
										}
										fclose(fp);

										// Empty the message box
										fp = fopen(filename, "w");
										fclose(fp);
									}
								

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcat(buffer, name);
									strcat(buffer, " is online!\n");
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									for (int u = 1; u < fd_count; u++) {
										if (pfds[u].fd != pfds[i].fd) {
											send(pfds[u].fd, buffer, strlen(buffer), 0);
										}
									}
								}
							}
							else if (strncmp(buffer, "EXIT", 4)==0){
								printf("Got exit message. Removing user from system\n");
								// send leave message to the other members
                                bzero(buffer, sizeof(buffer));
								strcpy(buffer, get_username(pfds[i].fd));
								strcat(buffer, " has left the chatroom\n");
								/*********************************/
								/* Broadcast the leave message to the other users in the group*/
								/**********************************/
								for (int j = 1; j < fd_count; j++) {
									if (pfds[j].fd != pfds[i].fd) {
										send(pfds[j].fd, buffer, strlen(buffer), 0);
									}
								}

								/*********************************/
								/* Change the state of this user to offline*/
								/**********************************/
								change_user_state(get_username(pfds[i].fd),0);
								
								//close the socket and remove the socket from pfds[]
								close(pfds[i].fd);
								del_from_pfds(pfds, i, &fd_count);
							}
							else if (strncmp(buffer, "WHO", 3)==0){
								// concatenate all the user names except the sender into a char array
								// char ToClient[MAX];
								printf("Got WHO message from client.\n");
								bzero(buffer, sizeof(buffer));
								/***************************************/
								/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
								/* The state of each user (online or offline)should be labelled.*/
								/***************************************/
								for (int v = 0; v < users_count; v++) {
									char status[MAX];
									snprintf(status, sizeof(status), "%s is %s\t", listOfUsers[v].username, (listOfUsers[v].state == 1) ? "online" : "offline");
									strcat(buffer, status);
								}
								strcat(buffer, "\n");
								send(pfds[i].fd, buffer, strlen(buffer), 0);
							}
							else if (strncmp(buffer, "#", 1)==0){
								// send direct message 
								// get send user name:
								printf("Got direct message.\n");
								// get which client sends the message
								char sendname[MAX];
								// get the destination username
								char destname[MAX];
								// get dest sock
								int destsock;
								// get the message
								char msg[MAX];
								/**************************************/
								/* Get the source name xx, the target username and its sockfd*/
								/*************************************/
								int items_read = sscanf(buffer, "#%1023[^:]: %1023[^\n]", destname, msg);
								
								if (items_read != 2){
									bzero(buffer,sizeof(buffer));
									strcpy(buffer,"Please follow the pattern #username: msg\n");
									send(pfds[i].fd, buffer, strlen(buffer), 0);
								}
								else {
									strcpy(sendname, get_username(pfds[i].fd));
									destsock = get_sockfd(destname);

									if (destsock == -1) {
										/**************************************/
										/* The target user is not found. Send "no such user..." messsge back to the source client*/
										/*************************************/
										char not_found_msg[MAX];
										snprintf(not_found_msg, sizeof(not_found_msg), "No such user %s. Please check the username and try again.\n", destname);
										send(pfds[i].fd, not_found_msg, strlen(not_found_msg), 0);
									}
									else {
										// The target user exists.
										// concatenate the message in the form "xx to you: msg"
										char sendmsg[MAX];
										strcpy(sendmsg, sendname);
										strcat(sendmsg, " to you: ");
										strcat(sendmsg, msg);
										strcat(sendmsg, "\n");
										// printf("%s\n", destname);
										// printf("%s\n", msg);

										/**************************************/
										/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
										/* For the offline case, send "...Leaving message successfully" message to the source client*/
										/*************************************/
										if (get_state(destname) == 1) {
											send(destsock, sendmsg, strlen(sendmsg), 0);
										} else {
											char filename[MAX];
											snprintf(filename, sizeof(filename), "%s.txt", destname);
											FILE *fp = fopen(filename, "a");
											if (fp != NULL) {
												fprintf(fp, "%s\n", sendmsg);
												fclose(fp);

												// Send a "Leaving message successfully" message to the source client
												char success_msg[MAX];
												snprintf(success_msg, sizeof(success_msg), "Leaving message successfully for offline user %s.\n", destname);
												send(pfds[i].fd, success_msg, strlen(success_msg), 0);
											}
										}
									}
								}
							}
							else{
								printf("Got broadcast message from user\n");
								/*********************************************/
								/* Broadcast the message to all users except the one who sent the message*/
								/*********************************************/
								char broadcast_msg[MAX];
								snprintf(broadcast_msg, sizeof(broadcast_msg), "%s\n", buffer);
								
								for (int j = 0; j < fd_count; j++) {
								// Skip the listener and the sender
									if (pfds[j].fd == listener || pfds[j].fd == pfds[i].fd) {
										continue;
									}
									// char destname[MAX];
									// strcpy(destname, get_username(pfds[j].fd));
									// if (get_state(destname) == 1){
									send(pfds[j].fd, broadcast_msg, strlen(broadcast_msg), 0);
									// }
									// else {
									// 	char filename[MAX];
									// 	snprintf(filename, sizeof(filename), "%s.txt", destname);
									// 	FILE *fp = fopen(filename, "a");
									// 	if (fp != NULL) {
									// 		fprintf(fp, "%s\n", broadcast_msg);
									// 		fclose(fp);
									// 	}

									// }
								}   
							}
                        
                    } // end handle data from client
                  } // end got new incoming connection
				}
			} // end looping through file descriptors
        } // end for(;;) 
		
	return 0;
}
