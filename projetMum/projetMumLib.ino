/*
 * File : projetMumLib
 * Description : Project's library
 * Developed by : Perhan Scudeller
 * E-mail address : perhan.scudeller@gmail.com
 * Licence : Copyright
 * Last update : 13.01.2020
 */

/*************** TIME ********************/ 
// Ethernet shield and NTP setup
void chooseServerNTP(){   
   int i = 0;
   int DHCP = 0;
   #ifdef DEBUG
   Serial.print("Server at : ");
   Serial.println(Ethernet.localIP());
   #endif
   DHCP = Ethernet.begin(mac);  
   //Try to get dhcp settings 30 times before giving up
   while( DHCP == 0 && i < 30){
     delay(1000);
     DHCP = Ethernet.begin(mac);
     i++;
   }
   if(!DHCP){
    #ifdef DEBUG
    Serial.println("DHCP FAILED");
    #endif
    #ifndef DEBUG
    matrix.fillScreen(0); //does not work - no ref to 'matrix' ?
    matrix.setCursor(1,1);
    matrix.setTextSize(2);
    matrix.setTextColor(matrix.Color333(5, 0, 0));
    matrix.println(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[46]))));
    #endif
    for(;;); //Infinite loop because DHCP Failed // TODO : DISPLAY "NOT CONNECTED"
   }
   #ifdef DEBUG
   Serial.println("DHCP Success");
   #endif

  //Find best server
   DNSClient dns;
   dns.begin(Ethernet.dnsServerIP());
   dns.getHostByName("pool.ntp.org",timeServer);
   #ifdef DEBUG
   Serial.print("NTP IP from the pool: ");
   Serial.println(timeServer);
   #endif
  
   //Try to get the date and time
   int trys=0;
   while(!getTimeAndDate() && trys<10) {
     trys++;
   }
}

// Do not alter this function, it is used by the system
int getTimeAndDate() {
   int flag=0;
   Udp.begin(localPort);
   sendNTPpacket(timeServer);
   delay(1000);
   if (Udp.parsePacket()){
     Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
     unsigned long highWord, lowWord, epoch;
     highWord = word(packetBuffer[40], packetBuffer[41]);
     lowWord = word(packetBuffer[42], packetBuffer[43]); 
     epoch = highWord << 16 | lowWord;
     epoch = epoch - 2208988800 + timeZoneOffset;
     flag=1;
     setTime(epoch);
     ntpLastUpdate = now();
   }
   return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;                 
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}

void updateServerTime(){
  if(now()-ntpLastUpdate > ntpSyncTime) {
      int trys=0;
      while(!getTimeAndDate() && trys<10){
        trys++;
      }
      if(trys<10){
        #ifdef DEBUG
        Serial.println("ntp server update success");
        #endif
      }
	  #ifdef DEBUG
      else{
        Serial.println("ntp server update failed");
      }
	  #endif
   }
}

void adjustJetLag(){
   int month_yet = month(now());
   #ifdef DEBUG
      Serial.println("Month : " + String(month_yet));
   #endif
   if(month_yet == 4 || month_yet == 5 || month_yet == 6 || month_yet == 7 || month_yet == 8 || month_yet == 9 || month_yet == 10){
    adjustTime(+3600);
    summerJetLag = true; //"summerTime"
   }
}


/***************** BUS SERVER **************/
void busServer(EthernetClient &busClient){
  boolean currentLineIsBlank = true;
  while (busClient.connected()) {
      if (busClient.available()) {
        char c = busClient.read();
        if(HTTP_req.length() < 100){
          HTTP_req += c;
        }
        #ifdef DEBUG
        Serial.write(c);
        #endif
        if (c == '\n' && currentLineIsBlank) {
          //busClient.println("HTTP/1.1 200 OK");
          //busClient.println("Content-Type: text/html");
          //busClient.println("Connection: close");
          busClient.println();   
          #ifdef DEBUGCOLOR
            Serial.println(HTTP_req);
          #endif
          busAPI(busClient);
          break;
        }if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
     } 
  }
  delay(1);
  busClient.stop();
  HTTP_req = "";
  #ifdef DEBUG
  Serial.println("Bus Client disconnected");
  #endif
}

