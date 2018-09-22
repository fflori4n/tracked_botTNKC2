#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
//#define max_cons 5


using  namespace std;

string TCPserv(bool init,string msgout = ""){

	#define wlcommsg "U R CONNECTED! \e[92m[ OK ]\e[39m \n"
	#define conlmtmsg "Con. lmt reached! \e[93m[ ER ]\e[39m\n"
	#define PORT 8234
	
	#define max_cons 5

	const string OK= "\e[92m[ OK ]\e[39m",ER="\e[93m[ ER ]\e[39m";

	char buffer[256];
	fd_set readfds;
	int opt = 1;
	int Er;
    static int master_socket, addrlen, new_socket, activity, valread, sd;
	static int client_socket[max_cons];
    static int max_sd;
    static struct sockaddr_in address;
	
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;		// might have to tweek this shiiet

	if(init){	// start server
		for (int i = 0; i < max_cons; i++){		// zero out file desc. array
			client_socket[i] = 0;
		}

		if((master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0){//create a master socket
			perror("socket failed");
			exit(0);
		} 
		//********* set master socket to allow multiple connections , this is just a good habit, it will work without this
		if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
			perror("setsockopt");
			exit(0);
		}
		//*********
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons( PORT );
		if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0){ // bind the master sock.
			perror("bind failed");
			exit(0);
		}
		cout<<"listening on port:"<<PORT<<endl;

		if (listen(master_socket, 3) < 0){
			perror("listen");
			exit(0);
		}
		addrlen = sizeof(address);
		cout<<"Waiting for connections ..."<<endl;
	}
	FD_ZERO(&readfds);						//clear the socket set
    FD_SET(master_socket, &readfds);		// add master sock.
    max_sd = master_socket;
    for (int i = 0 ; i < max_cons ; i++){	// add clients
        sd = client_socket[i]; 		//if valid socket descriptor then add to read list
        if(sd > 0)
           FD_SET( sd , &readfds);
        if(sd > max_sd)				//highest file descriptor number, need it for the select function
           max_sd = sd;
        }
	activity = select( max_sd + 1 , &readfds , NULL , NULL , &tv);	// check for activity on all fds
	if ((activity < 0) && (errno!=EINTR)){	// EINTR - no activity
		printf("select error");
	}
	if(errno == EINTR && msgout == "") 	// shortcut if read and no new msg
		return "";
	if (FD_ISSET(master_socket, &readfds)){	// if activity is on master - incomeing con.
		if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
			perror("accept");
			exit(0);
		}
		cout<<"new con. accepted: fd:"<<new_socket<<" IP: "<<inet_ntoa(address.sin_addr)<<" "<<ntohs(address.sin_port);
		bool concmpltd = false;
		for (int i = 0; i < max_cons; i++) { //add new socket to array of sockets
			if( client_socket[i] == 0 ){	//if position is empty
				client_socket[i] = new_socket;
				cout<<" as "<<i<< endl;
				concmpltd = true;
				break;
			}
		}
		if(concmpltd)
			write(new_socket,wlcommsg,strlen(wlcommsg));
		else
			write(new_socket, conlmtmsg, strlen(conlmtmsg));
	}
	for (int i = 0; i < max_cons; i++){ // check all client sock.s for FD_ISSET
		if(client_socket[i] != 0){
			sd = client_socket[i];
			if(msgout != ""){
				strcpy(buffer, msgout.c_str());
				Er = write(sd,buffer,strlen(buffer));
				if(Er > 0)
					cout<<"MSG SENT."<<endl;
				
			}
			if(FD_ISSET(client_socket[i], &readfds)) { // chk if activity is on current cli.
				if ((valread = read( sd , buffer, 256)) == 0){			// read unsuccesfull + FD_ISSET = cli. left
					cout<<"Host discon. "<<inet_ntoa(address.sin_addr)<<" "<<ntohs(address.sin_port)<<endl;
					close(client_socket[i]);
					client_socket[i] = 0;	// mark as open slot
				}
				else{
					buffer[valread] = '\0';
					cout<<"rcvd: "<<buffer<<endl;
					send(sd , "OK\n" , strlen("OK\n") , 0 );
					return buffer;
				}
            }
        }
	}
	return "";
}
int main(int argc , char *argv[])
{ 
	TCPserv(true);
	while(1){
		TCPserv(false);
		TCPserv(false,"hello world! \n");
		sleep(1);
		//cout<<"runing"<<endl;
	}
      
    return 0;
} 
