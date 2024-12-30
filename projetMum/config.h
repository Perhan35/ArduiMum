/*
 * File : config.h
 * Description : Configuraiton file for the library
 * Developed by : Perhan Scudeller
 * E-mail address : perhan.scudeller@gmail.com
 * Licence : Copyright
 * Last update : 13.01.2020
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TimeLib.h>
#include <Dns.h>

/************ RGB Display 32x64 **************/
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library
#define CLK 11
#define OE  31
#define A   A9 
#define B   A10
#define C   A11
#define D   A12
#define LAT A13
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, true, 64);

/****** PINS FOR MEGA panel=32x16 ******/
/*
CLK -> 11
OE -> 9
A -> A0
B -> A1
C -> A2
LAT -> A3
R1 -> 24
G1 -> 25
B1 -> 26
R2 -> 27
G2 -> 28
B2 -> 29
*/

/****** PINS FOR MEGA panel=64x32 ******/
/*
CLK -> 11
OE -> 9 -> 31
A -> A0 -> A9
B -> A1 -> A10
C -> A2 -> A11
D -> A3 -> A12
LAT -> A4 -> A13
R1 -> 24
G1 -> 25
B1 -> 26
R2 -> 27
G2 -> 28
B2 -> 29
*/

/****** PINS FOR MEGA panel=32x16 ******/
/*
CLK -> 11
OE -> 9 -> 31
A -> A0 -> A9
B -> A1 -> A10
C -> A2 -> A11
LAT -> A4 -> A13
R1 -> 24
G1 -> 25
B1 -> 26
R2 -> 27
G2 -> 28
B2 -> 29
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, true);

/************ RGB Display 32x64 **************/
/*
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library
#define OE   9
#define LAT 10
#define CLK 11
#define A   A0
#define B   A1
#define C   A2
#define D   A3
RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);
*/

/********** Ethernet Card Settings *********/
// Set this to your Ethernet Card Mac Address
byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0xDF, 0xAB};  // adresse MAC de la carte
byte ip[] = {192, 168, 1, 111};                     //adresse IP
EthernetClient client;
EthernetServer server(80);

/********** TIME *************/
const long timeZoneOffset = +3600L; //offset (in seconds) to your local time
unsigned int ntpSyncTime = 3600;  //Syncs to NTP server every 1 hour
unsigned int lastMin = 0;
/* ALTER THESE VARIABLES AT YOUR OWN RISK */
unsigned int localPort = 8888;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // Buffer to hold incoming and outgoing packets
EthernetUDP Udp;                    // A UDP instance to let us send and receive packets over UDP
unsigned long ntpLastUpdate = 0;    // Keeps track of how long ago we updated the NTP server
time_t prevDisplay = 0;             // Check last time clock displayed (Not in Production)

/* ******** NTP Server Settings ******** */
IPAddress timeServer(162, 210, 110, 4); //address of NTP server , autre possibilite : 145.238.203.10 //last : 94.23.204.123



/********** Variables ***********/
boolean displayBus = true;
boolean displayMessage = false;
boolean displayDynWeather = true;
boolean screenState = true;
boolean toggleBus = true;
boolean busHasChanged = false;
boolean toggleTempWeath = true;
boolean summerJetLag = false;
String busOnDisplay = "C3";
String TWOnDisplay = "Temp";
String temp = "0";
String weather_cond_txt = "";
int textX = matrix.width(); 
int textMin;
unsigned int dimmer = 75;
unsigned int varSpeedBus = 20; //20s
unsigned int varSpeedMsg = 0; //0ms between two display of message
unsigned int varSpeedWeather = 700; //700ms between two change of Dynamic Weather
unsigned int snowCursorY = 0;
unsigned int thCursor = 0;
unsigned int rainForecast[12]={1,1,1,1,1,1,1,1,1,1,1,1};
unsigned long previousToggleBus = 0;
unsigned long previousToggleTempWeath = 0;
String msg = "";
String HTTP_req = "";
String bus_C3[2];
String bus_9[2];
const uint16_t tinyWhite = matrix.Color333(1, 1, 1);
const uint16_t dark = matrix.Color333(0, 0, 0);
const uint16_t yellow = matrix.Color333(1, 1, 0);
const uint16_t clear_yellow = matrix.Color333(4, 4, 0);
uint16_t msg_color = tinyWhite;

