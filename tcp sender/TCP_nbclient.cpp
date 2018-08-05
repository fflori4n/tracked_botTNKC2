//
// 		TCP non-blocking client read/write

#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <unistd.h>
#include <string>
#include <iostream>
#include <fcntl.h>

using namespace std;

string sendrcv_TCP(int mode, string strout = ""){
	
	#define init_tmout 15

	const int portno = 8234;
	const char host[] = "192.168.1.200";
	const string OK= "\e[92m[ OK ]\e[39m",ER="\e[93m[ ER ]\e[39m";

	static int tcpsoc;					//fd														
	static struct sockaddr_in serv_addr;
	static struct hostent *server;

	fd_set wbuff;

	struct timeval tv;
	tv.tv_sec = 5;
	//tv.tv_usec = 0;
	char buffer[256] = "";
	int Er;

	if(mode == 0){	// WRITE 2 SOCK.
		FD_ZERO(&wbuff);
		FD_SET(tcpsoc, &wbuff);
		Er = select(tcpsoc + 1, NULL, &wbuff, NULL, &tv);				//chk if out buffer is overflowing
		if (Er > 0) {
			strcpy(buffer, strout.c_str());
			Er = write(tcpsoc,buffer,strlen(buffer));
			if (Er < 0)
				perror("Write socket error :");		
		}
		else{
			if (Er < 0) {
				perror("Write socket error :");
				sleep(1);
				return "";
			}
		}
	}
	else if (mode == 1){	// READ SOCK.
   		recv(tcpsoc, buffer, sizeof(buffer),0);
		return buffer;
	}
	else if (mode == 2){	// INIT SOCK.
		int tmout = 0;
		bool init_cmpltd;
		do{
			tmout++;
			if(tmout > init_tmout){
				cout<<"soc.init timeout!" + ER<<endl;
				return "";
			}
			sleep(1);
			tcpsoc = socket(AF_INET, SOCK_STREAM, 0);
			fcntl(tcpsoc, F_SETFL, O_NONBLOCK);
    			if (tcpsoc < 0) {
				perror("IN");
				init_cmpltd = false;
			}
   			server = gethostbyname(host);
   			bzero((char *) &serv_addr, sizeof(serv_addr));
   			serv_addr.sin_family = AF_INET;
   			bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
   			serv_addr.sin_port = htons(portno);
			connect(tcpsoc,(const sockaddr*)&serv_addr,sizeof(serv_addr));
			if (connect(tcpsoc,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0){
				perror("InitEr");
				init_cmpltd = false;
			}
			else
				init_cmpltd = true;
		}while(!init_cmpltd);
		cout<<"socket.init completed!" + OK <<endl;
	}
	return "";

}
int main(){
	sendrcv_TCP(2,"");
	while(true){
		sendrcv_TCP(0,"hello world");
		string msg = sendrcv_TCP(1,"");
		if(msg.length() > 0){
			cout<<"rcvd :"<<msg;
		}
		sleep(1);
	}
	return 0;
}