void busAPI(EthernetClient &busClient){
    if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[62])))) > 0){ //"busC3" - BUS C3
      #ifdef DEBUG
      Serial.println("C3 detected");
      #endif
      busHasChanged = true;
      bus_C3[0] = HTTP_req.substring(HTTP_req.indexOf("=")+1, HTTP_req.indexOf("&"));
      bus_C3[1] = HTTP_req.substring(HTTP_req.indexOf("&")+1, HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[56]))))); //" HTTP"
      #ifdef DEBUG
      Serial.println(bus_C3[0]);
      #endif
    }else if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[63])))) > 0){ //"bus9" - BUS 9
      #ifdef DEBUG
      Serial.print("9 detected");
      #endif
      busHasChanged = true;
      bus_9[0] = HTTP_req.substring(HTTP_req.indexOf("=")+1, HTTP_req.indexOf("&"));
      bus_9[1] = HTTP_req.substring(HTTP_req.indexOf("&")+1, HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[56]))))); //" HTTP"
      #ifdef DEBUG
      Serial.println(bus_9[0]);
      #endif
    } else if (HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[64])))) > 0) { //"displayBus"
      if(HTTP_req.indexOf("=true") > 0){
        displayBus = true;
        displayDynWeather = true;
        displayMessage = false;
      } else if(HTTP_req.indexOf("=false") > 0){
        displayBus = false;
        displayMessage = false;
      }else if(HTTP_req.indexOf("=toggle") > 0){
        displayBus = !displayBus;
        displayDynWeather = !displayDynWeather;        
        displayMessage = false;
      }
    }else if (HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[65])))) > 0){ //"busSpeed" - BUS SPEED
      if(HTTP_req.indexOf("busSpeed?") > 0){
        busClient.println("{\"busSpeed\" : "+String(varSpeedBus)+"}");
      }else if (HTTP_req.indexOf("busSpeed=") > 0){ 
        unsigned int tmpSpeed = HTTP_req.substring(HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[30])))) + 1, HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[56]))))).toInt(); //"=" //" HTTP"
        if(tmpSpeed > 1 && tmpSpeed <= 60){ varSpeedBus = tmpSpeed;}
        #ifdef DEBUG 
          Serial.println(varSpeedBus);
        #endif
      }else{}
    }else if(HTTP_req.indexOf("temp") > 0 || HTTP_req.indexOf("cond") > 0){ //WEATHER
      #ifdef DEBUG
        Serial.println("Weather detected");
      #endif
      temp = HTTP_req.substring(HTTP_req.indexOf("temp=")+5, HTTP_req.indexOf("&"));
      int weather_cond = HTTP_req.substring(HTTP_req.indexOf("cond=")+5, HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[56]))))).toInt(); //" HTTP"
      selectCondLogo(weather_cond);
    } else if (HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[66])))) > 0){ //"varSpeedWeather"
      if(HTTP_req.indexOf("=?") > 0){
        busClient.println("{ \"varSpeedWeather\" : "+String(varSpeedWeather)+" }");
      }else if(HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[30])))) > 0){ //"="
        varSpeedWeather = HTTP_req.substring(HTTP_req.indexOf("=")+1, HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[56]))))).toInt(); //" HTTP"
      }else{}
    } else if(HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[67])))) > 0){ //"dynWeather"
      if(HTTP_req.indexOf("=on") > 0){
        displayDynWeather = true;
      }else if(HTTP_req.indexOf("=off") > 0){
        displayDynWeather = false;
      }else if(HTTP_req.indexOf("=toggle") > 0){
        displayDynWeather = !displayDynWeather;
      }else if(HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[68])))) > 0){ //"dynWeatherState"
        busClient.print("{\"dynWeatherState\" : ");
        busClient.print(displayDynWeather ? "1" : "0");
        busClient.println(" }");
      }
    } else if(HTTP_req.indexOf("rain") > 0){ //RAIN
      String subStr = HTTP_req.substring(HTTP_req.indexOf("=")+1,HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[56]))))); //" HTTP"
      for(int i=0; i < 12; i++ ){
        rainForecast[i] = subStr.substring(0,subStr.indexOf("&")).toInt();
        subStr.remove(0,subStr.indexOf("&")+1);
        }
    } else if (HTTP_req.indexOf("screen") > 0) { //SCREEN
      #ifdef DEBUG
          Serial.println("Screen detected");
      #endif
      if (HTTP_req.indexOf("=on") > 0) {
        screenState = true; //TODO: piloter pwm écran selon valeur dimmer
      } else if (HTTP_req.indexOf("=off") > 0) {
        screenState = false; //TODO: piloter pwm écran
      } else if (HTTP_req.indexOf("%") > 0) {
        dimmer = HTTP_req.substring(14, 16).toInt(); //recuperer la valeur
        //TODO: piloter pwm de l'écran grâce à l'arduino + transistor
      } else if(HTTP_req.indexOf("screenState") > 0){
        busClient.print("{\"screenState\" : ");
        busClient.print(screenState ? "1" : "0");
        busClient.println(" }");
      }else{}
      #ifdef DEBUG
        Serial.println(screenState);
      #endif
   } else if (HTTP_req.indexOf("msg") > 0 || HTTP_req.indexOf("Msg") > 0) { // MESSAGE
    #ifdef DEBUG
        Serial.println("Message detected");
    #endif
    if(HTTP_req.indexOf("msg=") > 0){
      displayBus = false;
      displayDynWeather = false;
      displayMessage = true;
      msg = HTTP_req.substring(HTTP_req.indexOf("msg=")+4,HTTP_req.indexOf(strcpy_P(buffer,(char*)pgm_read_word(&(rec_str[56]))))); //" HTTP"
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[17]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[16])))); //("+"," ")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[18]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[16])))); //("%20"," ")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[19]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[12])))); //("%21","!")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[20]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[21])))); //("%23","#")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[49]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[50])))); //("%26","&")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[22]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[23])))); //("%27","'")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[44]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[45])))); //("%28","(")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[24]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[25])))); //("%29",")")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[47]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[48])))); //("%40","@")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[51]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[52])))); //("%2A","*")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[55]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[17])))); //("%2B","+")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[26]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[27])))); //("%2C",",")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[53]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[54])))); //("%2F","/")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[28]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[11])))); //("%3A",":")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[29]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[30])))); //("%3D","=")
      msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[31]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[32])))); //("%3F","?")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[33]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[34])))); //("%C3%A7","ç")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[35]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[36])))); //("%C3%A8","è")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[37]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[38])))); //("%C3%A9","é")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[39]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[40])))); //("%C3%AA","ê")
	    msg.replace(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[41]))), strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[42])))); //("%E2%82%AC","€")
      textMin = msg.length() * -12;
      /*
      if(HTTP_req.indexOf("&color=") > 0){
        msg_color = hex2rgb(HTTP_req.substring(HTTP_req.indexOf("&color=")+7, HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[56])))))); //" HTTP"
      }
      */
    }else if(HTTP_req.indexOf("msg_color") > 0){
      msg_color = hex2rgb(HTTP_req.substring(HTTP_req.indexOf("msg_color=")+10, HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[56])))))); //" HTTP"
    }else if(HTTP_req.indexOf("msg_speed") > 0){
      varSpeedMsg = 100 - HTTP_req.substring(HTTP_req.indexOf("msg_speed=")+10, HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[56]))))).toInt(); //" HTTP"
    }else if(HTTP_req.indexOf("varSpeedMsg") > 0){
      unsigned int tmp = 100 - varSpeedMsg;
      busClient.println("{\"varSpeedMsg\" : "+String(tmp)+" }");
    }else if(HTTP_req.indexOf("get_msg") > 0){
		busClient.print("{\"msg\" : "+msg+"}");
	}else if(HTTP_req.indexOf("msgColor") > 0){
      //to get the text color 
      //TODO ? 
    }else if(HTTP_req.indexOf("displayMsg") > 0){
      if(HTTP_req.indexOf("=on") > 0){
        displayBus = false;
        displayDynWeather = false;
        displayMessage = true;
      }else if(HTTP_req.indexOf("=off") > 0){
        displayMessage = false;
      }
    }
  } else if (HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[61])))) > 0) { // "nightclub" - Easter Egg
    #ifdef DEBUG
      Serial.println("NightClub detected");
    #endif
    nightClub();
  }else if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[58])))) > 0){ // "Time" - TIME
    if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[14])))) > 0){
      if(!summerJetLag){
        adjustTime(+3600);
        summerJetLag = true; // summerTime
      }
    }else if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[15])))) > 0){
      if(summerJetLag){
        adjustTime(-3600);
        summerJetLag = false; // winterTime
      }
    }else if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[59])))) > 0){ // "whatTime"
      busClient.print("{\"jet_lag\" : \"");
      busClient.print(summerJetLag ? strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[14]))) : strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[15])))); //summerTime   //winterTimer
      busClient.println("\" }");
    }else if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[60])))) > 0){ // "updateTime"
      updateServerTime();
    }
  }else if(HTTP_req.indexOf(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[57])))) > 0){ // "/admin" - WEB PAGE
    #ifdef DEBUG
      Serial.println("Sending HTML Page");
    #endif
    // send web page
    int i = 0;
    for(i; i < 37; i++){
      busClient.print(strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))));
    }
    busClient.print(screenState ? strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))) : strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i+1])))); //btn ecran
    i+=2;
    for(i; i < 44; i++){
      busClient.print(strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))));
    }
    busClient.print(displayBus ? strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))) : strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i+1])))); //btn bus
    i+=2;
    for(i; i < 52; i++){
      busClient.print(strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))));
    }
    busClient.print(summerJetLag ? strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))) : strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i+1])))); //btn jetlag
    i+=2;
    for(i; i < 73; i++){
      busClient.print(strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))));
    }
    busClient.print(String(varSpeedBus));
    for(i; i < 179; i++){ 
      busClient.print(strcpy_P(buffer, (char*)pgm_read_word(&(server_response[i]))));
    }    
  }else{
    busClient.println(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[43]))));
  }
}