/*** Server Web Page ***/
const PROGMEM char s_r_0[] = "<!DOCTYPE html><html>";
const PROGMEM char s_r_1[] = "<head><title>Serveur ";
const PROGMEM char s_r_2[] = "Arduino </title><style>";
const PROGMEM char s_r_3[] = ".screen { background";
const PROGMEM char s_r_4[] = "-color: #4CAF50; bor";
const PROGMEM char s_r_5[] = "der: none; color: wh";
const PROGMEM char s_r_6[] = "ite; padding: 15px 2";
const PROGMEM char s_r_7[] = "5px; text-align: cen";
const PROGMEM char s_r_8[] = "ter; font-size: 16px";
const PROGMEM char s_r_9[] = "; cursor: pointer;}";
const PROGMEM char s_r_10[] = ".screen:hover {backg";
const PROGMEM char s_r_11[] = "round: green;} .h1{te";
const PROGMEM char s_r_12[] = "xt-align: center;}";
const PROGMEM char s_r_13[] = ".bus {background-color:";
const PROGMEM char s_r_14[] = " #66ccff; border: none;";
const PROGMEM char s_r_15[] = " color: white; padding:";
const PROGMEM char s_r_16[] = " 15px 25px; text-align:";
const PROGMEM char s_r_17[] = " center; font-size: ";
const PROGMEM char s_r_18[] = "16px; cursor: pointer;}";
const PROGMEM char s_r_19[] = ".bus:hover { backgro";
const PROGMEM char s_r_20[] = "und: #0099ff; }";
const PROGMEM char s_r_21[] = ".chgt_heure {backgro";
const PROGMEM char s_r_22[] = "und-color: #FA5858; ";
const PROGMEM char s_r_23[] = "border: none; color:";
const PROGMEM char s_r_24[] = " white; padding: 15px ";
const PROGMEM char s_r_25[] = "25px; text-align: cen";
const PROGMEM char s_r_26[] = "ter; font-size: 16px;";
const PROGMEM char s_r_27[] = " cursor: pointer;}";
const PROGMEM char s_r_28[] = ".chgt_heure:hover {b";
const PROGMEM char s_r_29[] = "ackground: #FE2E2E;}";
const PROGMEM char s_r_30[] = "</style></head><body>";
const PROGMEM char s_r_31[] = "<h1>Bienvenue sur le ";
const PROGMEM char s_r_32[] = "serveur Web Arduino ";
const PROGMEM char s_r_33[] = "!</h1><br/><br/>";
const PROGMEM char s_r_34[] = "<table><tr><th><button ";
const PROGMEM char s_r_35[] = "class=\"screen\" id=\"s";
const PROGMEM char s_r_36[] = "creen\" type=\"button\">";

const PROGMEM char s_r_37[] = "Eteindre l'&eacutecran";
const PROGMEM char s_r_38[] = "Allumer l'&eacutecran";

const PROGMEM char s_r_39[] = "</button></th><th style";
const PROGMEM char s_r_40[] = "=\"width:15px\"></th>";
const PROGMEM char s_r_41[] = "<th><button class=\"";
const PROGMEM char s_r_42[] = "bus\" id=\"bus\" type";
const PROGMEM char s_r_43[] = "=\"button\">";

const PROGMEM char s_r_44[] = "Eteindre l'affichage des bus";
const PROGMEM char s_r_45[] = "Afficher les bus";

const PROGMEM char s_r_46[] = "</button></th>";
const PROGMEM char s_r_47[] = "<th style=\"width:15";
const PROGMEM char s_r_48[] = "px\"></th><th><button";
const PROGMEM char s_r_49[] = " class=\"chgt_heure\"";
const PROGMEM char s_r_50[] = " id=\"chgt_heure\" t";
const PROGMEM char s_r_51[] = "ype=\"button\">";

const PROGMEM char s_r_52[] = "Passer en heure d'hiver";
const PROGMEM char s_r_53[] = "Passer en heure d'&eacutet&eacute";

