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

#include <fcntl.h>
#include <termios.h> 
#include <netdb.h> 

using  namespace std;

const string OK= "\e[92m[ OK ]\e[39m",ER="\e[93m[ ER ]\e[39m";

string TCPserv(bool init,string msgout = ""){

	#define wlcommsg "U R CONNECTED! \e[92m[ OK ]\e[39m \n"
	#define conlmtmsg "Con. lmt reached! \e[93m[ ER ]\e[39m\n"
	#define PORT 8234
	//#define TCPverbose
	
	#define max_cons 5

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
		cout<<"listening on port:"<<PORT<<" "<<OK<<endl;

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
			if(strlen(msgout.c_str()) > 2){
				strcpy(buffer, msgout.c_str());
				Er = write(sd,buffer,strlen(buffer));
				if(Er < 0)
					perror("send er. ");
				
			}
			if(FD_ISSET(client_socket[i], &readfds)) { // chk if activity is on current cli.
				if ((valread = read( sd , buffer, 256)) == 0){			// read unsuccesfull + FD_ISSET = cli. left
					cout<<"Host discon. "<<inet_ntoa(address.sin_addr)<<" "<<ntohs(address.sin_port)<<endl;
					close(client_socket[i]);
					client_socket[i] = 0;	// mark as open slot
				}
				else{
					buffer[valread] = '\0';
					#ifdef TCPverbose
						cout<<"rcvd: "<<buffer<<endl;
						send(sd , "OK\n" , strlen("OK\n") , 0 );			// send acnl. msg
					#endif
					return buffer;
				}
            }
        }
	}
	return "";
}
string comArduino(string outmsg = ""){

	#define comports_size 3

	static int fd_ard = 0;
	static bool init = false;
	char buffer[256] = "";
	int Er;

	fd_set readfds;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;		// might have to tweek this shiiet
	
	if(!init || fd_ard == 0){

		struct termios SerialPortSettings;  						  // Create terminos struct   
		string comports[] = { "/dev/ttyUSB0","/dev/ttyUSB1","/dev/ttyUSB2" };

		for(int i = 0; i < comports_size; i++){

			fd_ard = open(comports[i].c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    		if (fd_ard != -1) {
				cout<<"connected to "<<comports[i]<<" "<<OK<<endl;
				TCPserv(false,"rpi: connected to " + comports[i] + "\n");
				break;
    		}
			else{
				perror ("open");
			}
			if(i == (comports_size - 1)){
				cout<<"can't connect to arduino [ ER ]\n"<<endl;
				TCPserv(false,"rpi: can't connect to ard. [ ER ]\n");
				return "";
			}
		}                      
		tcgetattr(fd_ard, &SerialPortSettings); 					  // Get the current attributes of the Serial port
		cfsetispeed(&SerialPortSettings,B115200); 					  // Set Read  Speed as 115200                       
		cfsetospeed(&SerialPortSettings,B115200); 					  // Set Write Speed as 115200  

		if((tcsetattr(fd_ard,TCSANOW,&SerialPortSettings)) != 0){ 	  // Set the attributes to the termios structure
  			printf("Error while setting attributes \n");
			return "";
		}
		init = true;
	}
	if(strlen(outmsg.c_str()) > 2){
		cout<<strlen(outmsg.c_str())<<endl;
		Er = write(fd_ard,outmsg.c_str(),strlen(outmsg.c_str()));
		if(Er < strlen(outmsg.c_str())){
			perror ("write ");
			TCPserv(false,"rpi: error sending.\n");
			init = false;
		}
		else{
			cout<<"sent to ard."<<endl;
			TCPserv(false,"rpi: sent.\n");
		}
		return "";
	}
	else{	// read
		for(int i= 0; i < 10; i++){
			Er = read(fd_ard, buffer, sizeof(buffer));
			if(Er < 0 && errno != EAGAIN){
				usleep(10000); 
				perror("read ");
			}
			else
				return buffer;
		}
	}
}
int main(int argc , char *argv[])
{ 

	string indata, outdata;
	TCPserv(true);
	while(1){
		indata = "";
		outdata = "";
		indata = TCPserv(false);
		if(strlen(indata.c_str()) > 0){
			//TCPserv(false,"OK\n");
			comArduino(indata);
			//sleep(.1);
		}
		outdata = comArduino();
		if(strlen(outdata.c_str()) > 0){
			cout<<outdata;
			TCPserv(false,outdata);
			//sleep(.1);
		}
		//cout<<"runing e"<<endl;
	}   
    return 0;
} 