void selectCondLogo(int& weather_cond){
  if(weather_cond == 1087
        || weather_cond == 1273
        || weather_cond == 1276
        || weather_cond == 1279
        || weather_cond == 1282){ //Thunderstorm Orage
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[1])));
  }else if(weather_cond == 1063
        || weather_cond == 1069
        || weather_cond == 1072
        || weather_cond == 1150
        || weather_cond == 1153
        || weather_cond == 1168
        || weather_cond == 1171
        || weather_cond == 1180
        || weather_cond == 1183
        || weather_cond == 1186
        || weather_cond == 1189
        || weather_cond == 1204
        || weather_cond == 1240
        || weather_cond == 1249
        || weather_cond == 1261){ //Drizzle petite pluie
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[2])));
  }else if(weather_cond == 1192
        || weather_cond == 1195
        || weather_cond == 1198
        || weather_cond == 1201
        || weather_cond == 1207
        || weather_cond == 1243
        || weather_cond == 1246
        || weather_cond == 1252
        || weather_cond == 1264){ //Rain pluie
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[3])));
  }else if(weather_cond == 1066 
        || weather_cond == 1114 
        || weather_cond == 1117
        || weather_cond == 1210
        || weather_cond == 1213
        || weather_cond == 1216
        || weather_cond == 1219
        || weather_cond == 1222
        || weather_cond == 1225
        || weather_cond == 1237
        || weather_cond == 1255
        || weather_cond == 1258){ //Snow neige
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[4])));
  }else if(weather_cond == 1030 
        || weather_cond == 1135
        || weather_cond == 1147){ //Mist brouillard
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[5])));
  }else if(weather_cond == 1000){ //Clear soleil
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[6])));
  }else if(weather_cond == 1003){ //Few Clouds
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[7])));
  }else if(weather_cond == 1006){ //Scattered Clouds 
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[8])));
  }else if(weather_cond == 1009){ //Broken Clouds 
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[9])));
  }else{
    weather_cond_txt = strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[32]))); //"?"
  }
}

uint16_t hex2rgb(String str){
  /*
  uint8_t r = strtoul(str.substring(1,3).c_str(), NULL, 16);
  uint8_t g = strtoul(str.substring(3,5).c_str(), NULL, 16);
  uint8_t b = strtoul(str.substring(5,7).c_str(), NULL, 16);
  #ifdef DEBUG
    Serial.println(r);
    Serial.println(g);
    Serial.println(b);
  #endif
  */
  return matrix.Color888(strtoul(str.substring(1,3).c_str(), NULL, 16),strtoul(str.substring(3,5).c_str(), NULL, 16),strtoul(str.substring(5,7).c_str(), NULL, 16));
}


/********************** DISPLAY ********************/
void onBoot(){
  int hue = 0;
  while(hue < 1536){
    matrix.setCursor(5,9);
    matrix.setTextColor(matrix.ColorHSV(hue, 255, 255, true));
    matrix.print(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[0])))); // Ardui'Mum
    matrix.swapBuffers(true);
    hue += 5;
  }
  delay(100);
  matrix.setCursor(5,9);
  matrix.setTextColor(matrix.ColorHSV(768, 255, 255, true));
  matrix.print(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[0])))); // Ardui'Mum
  matrix.swapBuffers(true);
}

// Clock display of the time and date (Basic)
void displayTime(){
  if(lastMin == minute()){
    return; 
  }else{
    lastMin = minute();
  matrix.setCursor(1, 2);
  matrix.fillRect(0, 0, 32, 16, 0); 
  matrix.setTextColor(matrix.Color333(4,4,4)); //(5,5,5)
  matrix.print(hour());
  matrix.print(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[11])))); // : 
  if(minute()<10){
    matrix.print(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[13])))); // 0
    }
  matrix.print(minute());
  }
  matrix.swapBuffers(true);
}

void displayTest(){
  //matrix.setCursor(0, 0);
  //matrix.drawRGBBitmap(0, 0, c3, 16, 16); //matrix.Color333(0, 5, 2) = vert
  /*
  matrix.setTextColor(matrix.Color333(4,7,0));
  matrix.drawChar(13, 9, '5', matrix.Color333(4,7,0), matrix.Color333(0,0,0), 1);
  matrix.drawPixel(19, 11, matrix.Color333(7,7,7)); // :
  matrix.drawPixel(19, 13, matrix.Color333(7,7,7)); // :
  matrix.drawChar(21, 9, '3', matrix.Color333(4,7,0), matrix.Color333(0,0,0), 1);
  matrix.drawChar(27, 9, '0', matrix.Color333(4,7,0), matrix.Color333(0,0,0), 1);
 */
 }


/* WEATHER */
void displayTemp(){
  int temp_int = temp.toInt();
  uint16_t color;
  if(temp_int <= 0){
    color = matrix.Color333(1,1,4); 
  }else if(temp_int <= 5){
    color = matrix.Color333(2,2,4);
  }else if(temp_int <= 10){
    color = matrix.Color333(2,2,2);
  }else if(temp_int <= 15){
    color = matrix.Color333(4,2,1);
  }else if(temp_int <= 20){
    color = matrix.Color333(5,2,1);
  }else if(temp_int <= 25){
    color = matrix.Color333(6,2,0);
  }else{ // > 25
    color = matrix.Color333(6,1,0);
  }
  matrix.fillRect(31, 0, 33, 17, 0);
  matrix.setCursor(32, 5);
  matrix.setTextColor(color);
  if(temp_int <= -10){
    matrix.setCursor(matrix.getCursorX()-2, matrix.getCursorY());
    matrix.print(temp_int);
  }else{
    if(temp_int < 10){matrix.setCursor(matrix.getCursorX()+3, matrix.getCursorY());}
    matrix.print(temp);
  }
  //degré
  int cursor_x = matrix.getCursorX();
  int cursor_y = matrix.getCursorY();
  matrix.drawCircle(cursor_x+1, cursor_y+1, 1, color);
  matrix.setCursor(cursor_x+4, cursor_y);
  matrix.print(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[10])))); //C
}

void displayRainForecast(){
  for(int i=0;i<12;i++){
    int strength = rainForecast[i];
    if(strength>1){
      matrix.drawPixel(38, 0, clear_yellow); //start
    }
    for(int j=1;j<strength;j++){
      matrix.drawPixel(64-(12*2)+(i*2), j-1, tinyWhite);
    }
  }
}

/* BUS */
void displayBusC3() {
  displayLogoC3();
  displayScheduleC3();
  matrix.swapBuffers(true);
}

void displayBus9() {
  displayLogo9();
  displaySchedule9();
  matrix.swapBuffers(true);
}

void displayScheduleC3(){
  matrix.fillRect(32, 16, 32, 16, 0); 
  //first bus
  matrix.setCursor(34,17); 
  int minutes = bus_C3[0].substring(0,2).toInt();
  if(bus_C3[0].substring(0,5).indexOf("-") >= 0){
    displaySmiley(48, 21); // display sth funny
  }else if(minutes < 2){
    matrix.setTextColor(matrix.Color444(10,1,0)); // rouge
    matrix.print(bus_C3[0]);
  }else if(minutes < 4){
    matrix.setTextColor(matrix.Color333(5,1,0)); // orange  
    matrix.print(bus_C3[0]);
  }else if(minutes < 6){
    matrix.setTextColor(matrix.Color333(4,4,0)); // jaune
    matrix.print(bus_C3[0]);
  }else{
    matrix.setTextColor(matrix.Color333(4,4,3)); //presque blanc chaud //(2,3,2) OR (3,3,3)
    matrix.print(bus_C3[0]);
  }
  // second bus
  matrix.setCursor(34,25);
  matrix.setTextColor(matrix.Color333(1,1,1)); //presque discret 
  matrix.print(bus_C3[1]);
}