const PROGMEM char s_r_54[] = "</button></th></tr>";
const PROGMEM char s_r_55[] = "</table><br/><h4>Swi";
const PROGMEM char s_r_56[] = "tcher entre bus C3 e";
const PROGMEM char s_r_57[] = "t bus n&ordm;9 toutes";
const PROGMEM char s_r_58[] = " les <input type=\"n";
const PROGMEM char s_r_59[] = "umber\" id=\"busSpeed";
const PROGMEM char s_r_60[] = "Input\" required name";
const PROGMEM char s_r_61[] = "=\"busSpeedInput\" p";
const PROGMEM char s_r_62[] = "laceholder=\"20\" mi";
const PROGMEM char s_r_63[] = "n=\"2\" max=\"60\"> <";
const PROGMEM char s_r_64[] = "/input> secondes.<bu";
const PROGMEM char s_r_65[] = "tton class=\"busSpee";
const PROGMEM char s_r_66[] = "d\" id=\"busSpeed\" ";
const PROGMEM char s_r_67[] = "type=\"button\" styl";
const PROGMEM char s_r_68[] = "e=\"margin-left: 10px";
const PROGMEM char s_r_69[] = ";\">Envoyer</button>";
const PROGMEM char s_r_70[] = "<h4><h5>Actuellement";
const PROGMEM char s_r_71[] = " le basculement s'ef";
const PROGMEM char s_r_72[] = "fectue toutes les ";

