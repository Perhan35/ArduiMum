/*
 * File : projetMum
 * Description : Project's core
 * Developed by : Perhan Scudeller
 * E-mail address : perhan.scudeller@gmail.com
 * Licence : Copyright
 * Last update : 13.01.2020
 */

#include "config.h"

//#define DEBUGCOLOR
//#define DEBUG

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Booting from Arduino MEGA");
  #endif
  
  Ethernet.begin(mac, ip); // OR for STATIC IP : <no return> Ethernet.begin(mac,ip);
  server.begin();
  matrix.begin();

  matrix.drawPixel(0, 0, matrix.Color333(7, 7, 7)); 
  matrix.swapBuffers(false);
  delay(1000);

  matrix.setTextWrap(false);
  matrix.setTextSize(1); // size 1 => 8 pixels high 
  matrix.fillScreen(0); 

  onBoot();

  chooseServerNTP();
  delay(1); 
  adjustJetLag();

  displaySmiley(32,21);
  matrix.swapBuffers(false);
  delay(300);

/**** FOR TRIAL ONLY !!! *****/ 
  /*
  matrix.fillScreen(matrix.Color333(7,7,7));
  matrix.swapBuffers(false);
  delay(1000000);
  */

  
  #ifdef DEBUG
  Serial.println("Booted");
  #endif
}


/****************** LOOP *********************/ 
void loop() {

  serverCallback();
  delay(1); //1ms
  
  if (screenState) { //screen on
	  delay(1);
    tempWeathHandler();
    delay(1);
    displayTime();
    delay(1);
    regulTempWeath();
    delay(1);
    if(displayBus){
        if(displayMessage == false){
          busHandler();
          delay(1);
          regulBus();
          delay(1); //1ms
        }
    }else{
        if(displayMessage){
              displayMsg();
              delay(1);
        }else{
              matrix.fillRect(0, 16, 64, 16, dark);
              matrix.swapBuffers(false);
              //delay(1000);
        }
    }
  }else{ //screen off
    matrix.fillScreen(dark);
    matrix.swapBuffers(false);
    delay(1000); // 1s
  }    
}


/***** TODO *****/
/*
 * current screen consumption: max = 3A (fullbright)
 */