void  displaySchedule9(){
  matrix.fillRect(32, 16, 32, 16, 0);
  //first bus
  matrix.setCursor(34,17); 
  int minutes = bus_9[0].substring(0,2).toInt();
  if(bus_9[0].substring(0,5).indexOf("-") >= 0){
    displaySmiley(48, 21); // display sth funny //(48,20)
  }else if(minutes < 2){
    matrix.setTextColor(matrix.Color444(10,1,0)); // rouge
    matrix.print(bus_9[0]);
  }else if(minutes < 4){
    matrix.setTextColor(matrix.Color333(5,1,0)); // orange  
    matrix.print(bus_9[0]);
  }else if(minutes < 6){
    matrix.setTextColor(matrix.Color333(4,4,0)); // jaune
    matrix.print(bus_9[0]);
  }else{
    matrix.setTextColor(matrix.Color333(4,4,3)); //presque blanc chaud //(2,3,2)
    matrix.print(bus_9[0]);
  }
  // second bus
  matrix.setCursor(34,25);
  matrix.setTextColor(matrix.Color333(1,1,1)); //presque discret
  matrix.print(bus_9[1]);
}

/* MESSAGE */
void displayMsg(){
  matrix.fillRect(0,16,64,16,dark);
  matrix.setTextSize(2);
  matrix.setTextColor(msg_color);
  matrix.setCursor(textX, 17);
  matrix.print(msg);
  matrix.swapBuffers(true);
  if((--textX) < textMin) textX = matrix.width();
  delay(varSpeedMsg);
  matrix.setTextSize(1);
}

void displayWeather() {
  if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[1]))))) {
    displayThunderstorm();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[2]))))) {
    displayDrizzle();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[3]))))) {
    displayRain();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[4]))))) {
    displaySnow();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[5]))))) {
    displayMist();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[6]))))) {
    displayClear();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[7]))))) {
    displaySunnyClouds();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[8]))))) {
    displayClouds();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[9]))))) {
    displayBrokenClouds();
  }else{
    displayWeatherError();
  }
  matrix.swapBuffers(true);
}

void displayAddsOn(){
  if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[1]))))) {
    displayThunderstormAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[2]))))) {
    displayDrizzleAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[3]))))) {
    displayRainAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[4]))))) {
    displaySnowAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[5]))))) {
    displayMistAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[6]))))) {
    displayClearAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[7]))))) {
    displaySunnyCloudsAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[8]))))) {
    displayCloudsAO();
  } else if (weather_cond_txt.equals(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[9]))))) {
    displayBrokenCloudsAO();
  }else {return;}
  matrix.swapBuffers(true);
}

void displayWeatherError(){
  matrix.fillRect(32,0,32,16,dark);
  matrix.setCursor(32+15,4);
  matrix.setTextColor(matrix.Color333(1,0,0));
  matrix.print(strcpy_P(buffer, (char*)pgm_read_word(&(rec_str[12]))));
  matrix.swapBuffers(true);
}

void displayThunderstorm() { //200
  matrix.fillRect(32, 0, 32, 16, dark);
  int nuage_x = 32;
  int nuage_y = 0;
  nuage(nuage_x,nuage_y);
  eclair(nuage_x+8, nuage_y+10);
  eclair(nuage_x+8+8, nuage_y+10);
  eclair(nuage_x+8+8+8, nuage_y+10);
}

void displayThunderstormAO(){
  matrix.fillRect(32, 0, 32, 16, dark);
  nuage(32,0);
  thCursor += 1;
  switch(thCursor){
    case 1:
      eclair(40, 10);
      break;
    case 2:
      eclair(40, 10);
      eclair(40+8, 10);
      break;
    case 3:
      eclair(40, 10);
      eclair(40+8, 10);
      eclair(40+8+8, 10);
      break;
    case 4:
      thCursor = 0;
      break;
    default:
      break;
  } 
}

void eclair(uint8_t x, uint8_t y){
    for(int i=0; i<3;i++){
      matrix.drawPixel(x, y, matrix.Color333(4, 4, 0));
      matrix.drawPixel(x, y+2, yellow);
    x-=1;
    y+=1;
  }
  matrix.drawPixel(x+2, y-1, matrix.Color333(2, 2, 0));
}

void displayDrizzle() { //300
  randomSeed(analogRead(0));
  matrix.fillRect(32, 0, 32, 16, dark);
  nuage(32,0);
  for(int i=0; i<20;i++){
    petite_goutte(random(33,64),random(11,15));
  }
}

void displayDrizzleAO(){
  displayDrizzle();
}

void petite_goutte(uint8_t x, uint8_t y){
  matrix.drawPixel(x, y, tinyWhite);
  matrix.drawPixel(x+1, y-1, tinyWhite);
}

void displayRain() { //500
  matrix.fillRect(32, 0, 32, 16, dark);
  //cloud
  int nuage_x = 32;
  int nuage_y = -1;
  nuage(nuage_x, nuage_y);
  //gouttes penchées
  int n=1;
  for(int i=nuage_x+8;i<=27;i+=4){
    if((n%2) == 0){ //pair
      goutte(i,nuage_y+10);
    }else{ //impair
      goutte(i,nuage_y+9);
    }
    n += 1;
  }
}

void displayRainAO(){
  matrix.fillRect(32, 0, 32, 16, dark);
  nuage(32, -1);
  thCursor += 1;
  int n=1;
  for(int i=39;i<=59;i+=4){
    if((n%2) == 0){ //pair
      goutte(i-thCursor,9+thCursor);
    }else{ //impair
      goutte(i-thCursor,8+thCursor);
    }
    n += 1;
  }
  if(thCursor == 4) thCursor = 0;
}

void nuage(uint8_t x, int8_t y){
  matrix.drawLine(x+4, y+9, x+27, y+9, tinyWhite);
  matrix.drawPixel(x+4, y+8, tinyWhite);
  matrix.drawPixel(x+4, y+7, tinyWhite);
  matrix.drawPixel(x+5, y+7, tinyWhite);
  matrix.drawPixel(x+6, y+6, tinyWhite);
  matrix.drawPixel(x+7, y+5, tinyWhite);
  matrix.drawPixel(x+8, y+4, tinyWhite);
  matrix.drawPixel(x+9, y+4, tinyWhite);
  matrix.drawPixel(x+10, y+4, tinyWhite);
  matrix.drawPixel(x+11, y+3, tinyWhite);
  matrix.drawPixel(x+12, y+2, tinyWhite);
  matrix.drawPixel(x+13, y+1, tinyWhite);
  matrix.drawPixel(x+14, y+1, tinyWhite);
  matrix.drawPixel(x+15, y+1, tinyWhite);
  matrix.drawPixel(x+16, y+1, tinyWhite);
  matrix.drawPixel(x+17, y+1, tinyWhite);
  matrix.drawPixel(x+18, y+2, tinyWhite);
  matrix.drawPixel(x+19, y+3, tinyWhite);
  matrix.drawPixel(x+20, y+4, tinyWhite);
  matrix.drawPixel(x+21, y+5, tinyWhite);
  matrix.drawPixel(x+22, y+5, tinyWhite);
  matrix.drawPixel(x+23, y+5, tinyWhite);
  matrix.drawPixel(x+24, y+5, tinyWhite);
  matrix.drawPixel(x+25, y+6, tinyWhite);
  matrix.drawPixel(x+26, y+7, tinyWhite);
  matrix.drawPixel(x+27, y+8, tinyWhite);
  matrix.drawPixel(x+24, y+6, tinyWhite);
  matrix.drawPixel(x+21, y+4, tinyWhite);
  matrix.drawPixel(x+17, y+2, tinyWhite);
  matrix.drawPixel(x+13, y+2, tinyWhite);
  matrix.drawPixel(x+10, y+3, tinyWhite);
  matrix.drawPixel(x+8,  y+5, tinyWhite);
}