const PROGMEM char s_r_73[] = " secondes.</h5><br/>";
const PROGMEM char s_r_74[] = "<br/><div id=\"reque";
const PROGMEM char s_r_75[] = "st_sent\" style=\"fo";
const PROGMEM char s_r_76[] = "nt-style: italic; di";
const PROGMEM char s_r_77[] = "splay:none\"> Requ&e";
const PROGMEM char s_r_78[] = "circ;te envoy&eacute";
const PROGMEM char s_r_79[] = "e.</div></br><div id";
const PROGMEM char s_r_80[] = "=\"request_ok\" styl";
const PROGMEM char s_r_81[] = "e=\"font-style: ital";
const PROGMEM char s_r_82[] = "ic; display:none\"> ";
const PROGMEM char s_r_83[] = "Requ&ecirc;te bien r";
const PROGMEM char s_r_84[] = "e&ccedilue par le serv";
const PROGMEM char s_r_85[] = "eur.</div></body></html>";
const PROGMEM char s_r_86[] = "<script>function htt";
const PROGMEM char s_r_87[] = "pGetAsync(Url){var x";
const PROGMEM char s_r_88[] = "mlHttp = new XMLHttp";
const PROGMEM char s_r_89[] = "Request(); xmlHttp.o";
const PROGMEM char s_r_90[] = "nreadystatechange = ";
const PROGMEM char s_r_91[] = "function() {if (xmlH";
const PROGMEM char s_r_92[] = "ttp.readyState == 4 ";
const PROGMEM char s_r_93[] = "&& xmlHttp.status ==";
const PROGMEM char s_r_94[] = " 200){document.getEl";
const PROGMEM char s_r_95[] = "ementById(\"request_";
const PROGMEM char s_r_96[] = "ok\").style.display=";
const PROGMEM char s_r_97[] = "";
const PROGMEM char s_r_98[] = "\"inline\"; setTimeout";
const PROGMEM char s_r_99[] = "(function(){document";
const PROGMEM char s_r_100[] = ".getElementById(\"re";
const PROGMEM char s_r_101[] = "quest_ok\").style.di";
const PROGMEM char s_r_102[] = "splay=\"none\";}, 30";
const PROGMEM char s_r_103[] = "00);}} \r\n xmlHttp.ope";
const PROGMEM char s_r_104[] = "n(\"GET\", Url, true); ";
const PROGMEM char s_r_105[] = "xmlHttp.send(null); d";
const PROGMEM char s_r_106[] = "ocument.getElementBy";
const PROGMEM char s_r_107[] = "Id(\"request_sent\")";
const PROGMEM char s_r_108[] = ".style.display=\"inl";
const PROGMEM char s_r_109[] = "ine\"; setTimeout(fu";
const PROGMEM char s_r_110[] = "nction(){document.ge";
const PROGMEM char s_r_111[] = "tElementById(\"reque";
const PROGMEM char s_r_112[] = "st_sent\").style.displ";
const PROGMEM char s_r_113[] = "ay=\"none\";}, 3000);}";
const PROGMEM char s_r_114[] = " function changeBtnSc";
const PROGMEM char s_r_115[] = "reenName(){ if(docum";
const PROGMEM char s_r_116[] = "ent.getElementById(\"";
const PROGMEM char s_r_117[] = "screen\").innerHTML.";
const PROGMEM char s_r_118[] = "includes(\"Eteindre\"))";
const PROGMEM char s_r_119[] = "{ httpGetAsync(\"/sc";
const PROGMEM char s_r_120[] = "reen=off\"); documen";
const PROGMEM char s_r_121[] = "t.getElementById(\"";
const PROGMEM char s_r_122[] = "screen\").innerHTML ";
const PROGMEM char s_r_123[] = "=  \"Allumer l'&eacu";
const PROGMEM char s_r_124[] = "tecran\"; }else{ htt";
const PROGMEM char s_r_125[] = "pGetAsync(\"/screen=";
const PROGMEM char s_r_126[] = "on\"); document.getE";
const PROGMEM char s_r_127[] = "lementById(\"screen\")";
const PROGMEM char s_r_128[] = ".innerHTML =  \"Eteindr";
const PROGMEM char s_r_129[] = "e l'&eacutecran\";}}";
const PROGMEM char s_r_130[] = "function changeBtnBu";
const PROGMEM char s_r_131[] = "sName(){ if(document";
const PROGMEM char s_r_132[] = ".getElementById(\"bu";
const PROGMEM char s_r_133[] = "s\").innerHTML ==  \"";
const PROGMEM char s_r_134[] = "Eteindre l'affichage";
const PROGMEM char s_r_135[] = " des bus\"){ httpGet";
const PROGMEM char s_r_136[] = "Async(\"/?displayBus";
const PROGMEM char s_r_137[] = "=false\"); document.";
const PROGMEM char s_r_138[] = "getElementById(\"bus";
const PROGMEM char s_r_139[] = "\").innerHTML =  \"A";
const PROGMEM char s_r_140[] = "fficher les bus\"; }";
const PROGMEM char s_r_141[] = "else{ httpGetAsync(\"";
const PROGMEM char s_r_142[] = "/?displayBus=true\")";
const PROGMEM char s_r_143[] = "; document.getElemen";
const PROGMEM char s_r_144[] = "tById(\"bus\").inner";
const PROGMEM char s_r_145[] = "HTML =  \"Eteindre l";
const PROGMEM char s_r_146[] = "'affichage des bus\";}}";
const PROGMEM char s_r_147[] = "function busSpeed(){";
const PROGMEM char s_r_148[] = " http_req = \"/?busS";
const PROGMEM char s_r_149[] = "peed=\"; http_req += ";
const PROGMEM char s_r_150[] = "document.getElementB";
const PROGMEM char s_r_151[] = "yId(\"busSpeedInput\")";
const PROGMEM char s_r_152[] = ".value; httpGetAsync";
const PROGMEM char s_r_153[] = "(http_req);} function ";
const PROGMEM char s_r_154[] = "setJetLag() { if(doc";
const PROGMEM char s_r_155[] = "ument.getElementById";
const PROGMEM char s_r_156[] = "(\"chgt_heure\").inner";
const PROGMEM char s_r_157[] = "HTML.includes(\"hive";
const PROGMEM char s_r_158[] = "r\")){httpGetAsync(\"";
const PROGMEM char s_r_159[] = "/winterTime\");}else";
const PROGMEM char s_r_160[] = "{httpGetAsync(\"/sum";
const PROGMEM char s_r_161[] = "merTime\");} documen";
const PROGMEM char s_r_162[] = "t.getElementById(\"c";
const PROGMEM char s_r_163[] = "hgt_heure\").style.d";
const PROGMEM char s_r_164[] = "isplay=\"none\";} do";
const PROGMEM char s_r_165[] = "cument.getElementByI";
const PROGMEM char s_r_166[] = "d(\"screen\").onclick";
const PROGMEM char s_r_167[] = " = changeBtnScreenName;";
const PROGMEM char s_r_168[] = "document.getElementB";
const PROGMEM char s_r_169[] = "yId(\"bus\").onclick";
const PROGMEM char s_r_170[] = " = changeBtnBusName;";
const PROGMEM char s_r_171[] = " document.getElementB";
const PROGMEM char s_r_172[] = "yId(\"busSpeed\").on";
const PROGMEM char s_r_173[] = "click = busSpeed; doc";
const PROGMEM char s_r_174[] = "ument.getElementById";
const PROGMEM char s_r_175[] = "(\"chgt_heure\").onc";
const PROGMEM char s_r_176[] = "lick = setJetLag;";
const PROGMEM char s_r_177[] = "</script>";
const PROGMEM char s_r_178[] = "";

