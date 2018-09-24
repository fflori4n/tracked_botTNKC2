#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h> 
#include <netdb.h> 

using namespace std;

string comArduino(string outmsg = ""){

	#define comports_size 3

	static int fd_ard = 0, Er;
	static bool init = false;
	char buffer[256];
	
	if(!init || fd_ard == 0){

		struct termios SerialPortSettings;  						  // Create terminos struct   
		string comports[] = { "/dev/ttyUSB0","/dev/ttyUSB1","/dev/ttyUSB2" };

		for(int i = 0; i < comports_size; i++){

			fd_ard = open(comports[i].c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    		if (fd_ard != -1) {
				cout<<"connected to "<<comports[i]<<endl;
				break;
    		}
			else{
				perror ("open");
			}
			if(i == (comports_size - 1)){
				cout<<"can't connect to arduino [ ER ]\n"<<endl;
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
	if(outmsg != ""){
		Er = write(fd_ard,outmsg.c_str(),strlen(outmsg.c_str()));
		if(Er < strlen(outmsg.c_str())){
			perror ("write ");
			init = false;
		}
		else
			cout<<"sent to ard."<<endl;
		return "";
	}
	else{	// read
		Er = read(fd_ard, buffer, sizeof(buffer));
		if(Er < 0){
			//perror("read ");
			//cout<<buffer<<endl;
			return "";
		}
		cout<<buffer<<endl;
		return buffer;
	}
}
int main(){
	bool flg = false;
	while(1){
		flg = !flg;
		if(flg)
			comArduino("R40,100");
		else
			comArduino("R320,100");
		sleep(4);
		comArduino();
		sleep(1);
	}
	return 0;
}