void goutte(uint8_t x, uint8_t y){
  for(int i=0; i<3;i++){
    matrix.drawPixel(x, y, tinyWhite);
    x-=1;
    y+=1;
  }
}

void displaySnow() { //511
  matrix.fillRect(32, 0, 32, 16, dark);
  matrix.setCursor(32, snowCursorY); //(32, snowCursorY)
  flocon(34, snowCursorY + 0);
  flocon(46, snowCursorY + 5);
  flocon(57, snowCursorY + 1);
  if (snowCursorY == 14) {
    snowCursorY = 0;
  } else {
    snowCursorY += 1;
  }
}

void displaySnowAO(){
  displaySnow();
}

void flocon(uint8_t x, uint8_t y) {
  //affiche un flocon
  //vertical line
  if(y+0 <= 15){
    matrix.drawPixel(x + 3, y + 0, tinyWhite);
  }
  if(y+1 <= 15){
    matrix.drawPixel(x + 3, y + 1, tinyWhite);
  }
  if(y+2 <= 15){
    matrix.drawPixel(x + 3, y + 2, tinyWhite);
  }
  if(y+3 <= 15){
    matrix.drawPixel(x + 3, y + 3, tinyWhite);
  }
  if(y+4 <= 15){
    matrix.drawPixel(x + 3, y + 4, tinyWhite);
  }
  if(y+5 <= 15){
    matrix.drawPixel(x + 3, y + 5, tinyWhite);
  }
  if(y+6 <= 15){
    matrix.drawPixel(x + 3, y + 6, tinyWhite);
  }
  //matrix.drawLine(x + 3, y + 0, x + 3, y + 6, tinyWhite);
  //horizontal line
  if(y+3 <= 15){
    matrix.drawPixel(x + 0, y + 3, tinyWhite);
    matrix.drawPixel(x + 1, y + 3, tinyWhite);
    matrix.drawPixel(x + 2, y + 3, tinyWhite);
    matrix.drawPixel(x + 3, y + 3, tinyWhite);
    matrix.drawPixel(x + 4, y + 3, tinyWhite);
    matrix.drawPixel(x + 5, y + 3, tinyWhite);
    matrix.drawPixel(x + 6, y + 3, tinyWhite);
  }
  //matrix.drawLine(x + 0, y + 3, x + 6, y + 3, tinyWhite);
  if(y+1 <= 15){
    matrix.drawPixel(x + 1, y + 1, tinyWhite);
    matrix.drawPixel(x + 5, y + 1, tinyWhite);
  }
  if(y+2 <= 15){
    matrix.drawPixel(x + 2, y + 2, tinyWhite);
    matrix.drawPixel(x + 4, y + 2, tinyWhite);
  }
  if(y+4 <= 15){
    matrix.drawPixel(x + 2, y + 4, tinyWhite);
    matrix.drawPixel(x + 4, y + 4, tinyWhite);
  }
  if(y+5 <= 15){
    matrix.drawPixel(x + 1, y + 5, tinyWhite);
    matrix.drawPixel(x + 5, y + 5, tinyWhite);
  }
}

void displayMist() { //701
  matrix.fillRect(32, 0, 32, 16, dark);
  nuage(32,0);
  matrix.drawLine(8+32, 11, 21+32, 11, tinyWhite);
  matrix.drawLine(9+32, 13, 23+32, 13, tinyWhite);
  matrix.drawLine(8+32, 15, 21+32, 15, tinyWhite);
}

void displayMistAO(){
  matrix.fillRect(32, 0, 32, 16, dark);
  thCursor += 1;
  nuage(32,0);
  switch(thCursor){
    case 1:
      matrix.drawLine(6+32, 11, 19+32, 11, tinyWhite);
      matrix.drawLine(8+32, 13, 21+32, 13, tinyWhite);
      matrix.drawLine(6+32, 15, 19+32, 15, tinyWhite);
      break;
    case 2:
      matrix.drawLine(7+32, 11, 20+32, 11, tinyWhite);
      matrix.drawLine(9+32, 13, 22+32, 13, tinyWhite);
      matrix.drawLine(7+32, 15, 20+32, 15, tinyWhite);
      break;
    case 3:
      matrix.drawLine(8+32, 11, 21+32, 11, tinyWhite);
      matrix.drawLine(9+32, 13, 23+32, 13, tinyWhite);
      matrix.drawLine(8+32, 15, 21+32, 15, tinyWhite);
      break;
    case 4:
      matrix.drawLine(9+32, 11, 22+32, 11, tinyWhite);
      matrix.drawLine(10+32, 13, 24+32, 13, tinyWhite);
      matrix.drawLine(9+32, 15, 22+32, 15, tinyWhite);
      break;
    case 5:
      matrix.drawLine(10+32, 11, 23+32, 11, tinyWhite);
      matrix.drawLine(11+32, 13, 25+32, 13, tinyWhite);
      matrix.drawLine(10+32, 15, 23+32, 15, tinyWhite);
      break;
    case 6:
      matrix.drawLine(11+32, 11, 24+32, 11, tinyWhite);
      matrix.drawLine(12+32, 13, 26+32, 13, tinyWhite);
      matrix.drawLine(11+32, 15, 24+32, 15, tinyWhite);
      thCursor = 0;
      break;
    default:
      break;
  }
}

void displayClear() { //800
  matrix.fillRect(32, 0, 32, 17, dark);
  matrix.drawCircle(16+32, 7, 4, clear_yellow); 
  matrix.drawPixel(16+32, 0, clear_yellow); //haut
  matrix.drawPixel(16+32, 1, clear_yellow);
  matrix.drawPixel(16+32, 15, clear_yellow); //bas
  matrix.drawPixel(16+32, 14, clear_yellow);
  matrix.drawPixel(16+32, 13, clear_yellow);
  matrix.drawPixel(10+32, 7, clear_yellow); //gauche
  matrix.drawPixel(9+32, 7, clear_yellow);
  matrix.drawPixel(8+32, 7, clear_yellow);
  matrix.drawPixel(7+32, 7, clear_yellow);
  matrix.drawPixel(22+32, 7, clear_yellow); //droite
  matrix.drawPixel(23+32, 7, clear_yellow);
  matrix.drawPixel(24+32, 7, clear_yellow);
  matrix.drawPixel(25+32, 7, clear_yellow);
  matrix.drawPixel(11+32, 2, clear_yellow); // haut gauche
  matrix.drawPixel(10+32, 1, clear_yellow);
  matrix.drawPixel(9+32, 0, clear_yellow);
  matrix.drawPixel(21+32, 2, clear_yellow); //haut droite
  matrix.drawPixel(22+32, 1, clear_yellow);
  matrix.drawPixel(23+32, 0, clear_yellow);
  matrix.drawPixel(11+32, 12, clear_yellow); //bas gauche
  matrix.drawPixel(10+32, 13, clear_yellow);
  matrix.drawPixel(9+32, 14, clear_yellow);
  matrix.drawPixel(21+32, 12, clear_yellow); //bas droite
  matrix.drawPixel(22+32, 13, clear_yellow);
  matrix.drawPixel(23+32, 14, clear_yellow);
  //smiley
  matrix.drawPixel(15+32, 6, yellow);
  matrix.drawPixel(17+32, 6, yellow);
  matrix.drawPixel(14+32, 8, yellow);
  matrix.drawPixel(15+32, 9, yellow);
  matrix.drawPixel(16+32, 9, yellow);
  matrix.drawPixel(17+32, 9, yellow);
  matrix.drawPixel(18+32, 8, yellow);
}