const char* const server_response[] PROGMEM = {s_r_0,s_r_1,s_r_2,s_r_3,s_r_4,s_r_5,s_r_6,s_r_7,s_r_8,s_r_9,s_r_10,s_r_11,s_r_12,s_r_13,s_r_14,s_r_15,
                                               s_r_16,s_r_17,s_r_18,s_r_19,s_r_20,s_r_21,s_r_22,s_r_23,s_r_24,s_r_25,s_r_26,s_r_27,s_r_28,s_r_29,s_r_30,s_r_31,s_r_32,s_r_33,
                                               s_r_34,s_r_35,s_r_36,s_r_37,s_r_38,s_r_39,s_r_40,s_r_41,s_r_42,s_r_43,s_r_44,s_r_45,s_r_46,s_r_47,s_r_48,s_r_49,s_r_50,s_r_51,
                                               s_r_52,s_r_53,s_r_54,s_r_55,s_r_56,s_r_57,s_r_58,s_r_59,s_r_60,s_r_61,s_r_62,s_r_63,s_r_64,s_r_65,s_r_66,s_r_67,s_r_68,s_r_69,
                                               s_r_70,s_r_71,s_r_72,s_r_73,s_r_74,s_r_75,s_r_76,s_r_77,s_r_78,s_r_79,s_r_80,s_r_81,s_r_82,s_r_83,s_r_84,s_r_85,s_r_86,s_r_87,
                                               s_r_88,s_r_89,s_r_90,s_r_91,s_r_92,s_r_93,s_r_94,s_r_95,s_r_96,s_r_97,s_r_98,s_r_99,s_r_100,s_r_101,s_r_102,s_r_103,s_r_104,
                                               s_r_105,s_r_106,s_r_107,s_r_108,s_r_109,s_r_110,s_r_111,s_r_112,s_r_113,s_r_114,s_r_115,s_r_116,s_r_117,s_r_118,s_r_119,s_r_120,
                                               s_r_121,s_r_122,s_r_123,s_r_124,s_r_125,s_r_126,s_r_127,s_r_128,s_r_129,s_r_130,s_r_131,s_r_132,s_r_133,s_r_134,s_r_135,s_r_136,
                                               s_r_137,s_r_138,s_r_139,s_r_140,s_r_141,s_r_142,s_r_143,s_r_144,s_r_145,s_r_146,s_r_147,s_r_148,s_r_149,s_r_150,s_r_151,s_r_152,
                                               s_r_153,s_r_154,s_r_155,s_r_156,s_r_157,s_r_158,s_r_159,s_r_160,s_r_161,s_r_162,s_r_163,s_r_164,s_r_165,s_r_166,s_r_167,s_r_168,
                                               s_r_169,s_r_170,s_r_171,s_r_172,s_r_173,s_r_174,s_r_175,s_r_176,s_r_177,s_r_178};


