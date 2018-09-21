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

string sendrcv_TCP(string strout = ""){
	#define init_tmout 15

	const int portno = 8234;
	const char host[] = "192.168.1.109";
	const string OK= "\e[92m[ OK ]\e[39m",ER="\e[93m[ ER ]\e[39m";

	static int tcpsoc = 0;					//fd														
	static struct sockaddr_in serv_addr;			// sizeof(serv_addr) = 16?
	static struct hostent *server;
	static bool COM_OK = false;

	fd_set wbuff;

	struct timeval tv;
	tv.tv_sec = 15;
	char buffer[256] = "";
	int Er;

	if(tcpsoc == 0){					// automatically reset,bind soc, if write failed
		int tmout = 0;
		bool init_cmpltd = false;
		do{
			tmout++;
			if(tmout > init_tmout){
				cout<<"soc.init timeout!" + ER<<endl;
				return "";
			}
			tcpsoc = socket(AF_INET, SOCK_STREAM, 0);
			fcntl(tcpsoc, F_SETFL, O_NONBLOCK);			// set nonblocking
    			if (tcpsoc < 0) {
				perror("socket def :");
				exit(0);
			}
   			server = gethostbyname(host);
   			bzero((char *) &serv_addr, sizeof(serv_addr));
   			serv_addr.sin_family = AF_INET;
   			bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
   			serv_addr.sin_port = htons(portno);
			if(connect(tcpsoc,(const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0 ){	
        			if (errno != EINPROGRESS){
					perror("connect() failed ");
            			}
				init_cmpltd = true;
			}
			else{
				init_cmpltd = true;
			}
		}while(!init_cmpltd);
	}
	if(strout != ""){	// WRITE 2 SOCK.
		FD_ZERO(&wbuff);
		FD_SET(tcpsoc, &wbuff);
		Er = select(tcpsoc + 1, NULL, &wbuff, NULL, &tv);			
		if(Er <= 0){					// buffer is overflowing
			perror("Write select error :");
			sleep(0.2);
			tcpsoc = 0;
			return "";
		}
		strcpy(buffer, strout.c_str());
		Er = write(tcpsoc,buffer,strlen(buffer));
		if (Er < 0){					// error when writing
			if(COM_OK)
				cout<<"COM. LINK STOPPED"<<ER<<endl;
			else{
				perror("Connecting :\e[93m[ ER ]\e[39m");
			}
			COM_OK = false;	
			tcpsoc = 0;	
			return "";
		}
		else{
			if(!COM_OK)
				cout<<"COM. LINK STARTED"<<OK<<endl;
			COM_OK = true;
			cout<<"MSG SENT."<<endl;
		}
	}
	else{	// READ SOCK.
   		recv(tcpsoc, buffer, sizeof(buffer),0);
		return buffer;
	}
	return "";
}
int main(){
	while(true){
		sendrcv_TCP("hello ");
		string msg = sendrcv_TCP();
		if(msg.length() > 0){
			cout<<"rcvd :"<<msg;
		}
		sleep(0.1);
	}
	return 0;
}