void displayClearAO(){
  matrix.fillRect(32, 0, 32, 17, dark); 
  matrix.drawCircle(16+32, 7, 4, clear_yellow); 
  matrix.drawPixel(15+32, 6, yellow);
  matrix.drawPixel(17+32, 6, yellow);
  matrix.drawPixel(14+32, 8, yellow);
  matrix.drawPixel(15+32, 9, yellow);
  matrix.drawPixel(16+32, 9, yellow);
  matrix.drawPixel(17+32, 9, yellow);
  matrix.drawPixel(18+32, 8, yellow);
  thCursor += 1;
  switch (thCursor) {
    case 1:
      matrix.drawPixel(16+32, 0, clear_yellow); //haut
      matrix.drawPixel(16+32, 1, clear_yellow);
      matrix.drawPixel(16+32, 2, clear_yellow);
      matrix.drawPixel(16+32, 15, clear_yellow); //bas
      matrix.drawPixel(16+32, 14, clear_yellow);
      matrix.drawPixel(16+32, 13, clear_yellow);
      matrix.drawPixel(16+32, 12, clear_yellow);
      matrix.drawPixel(10+32, 7, clear_yellow); //gauche
      matrix.drawPixel(9+32, 7, clear_yellow);
      matrix.drawPixel(8+32, 7, clear_yellow);
      matrix.drawPixel(11+32, 7, clear_yellow);
      matrix.drawPixel(22+32, 7, clear_yellow); //droite
      matrix.drawPixel(23+32, 7, clear_yellow);
      matrix.drawPixel(24+32, 7, clear_yellow);
      matrix.drawPixel(21+32, 7, clear_yellow);
      matrix.drawPixel(11+32, 2, clear_yellow); // haut gauche
      matrix.drawPixel(10+32, 1, clear_yellow);
      matrix.drawPixel(12+32, 3, clear_yellow);
      matrix.drawPixel(21+32, 2, clear_yellow); //haut droite
      matrix.drawPixel(22+32, 1, clear_yellow);
      matrix.drawPixel(20+32, 3, clear_yellow);
      matrix.drawPixel(11+32, 12, clear_yellow); //bas gauche
      matrix.drawPixel(10+32, 13, clear_yellow);
      matrix.drawPixel(12+32, 11, clear_yellow);
      matrix.drawPixel(21+32, 12, clear_yellow); //bas droite
      matrix.drawPixel(22+32, 13, clear_yellow);
      matrix.drawPixel(20+32, 11, clear_yellow);
      break;
    case 2:  
      matrix.drawPixel(16+32, 0, clear_yellow); //haut
      matrix.drawPixel(16+32, 1, clear_yellow);
      matrix.drawPixel(16+32, 15, clear_yellow); //bas
      matrix.drawPixel(16+32, 14, clear_yellow);
      matrix.drawPixel(16+32, 13, clear_yellow);
      matrix.drawPixel(10+32, 7, clear_yellow); //gauche
      matrix.drawPixel(9+32, 7, clear_yellow);
      matrix.drawPixel(8+32, 7, clear_yellow);
      matrix.drawPixel(7+32, 7, clear_yellow);
      matrix.drawPixel(22+32, 7, clear_yellow); //droite
      matrix.drawPixel(23+32, 7, clear_yellow);
      matrix.drawPixel(24+32, 7, clear_yellow);
      matrix.drawPixel(25+32, 7, clear_yellow);
      matrix.drawPixel(11+32, 2, clear_yellow); // haut gauche
      matrix.drawPixel(10+32, 1, clear_yellow);
      matrix.drawPixel(9+32, 0, clear_yellow);
      matrix.drawPixel(21+32, 2, clear_yellow); //haut droite
      matrix.drawPixel(22+32, 1, clear_yellow);
      matrix.drawPixel(23+32, 0, clear_yellow);
      matrix.drawPixel(11+32, 12, clear_yellow); //bas gauche
      matrix.drawPixel(10+32, 13, clear_yellow);
      matrix.drawPixel(9+32, 14, clear_yellow);
      matrix.drawPixel(21+32, 12, clear_yellow); //bas droite
      matrix.drawPixel(22+32, 13, clear_yellow);
      matrix.drawPixel(23+32, 14, clear_yellow);
      break;
    case 3: 
      matrix.drawPixel(16+32, 0, clear_yellow); //haut
      matrix.drawPixel(16+32, 2, clear_yellow);
      matrix.drawPixel(16+32, 15, clear_yellow); //bas
      matrix.drawPixel(16+32, 14, clear_yellow);
      matrix.drawPixel(16+32, 16, clear_yellow);
      matrix.drawPixel(16+32, 12, clear_yellow);
      matrix.drawPixel(6+32, 7, clear_yellow); //gauche
      matrix.drawPixel(9+32, 7, clear_yellow);
      matrix.drawPixel(8+32, 7, clear_yellow);
      matrix.drawPixel(7+32, 7, clear_yellow);
      matrix.drawPixel(11+32, 7, clear_yellow);
      matrix.drawPixel(26+32, 7, clear_yellow); //droite
      matrix.drawPixel(23+32, 7, clear_yellow);
      matrix.drawPixel(24+32, 7, clear_yellow);
      matrix.drawPixel(25+32, 7, clear_yellow);
      matrix.drawPixel(21+32, 7, clear_yellow);
      matrix.drawPixel(8+32, -1, clear_yellow); // haut gauche
      matrix.drawPixel(10+32, 1, clear_yellow);
      matrix.drawPixel(9+32, 0, clear_yellow);
      matrix.drawPixel(12+32, 3, clear_yellow);
      matrix.drawPixel(24+32, -1, clear_yellow); //haut droite
      matrix.drawPixel(22+32, 1, clear_yellow);
      matrix.drawPixel(23+32, 0, clear_yellow);
      matrix.drawPixel(20+32, 3, clear_yellow);
      matrix.drawPixel(8+32, 15, clear_yellow); //bas gauche
      matrix.drawPixel(10+32, 13, clear_yellow);
      matrix.drawPixel(9+32, 14, clear_yellow);
      matrix.drawPixel(12+32, 11, clear_yellow);
      matrix.drawPixel(24+32, 15, clear_yellow); //bas droite
      matrix.drawPixel(22+32, 13, clear_yellow);
      matrix.drawPixel(23+32, 14, clear_yellow);
      matrix.drawPixel(20+32, 11, clear_yellow);
      break;
    case 4:
      //matrix.drawPixel(16, 0, clear_yellow); //haut
      matrix.drawPixel(16+32, 1, clear_yellow);
      matrix.drawPixel(16+32, 2, clear_yellow);
      matrix.drawPixel(16+32, 15, clear_yellow); //bas
      //matrix.drawPixel(16, 17, clear_yellow);
      matrix.drawPixel(16+32, 16, clear_yellow);
      matrix.drawPixel(16+32, 13, clear_yellow);
      matrix.drawPixel(16+32, 12, clear_yellow);
      matrix.drawPixel(6+32, 7, clear_yellow); //gauche
      matrix.drawPixel(10+32, 7, clear_yellow);
      matrix.drawPixel(5+32, 7, clear_yellow);
      matrix.drawPixel(8+32, 7, clear_yellow);
      matrix.drawPixel(7+32, 7, clear_yellow);
      matrix.drawPixel(11+32, 7, clear_yellow);
      matrix.drawPixel(26+32, 7, clear_yellow); //droite
      matrix.drawPixel(27+32, 7, clear_yellow);
      matrix.drawPixel(24+32, 7, clear_yellow);
      matrix.drawPixel(25+32, 7, clear_yellow);
      matrix.drawPixel(22+32, 7, clear_yellow);
      matrix.drawPixel(21+32, 7, clear_yellow);
      //matrix.drawPixel(8, 0, clear_yellow); // haut gauche
      matrix.drawPixel(11+32, 2, clear_yellow);
      matrix.drawPixel(9+32, 0, clear_yellow);
      matrix.drawPixel(12+32, 3, clear_yellow);
      //matrix.drawPixel(24, 0, clear_yellow); //haut droite
      matrix.drawPixel(21+32, 2, clear_yellow);
      matrix.drawPixel(23+32, 0, clear_yellow);
      matrix.drawPixel(20+32, 3, clear_yellow);
      matrix.drawPixel(8+32, 15, clear_yellow); //bas gauche
      matrix.drawPixel(7+32, 16, clear_yellow);
      matrix.drawPixel(9+32, 14, clear_yellow);
      matrix.drawPixel(11+32, 12, clear_yellow);
      matrix.drawPixel(12+32, 11, clear_yellow);
      matrix.drawPixel(24+32, 15, clear_yellow); //bas droite
      matrix.drawPixel(25+32, 16, clear_yellow);
      matrix.drawPixel(23+32, 14, clear_yellow);
      matrix.drawPixel(21+32, 12, clear_yellow);
      matrix.drawPixel(20+32, 11, clear_yellow);
      break;
    case 5:
      thCursor = 0;
      break;
    default:
      break;
  }
}