/**** Recorded Strings ****/
const PROGMEM char r_s_0[] = "Ardui'Mum";
const PROGMEM char r_s_1[] = "Thunderstorm";
const PROGMEM char r_s_2[] = "Drizzle";
const PROGMEM char r_s_3[] = "Rain";
const PROGMEM char r_s_4[] = "Snow";
const PROGMEM char r_s_5[] = "Mist";
const PROGMEM char r_s_6[] = "Clear";
const PROGMEM char r_s_7[] = "SunnyClouds";
const PROGMEM char r_s_8[] = "Clouds";
const PROGMEM char r_s_9[] = "BrokenClouds";
const PROGMEM char r_s_10[] = "C";
const PROGMEM char r_s_11[] = ":";
const PROGMEM char r_s_12[] = "!";
const PROGMEM char r_s_13[] = "0";
const PROGMEM char r_s_14[] = "summerTime";
const PROGMEM char r_s_15[] = "winterTime";
const PROGMEM char r_s_16[] = " ";
const PROGMEM char r_s_17[] = "+";
const PROGMEM char r_s_18[] = "%20";
const PROGMEM char r_s_19[] = "%21";
const PROGMEM char r_s_20[] = "%23";
const PROGMEM char r_s_21[] = "#";
const PROGMEM char r_s_22[] = "%27";
const PROGMEM char r_s_23[] = "'";
const PROGMEM char r_s_24[] = "%29";
const PROGMEM char r_s_25[] = ")";
const PROGMEM char r_s_26[] = "%2C";
const PROGMEM char r_s_27[] = ",";
const PROGMEM char r_s_28[] = "%3A";
const PROGMEM char r_s_29[] = "%3D";
const PROGMEM char r_s_30[] = "=";
const PROGMEM char r_s_31[] = "%3F";
const PROGMEM char r_s_32[] = "?";
const PROGMEM char r_s_33[] = "%C3%A7";
const PROGMEM char r_s_34[] = "ç";
const PROGMEM char r_s_35[] = "%C3%A8";
const PROGMEM char r_s_36[] = "è";
const PROGMEM char r_s_37[] = "%C3%A9";
const PROGMEM char r_s_38[] = "é";
const PROGMEM char r_s_39[] = "%C3%AA";
const PROGMEM char r_s_40[] = "ê";
const PROGMEM char r_s_41[] = "%E2%82%AC";
const PROGMEM char r_s_42[] = "€";
const PROGMEM char r_s_43[] = "Go to /admin";
const PROGMEM char r_s_44[] = "%28";
const PROGMEM char r_s_45[] = "(";
const PROGMEM char r_s_46[] = "Error DHCP";
const PROGMEM char r_s_47[] = "%40";
const PROGMEM char r_s_48[] = "@";
const PROGMEM char r_s_49[] = "%26";
const PROGMEM char r_s_50[] = "&";
const PROGMEM char r_s_51[] = "%2A";
const PROGMEM char r_s_52[] = "*";
const PROGMEM char r_s_53[] = "%2F";
const PROGMEM char r_s_54[] = "/";
const PROGMEM char r_s_55[] = "%2B";
const PROGMEM char r_s_56[] = " HTTP";
const PROGMEM char r_s_57[] = "/admin";
const PROGMEM char r_s_58[] = "Time";
const PROGMEM char r_s_59[] = "whatTime";
const PROGMEM char r_s_60[] = "updateTime";
const PROGMEM char r_s_61[] = "nightclub";
const PROGMEM char r_s_62[] = "busC3";
const PROGMEM char r_s_63[] = "bus9";
const PROGMEM char r_s_64[] = "displayBus";
const PROGMEM char r_s_65[] = "busSpeed";
const PROGMEM char r_s_66[] = "varSpeedWeather";
const PROGMEM char r_s_67[] = "dynWeather";
const PROGMEM char r_s_68[] = "dynWeatherState";
const PROGMEM char r_s_69[] = "";

const char* const rec_str[] PROGMEM = {r_s_0,r_s_1,r_s_2,r_s_3,r_s_4,r_s_5,r_s_6,r_s_7,r_s_8,r_s_9,r_s_10,r_s_11,r_s_12,r_s_13,r_s_14,r_s_15,
										r_s_16,r_s_17,r_s_18,r_s_19,r_s_20,r_s_21,r_s_22,r_s_23,r_s_24,r_s_25,r_s_26,r_s_27,r_s_28,r_s_29,
										r_s_30,r_s_31,r_s_32,r_s_33,r_s_34,r_s_35,r_s_36,r_s_37,r_s_38,r_s_39,r_s_40,r_s_41,r_s_42,r_s_43,
										r_s_44,r_s_45,r_s_46,r_s_47,r_s_48,r_s_49,r_s_50,r_s_51,r_s_52,r_s_53,r_s_54,r_s_55,r_s_56,r_s_57,
										r_s_58,r_s_59,r_s_60,r_s_61,r_s_62,r_s_63,r_s_64,r_s_65,r_s_66,r_s_67,r_s_68,r_s_69};


char buffer[25];
