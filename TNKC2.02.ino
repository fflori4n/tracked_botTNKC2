#include <Wire.h> //I2C Arduino Library

#define _DSBL 6
#define _DIR1 5
#define _STP1 4
#define _DIR2 3
#define _STP2 2

#define _SCNR_A 7
#define _SCNR_B 8
#define _SCNR_C 9
#define _SCNR_D 10

#define _LED_BUILTIN 13

double ramp_prcnt = 50;
double Calc_intrpt = 200; 

bool MOT1 = false, MOT2 = false;
bool DIR1 = false, DIR2 = false;

struct pos{
  double HDG;
  //double PTC;
  byte PTC;
  //double ROL;
  byte ROL;
};
pos BOTPOS;

void setspeed(double compregval = 20){

  compregval = constrain(map(compregval, 0, 100, 1000, 50),50,1000);
  if(compregval > 14 && compregval < 10000){  // 15 
    OCR1A = compregval;                                   //(must be <65536) set compare match register
    Calc_intrpt = compregval; 
  }
  else{
    Serial.println(F("ER: Invalid Feed ! "));
  }
  sei();                                            //allow interrupts
  return;
}

ISR(TIMER1_COMPA_vect){                                     // -- ON TIMER1 INTRPT -- 
  // 0b76543210
  #define invdir1 true
  #define invdir2 false

  byte portval = 0b10000011;  //!!!
                            
  if(MOT1 || MOT2){
    PORTD &= 0b10000011;
    if(DIR1 == invdir1){  ///!!!!
      portval |= (1<<5);
    }                                           //0b00100000;
    if(DIR2 == invdir2){
      digitalWrite(_DIR2,1);          // fix this shiet! 
      //portval |= 0b00001000;//(1<<3);                         //0b00001000;
    } 
    if(MOT1){
      portval |= (1<<4);                                  //0b00010000;
      MOT1 = false;
    }
    if(MOT2){
      portval |= (1<<2);                                  //0b00000100;
      MOT2 = false;
    }
    PORTD |= portval;
    delayMicroseconds(40);
    portval = 0b11000011;                                 // clear everything
    PORTD &= portval;
  }
  return;
}
pos getPOS(int avrgscans = 3){
																	// WIRE.h !!!
	#define addr 0x1E 												//I2C Address for The HMC5883
	#define pi 3.1415
	//#define COMPDBG
	#define PTClmt 130
	#define ROLlmt 130

	Wire.begin();
	Wire.beginTransmission(addr); 									//start talking
	Wire.write(0x02); 												// Set the Register
	Wire.write(0x00); 												// Tell the HMC5883 to Continuously Measure
	Wire.endTransmission();

	pos dat;
	double x = 0,y = 0,z = 0; 										//mag. of field vectors
    int newx,newy,newz;
  
    for(int i = 0; i < avrgscans; i++){
		delayMicroseconds(2000);
        Wire.beginTransmission(addr);
        Wire.write(0x03); //start with register 3.
        Wire.endTransmission();
        Wire.requestFrom(addr, 6);      //Read the data.. 2 bytes for each axis.. 6 total bytes
		if(6<=Wire.available()){
			newx = Wire.read()<<8; //MSB  x 
			newx |= Wire.read(); //LSB  x
			newz = Wire.read()<<8; //MSB  z
			newz |= Wire.read(); //LSB z
 			newy = Wire.read()<<8; //MSB y
			newy |= Wire.read(); //LSB y

			x += newx;
			x/=2;
			y += newy;
			y/=2;
			z += newz;
			z/=2;
        }
    }
    dat.HDG=180 + (atan2(x,y)*180/pi);
    dat.PTC=atan2(y,z)*180/pi;
    /*dat.PTC = 0;
    dat.ROL = 0;
    if(abs(atan2(y,z)*180/pi) < PTClmt){
      dat.PTC = 1;
    }
    if(abs(atan2(x,z)*180/pi) < ROLlmt){
      dat.ROL = 1;
    }*/
    dat.ROL=atan2(x,z)*180/pi;
  
    #ifdef COMPDBG
      //Serial.print("HDG: ");
        Serial.print(dat.HDG);
      Serial.print(" ");
        //Serial.print(" PITCH: ");
        Serial.print(dat.PTC);
      Serial.print(" ");
        //Serial.print(" ROLL: ");
        Serial.println(dat.ROL);
	#endif
	return dat;
}
void setup() {
    Serial.begin(115200);
    Serial.println(F("Serial RDY")); 

  pinMode(_LED_BUILTIN, OUTPUT);
  pinMode(_DIR1, OUTPUT);
  pinMode(_DIR2, OUTPUT);
  pinMode(_STP1, OUTPUT);
  pinMode(_STP2, OUTPUT);
  pinMode(_DSBL, OUTPUT);

  pinMode(_SCNR_A, OUTPUT);
  pinMode(_SCNR_B, OUTPUT);
  pinMode(_SCNR_C, OUTPUT);
  pinMode(_SCNR_D, OUTPUT);

  cli();                                            //stop interrupts
  TCCR1A = 0;                                         // set entire TCCR1A register to 0
  TCCR1B = 0;                                         // same for TCCR1B
  TCNT1  = 0;                                         //initialize counter value to 0
  OCR1A = 50;                                         // = (16*10^6) / (600*1024) - 1 (must be <65536) // set compare match register for 1hz increments
  TCCR1B |= (1 << WGM12);                                   // turn on CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10);                              // Set CS12 and CS10 bits for 1024 prescaler
  TIMSK1 |= (1 << OCIE1A);                                  // enable timer compare interrupt
  sei();  

  digitalWrite(_DSBL, HIGH); 
  getPOS(true);

}
void step(double Lsteps, double Rsteps,int speed = 80,int targdeg = -1){
  #define lowspeed 200
  #define Rtolerance 2
  #define Rcrct_cnt 10
  #define Rchk_tuner 1.2
  double rampup = (min(abs(Lsteps),abs(Rsteps))/100)*ramp_prcnt;
  double rampdown = (min(abs(Lsteps),abs(Rsteps))-rampup);
  int Ramp_OCRA1;
  int s = 0;
	
  speed = constrain(speed,0,100);

  if(targdeg < 0){
    setspeed(speed);  // 80 for F/B 40 for turning
    for(int i = 0; i < i + 1; i){
      if(!(MOT1 || MOT2)){
        if(i < rampup){ //|| i > rampdown){
          Ramp_OCRA1 = constrain(map(s, 0, abs(rampup), lowspeed, Calc_intrpt),50,lowspeed);
        }
        else if(i > rampdown && Calc_intrpt < lowspeed){ //|| i > rampdown){
          Ramp_OCRA1 = constrain(map((min(abs(Lsteps),abs(Rsteps)) - s), 0, abs(rampup), lowspeed, Calc_intrpt),50,lowspeed);
        }
		cli();
        OCR1A = Ramp_OCRA1;
        sei();
        s++;
        if(Lsteps > 0){
          MOT1 = true;
          DIR1 = false;
          delay(0);
          Lsteps--;
        }else if(Lsteps < 0){
          MOT1 = true;
          DIR1 = true;
          delay(0);
          Lsteps++;
        }
        if(Rsteps > 0){
          MOT2 = true;
          DIR2 = false;
          delay(0);
          Rsteps--;
        }else if(Rsteps < 0){
          MOT2 = true;
          DIR2 = true;
          delay(0);
          Rsteps++;
        }
      }
      if((!Rsteps) && (!Lsteps))
        break;
    }
    digitalWrite(_DSBL, 1); 
  }
  else{             // TURN TO HDG									// 560 steps 90 deg
	int Rfinished = 0;
	
	setspeed(speed);
	while(true){
		BOTPOS = getPOS(20);
		Serial.println("turning");
		while(abs(BOTPOS.HDG - targdeg) > Rtolerance){
			Rfinished = 0;
			delay(200);
			BOTPOS = getPOS(20);
			//Serial.println((int)getPOS(20).HDG);
			
			DIR1 = true;
			DIR2 = false;
			int COMPN = BOTPOS.HDG;
			if(COMPN > 180){
				COMPN = -(360 - COMPN);
			}
			if(((targdeg - COMPN)*(pi/180)) < 360 && sin((targdeg - COMPN)*(pi/180)) > 0){			//set to turn CCW
				DIR1 = false;
				DIR2 = true;
				Serial.println("CCVturn");
			}

			Serial.println((int)BOTPOS.HDG);
			int Rchk = abs(min(abs(targdeg - COMPN),(360 - abs(targdeg - COMPN))));
			Rchk = map(Rchk,0,90,0,560);
			Rchk *= Rchk_tuner;
			//Serial.print(" Rchk :");
			//Serial.println(Rchk);
			
			rampup = (Rchk/100)*ramp_prcnt;
			rampdown = Rchk - rampup; 
			s = 0;
			for(int i = 0; i < (Rchk); i++){
				Serial.println((int)getPOS().HDG);
				if(Rchk > 100){
					if(i < rampup){
          				Ramp_OCRA1 = constrain(map(s, 0, abs(rampup), lowspeed, Calc_intrpt),50,lowspeed);
        			}
       				else if(i > rampdown){ //|| i > rampdown){
          				Ramp_OCRA1 = constrain(map((Rchk - s), 0, abs(rampup), lowspeed, Calc_intrpt),50,lowspeed);
        			}
				}else{
					Ramp_OCRA1 = lowspeed;
				}
				cli();												// so it wouldnt speed up when not exc nxt 3 cmds
          		OCR1A = Ramp_OCRA1;
          		sei();
				while(MOT1 || MOT2){
					delay(0);
				}
				MOT1= true;
				MOT2= true;
				s++;
			}
		}
		Rfinished++;
		if(Rfinished > Rcrct_cnt)
			break;
		
	}
    Serial.print(BOTPOS.HDG);
    Serial.print(" ");
    Serial.print(BOTPOS.PTC);
    Serial.print(" ");
    Serial.println(BOTPOS.ROL);
  }
}
void loop() {
  //MOT1 = true;
  //MOT2 = true;
  Serial.println(getPOS(50).HDG); 
  //step(600,600);
  //delay(1000);
  //step(-600,-600);
  delay(1000);
  //step(600,600,100);
  //step(-600,-600,100);
  //step(0,0,100,340);
  step(0,0,50,0);
}