void displaySunnyClouds() { //801
  matrix.fillRect(32, 0, 32, 17, dark);
  matrix.drawCircle(20+34, 8, 3, yellow);
  nuage(31,4);
  matrix.drawPixel(17+34, 9, dark); //erase
  matrix.drawPixel(18+34, 10, dark);
  matrix.drawPixel(19+34, 11, dark);
  matrix.drawPixel(20+34, 11, dark);
  matrix.drawPixel(21+34, 11, dark);
  matrix.drawPixel(20+34, 1, yellow); //strahle haut
  matrix.drawPixel(20+34, 2, yellow);
  matrix.drawPixel(20+34, 3, yellow);
  matrix.drawPixel(16+34, 4, yellow); //haut gauche
  matrix.drawPixel(15+34, 3, yellow);
  matrix.drawPixel(14+34, 2, yellow);
  matrix.drawPixel(24+34, 4, yellow); //haut droit
  matrix.drawPixel(25+34, 3, yellow);
  matrix.drawPixel(26+34, 2, yellow);
  matrix.drawPixel(25+34, 8, yellow); //droit
  matrix.drawPixel(26+34, 8, yellow);
  matrix.drawPixel(27+34, 8, yellow);
  matrix.drawPixel(24+34, 11, yellow); //bas drt ????
  matrix.drawPixel(25+34, 12, yellow);
  matrix.drawPixel(26+34, 13, yellow);
}

void displaySunnyCloudsAO(){
  matrix.fillRect(32, 0, 32, 17, dark);
  thCursor += 1;
  switch(thCursor){
    case 1:
      nuage(30,4);
      matrix.drawCircle(20+34, 8, 3, yellow);
      matrix.drawPixel(17+34, 9, tinyWhite); 
      matrix.drawPixel(18+34, 10, dark);
      matrix.drawPixel(19+34, 11, dark); //erase
      matrix.drawPixel(20+34, 11, dark);
      matrix.drawPixel(21+34, 11, dark);
      matrix.drawPixel(20+34, 1, yellow); //strahle haut
      matrix.drawPixel(20+34, 2, yellow);
      matrix.drawPixel(20+34, 3, yellow);
      matrix.drawPixel(16+34, 4, yellow); //haut gauche
      matrix.drawPixel(15+34, 3, yellow);
      matrix.drawPixel(14+34, 2, yellow);
      matrix.drawPixel(24+34, 4, yellow); //haut droit
      matrix.drawPixel(25+34, 3, yellow);
      matrix.drawPixel(26+34, 2, yellow);
      matrix.drawPixel(25+34, 8, yellow); //droit
      matrix.drawPixel(26+34, 8, yellow);
      matrix.drawPixel(27+34, 8, yellow);
      matrix.drawPixel(24+34, 11, yellow); //bas drt ????
      matrix.drawPixel(25+34, 12, yellow);
      matrix.drawPixel(26+34, 13, yellow);
      break;
    case 2:
      nuage(29,4);
      matrix.drawCircle(20+34, 8, 3, yellow);
      matrix.drawPixel(17+34, 9, tinyWhite); 
      matrix.drawPixel(21+34, 11, tinyWhite);
      matrix.drawPixel(18+34, 10, dark); //erase
      matrix.drawPixel(19+34, 11, dark);
      matrix.drawPixel(20+34, 11, dark);
      matrix.drawPixel(20+34, 1, yellow); //strahle haut
      matrix.drawPixel(20+34, 2, yellow);
      matrix.drawPixel(20+34, 3, yellow);
      matrix.drawPixel(16+34, 4, yellow); //haut gauche
      matrix.drawPixel(15+34, 3, yellow);
      matrix.drawPixel(14+34, 2, yellow);
      matrix.drawPixel(24+34, 4, yellow); //haut droit
      matrix.drawPixel(25+34, 3, yellow);
      matrix.drawPixel(26+34, 2, yellow);
      matrix.drawPixel(25+34, 8, yellow); //droit
      matrix.drawPixel(26+34, 8, yellow);
      matrix.drawPixel(27+34, 8, yellow);
      matrix.drawPixel(24+34, 11, yellow); //bas drt ????
      matrix.drawPixel(25+34, 12, yellow);
      matrix.drawPixel(26+34, 13, yellow);
      break;
    case 3:
      nuage(28,4);
      matrix.drawCircle(20+34, 8, 3, yellow);
      matrix.drawPixel(17+34, 9, tinyWhite);  
      matrix.drawPixel(20+34, 11, tinyWhite);
      matrix.drawPixel(21+34, 11, tinyWhite);
      matrix.drawPixel(18+34, 10, dark); //erase
      matrix.drawPixel(19+34, 11, dark);
      matrix.drawPixel(20+34, 1, yellow); //strahle haut
      matrix.drawPixel(20+34, 2, yellow);
      matrix.drawPixel(20+34, 3, yellow);
      matrix.drawPixel(16+34, 4, yellow); //haut gauche
      matrix.drawPixel(15+34, 3, yellow);
      matrix.drawPixel(14+34, 2, yellow);
      matrix.drawPixel(24+34, 4, yellow); //haut droit
      matrix.drawPixel(25+34, 3, yellow);
      matrix.drawPixel(26+34, 2, yellow);
      matrix.drawPixel(25+34, 8, yellow); //droit
      matrix.drawPixel(26+34, 8, yellow);
      matrix.drawPixel(27+34, 8, yellow);
      matrix.drawPixel(24+34, 11, yellow); //bas drt ????
      matrix.drawPixel(25+34, 12, yellow);
      matrix.drawPixel(26+34, 13, yellow);
      break;
    case 4:
      nuage(28,4);
      matrix.drawCircle(20+34, 8, 3, yellow);
      matrix.drawPixel(17+34, 9, tinyWhite);  
      matrix.drawPixel(20+34, 11, tinyWhite);
      matrix.drawPixel(21+34, 11, tinyWhite);
      matrix.drawPixel(18+34, 10, dark); //erase
      matrix.drawPixel(19+34, 11, dark);
      matrix.drawPixel(20+34, 1, clear_yellow); //strahle haut
      matrix.drawPixel(20+34, 2, clear_yellow);
      matrix.drawPixel(20+34, 3, clear_yellow);
      matrix.drawPixel(16+34, 4, clear_yellow); //haut gauche
      matrix.drawPixel(15+34, 3, clear_yellow);
      matrix.drawPixel(14+34, 2, clear_yellow);
      matrix.drawPixel(24+34, 4, clear_yellow); //haut droit
      matrix.drawPixel(25+34, 3, clear_yellow);
      matrix.drawPixel(26+34, 2, clear_yellow);
      matrix.drawPixel(25+34, 8, clear_yellow); //droit
      matrix.drawPixel(26+34, 8, clear_yellow);
      matrix.drawPixel(27+34, 8, clear_yellow);
      matrix.drawPixel(24+34, 11, clear_yellow); //bas drt ????
      matrix.drawPixel(25+34, 12, clear_yellow);
      matrix.drawPixel(26+34, 13, clear_yellow);
      break;
    case 5:
      thCursor = 0;
      nuage(30,4);
      break;
    default:
      break;
  }
}

void displayClouds() { //802
  matrix.fillRect(32, 0, 32, 16, dark);
  nuage(31,4);
  nuage(12+32,0);
  matrix.drawPixel(16+32, 7, dark);
  matrix.drawPixel(17+32, 7, dark);
  matrix.drawPixel(16+32, 8, dark);
  matrix.drawPixel(16+32, 9, dark);
  matrix.drawPixel(17+32, 9, dark);
  matrix.drawPixel(18+32, 9, dark);
  matrix.drawPixel(19+32, 9, dark);
}

void displayCloudsAO(){
  matrix.fillRect(32, 0, 32, 16, dark);
  thCursor += 1;
  switch(thCursor){
    case 1:
      nuage(31,4);
      nuage(12+32,0);
      matrix.drawPixel(16+32, 7, dark);
      matrix.drawPixel(17+32, 7, dark);
      matrix.drawPixel(16+32, 8, dark);
      matrix.drawPixel(16+32, 9, dark);
      matrix.drawPixel(17+32, 9, dark);
      matrix.drawPixel(18+32, 9, dark);
      matrix.drawPixel(19+32, 9, dark);
      break;
    case 2:
      nuage(30,4);
      nuage(13+32,0);
      matrix.drawPixel(17+32, 8, dark);
      matrix.drawPixel(17+32, 9, dark);
      matrix.drawPixel(18+32, 9, dark);
      break;
    case 3:
      nuage(29,5);
      nuage(14+32,0);
      break;
    case 4:
      nuage(28,6);
      nuage(15+32,0);
      break;
    case 5:
      thCursor = 0;
      break;
    default:
      break;
  }
}

void displayBrokenClouds() { //803
  displayClouds();
}

void displayBrokenCloudsAO(){
  displayCloudsAO();  
}

void displayLogoC3() {
  matrix.fillRect(0, 16, 32, 16, dark);
  uint16_t orange = matrix.Color888(230, 142, 0);
  matrix.fillRect(0, 16, 24, 16, orange);
  //C
  matrix.drawChar(1, 17, 'C', dark, orange, 2);
  //
  matrix.drawPixel(2, 18, dark);
  matrix.drawPixel(3, 19, dark);
  matrix.drawPixel(2, 29, dark);
  matrix.drawPixel(3, 28, dark); 
  matrix.drawPixel(9, 18, dark); 
  matrix.drawPixel(8, 19, dark);
  matrix.drawPixel(9, 29, dark);
  matrix.drawPixel(8, 28, dark);
  //5
  matrix.drawChar(12, 17, '5', dark, orange, 2);
  //
  matrix.drawPixel(21, 17, orange);
  matrix.drawPixel(12, 17, orange);
  matrix.drawPixel(20, 22, dark);
  matrix.drawPixel(20, 29, dark);
  matrix.drawPixel(13, 29, dark);
}

void displayLogo9() {
  matrix.fillRect(0, 16, 32, 16, dark);
  uint16_t orange = matrix.Color888(160, 109, 31);
  matrix.fillRect(0, 16, 24, 16, orange);
  //1
  matrix.drawChar(1, 17, '1', dark, orange, 2);
  //
  matrix.drawPixel(4, 18, dark);
  matrix.drawPixel(2, 20, dark);
  //4
  matrix.drawChar(12, 17, '4', dark, orange, 2);
  //
  matrix.drawPixel(17, 18, dark);
  matrix.drawPixel(15, 20, dark);
  matrix.drawPixel(13, 22, dark);
  matrix.drawPixel(15, 22, orange);
}

void displaySmiley(uint8_t x, uint8_t y) {
  matrix.drawCircle(x, y, 4, yellow);
  matrix.drawPixel(x-1, y-1, yellow);
  matrix.drawPixel(x+1, y-1, yellow);
  matrix.drawPixel(x-2, y+1, yellow);
  matrix.drawPixel(x-1, y+2, yellow);
  matrix.drawPixel(x, y+2, yellow);
  matrix.drawPixel(x+1, y+2, yellow);
  matrix.drawPixel(x+2, y+1, yellow);
}

void nightClub(){
  while(1){
    matrix.fillScreen(matrix.Color333(7,7,7));
    matrix.swapBuffers(false);
    delay(100);
    matrix.fillScreen(0);
    matrix.swapBuffers(false);
    delay(100);
  }
}

void regulBus() {
  if(toggleBus){ //if true, C3 should be displayed 
    if(!busOnDisplay.equals("C3")){ 
      busOnDisplay = "C3";
      displayBusC3(); 
    }else if(busHasChanged){
      busHasChanged = false;
      displayScheduleC3(); //update bus9 (schedule only) on display
    }else return;
  }else{
    if(!busOnDisplay.equals("9")){ 
      busOnDisplay = "9";
      displayBus9();
    }else if(busHasChanged){
      busHasChanged = false;
      displaySchedule9(); //update bus9 (schedule only) on display
    }else return;
  }
}

void regulTempWeath() {
  if(toggleTempWeath){
    if(!TWOnDisplay.equals("Temp")){
      TWOnDisplay = "Temp";
      displayTemp();
      displayRainForecast();
      matrix.swapBuffers(true);
    }else return;
  }else{
    if(!TWOnDisplay.equals("Weath")){
      TWOnDisplay = "Weath";
      displayWeather();
      delay(100);
    }
    if(displayDynWeather){
      displayAddsOn();
      delay(varSpeedWeather);
    }
  }
}

void busHandler(){ //every 'varSpeedBus' seconds
  time_t now_int = now();
  if(now_int - previousToggleBus >= varSpeedBus){
    toggleBus =! toggleBus;
    previousToggleBus = now_int;
  }
}

void tempWeathHandler(){ //every 7s
  time_t now_int = now();
  if((now_int - previousToggleTempWeath) >= 7){
    toggleTempWeath =! toggleTempWeath;
    previousToggleTempWeath = now_int;
  }
}

void serverCallback() {
  EthernetClient busClient = server.available();
  if (busClient) {
    busServer(busClient);
  } delay(1);
}
