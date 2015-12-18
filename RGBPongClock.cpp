/*  RGB Pong Clock - Andrew Holmes @pongclock
**  Inspired by, and shamelessly derived from 
**      Nick's LED Projects
**  https://123led.wordpress.com/about/
**  
**  Videos of the clock in action:
**  https://vine.co/v/hwML6OJrBPw
**  https://vine.co/v/hgKWh1KzEU0
**  https://vine.co/v/hgKz5V0jrFn
**  I run this on a Mega 2560, your milage on other chips may vary,
**  Can definately free up some memory if the bitmaps are shrunk down to size.
**  Uses an Adafruit 16x32 RGB matrix availble from here:
**  http://www.phenoptix.com/collections/leds/products/16x32-rgb-led-matrix-panel-by-adafruit
**  This microphone:
**  http://www.phenoptix.com/collections/adafruit/products/electret-microphone-amplifier-max4466-with-adjustable-gain-by-adafruit-1063
**  a DS1307 RTC chip (not sure where I got that from - was a spare)
**  and an Ethernet Shield
**  http://hobbycomponents.com/index.php/dvbd/dvbd-ardu/ardu-shields/2012-ethernet-w5100-network-shield-for-arduino-uno-mega-2560-1280-328.html
** 
*/


#include "Adafruit_mfGFX.h"   // Core graphics library
#include "RGBmatrixPanel.h" // Hardware-specific library
#include "fix_fft.h"
#include "blinky.h"
#include "font3x5.h"
#include "font5x5.h"

//#define DEBUGME

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);


// allow us to use itoa() in this scope
extern char* itoa(int a, char* buffer, unsigned char radix);


#define		RGBPCversion	"V1.03g"

#ifdef DEBUGME
	#define DEBUGp(message)		Serial.print(message)
	#define DEBUGpln(message)	Serial.println(message)
#else
	#define DEBUGp(message)
	#define DEBUGpln(message)
#endif


/** Define RGB matrix panel GPIO pins **/
#if defined (STM32F10X_MD)	//Core
	#define CLK D6
	#define OE  D7
	#define LAT A4
	#define A   A0
	#define B   A1
	#define C   A2
	#define D	A3
#endif

#if defined (STM32F2XX)	//Photon
	#define CLK D6
	#define OE  D7
	#define LAT A4
	#define A   A0
	#define B   A1
	#define C   A2
	#define D	A3
#endif
/****************************************/

#define SHOWCLOCK 10000  
#define MAX_CLOCK_MODE		7                 // Number of clock modes

/********** RGB565 Color definitions **********/
#define Black           0x0000
#define Navy            0x000F
#define DarkGreen       0x03E0
#define DarkCyan        0x03EF
#define Maroon          0x7800
#define Purple          0x780F
#define Olive           0x7BE0
#define LightGrey       0xC618
#define DarkGrey        0x7BEF
#define Blue            0x001F
#define Green           0x07E0
#define Cyan            0x07FF
#define Red             0xF800
#define Magenta         0xF81F
#define Yellow          0xFFE0
#define White           0xFFFF
#define Orange          0xFD20
#define GreenYellow     0xAFE5
#define Pink			0xF81F
/**********************************************/



/********** PACMAN definitions **********/
#define usePACMAN			// Uncomment to enable PACMAN animations

#define BAT1_X 2                         // Pong left bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define BAT2_X 28        

#define X_MAX 31                         // Matrix X max LED coordinate (for 2 displays placed next to each other)
#define Y_MAX 15

int powerPillEaten = 0;
              
/****************************************/


/***** Weather webhook definitions *****/
#define HOOK_RESP		"hook-response/pmk_w3"
#define HOOK_PUB		"pmk_w3"
#define DEFAULT_CITY	"\"mycity\":\"Ottawa,ON\""	// Change to desired default city,state
#define API_KEY			"\"apikey\":\"62810bbd5419f1ff67f8f61588833bf6\""
#define UNITS			"\"units\":\"metric\""		// Change to "imperial" for farenheit units
/***************************************/

/***** Create RGBmatrix Panel instance *****
 Last parameter = 'true' enables double-buffering, for flicker-free,
 buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
 until the first call to swapBuffers().  This is normal. */
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, true, 32);	// 16x32
//RGBmatrixPanel matrix(A, B, C, D,CLK, LAT, OE, true, 32);	// 32x32
//RGBmatrixPanel matrix(A, B, C, D,CLK, LAT, OE, true, 64); // 64x32
/*******************************************/

int stringPos;
boolean weatherGood=false;
int badWeatherCall;
char w_temp[8][7] = {""};
char w_id[8][4] = {""};

/*** Set default city for weather webhook ***/
char city[40] = DEFAULT_CITY;

boolean wasWeatherShownLast= true;
unsigned long lastWeatherTime =0;

int mode_changed = 0;			// Flag if mode changed.
bool mode_quick = false;		// Quick weather display
int clock_mode = 0;				// Default clock mode (1 = pong)
uint16_t showClock = 300;		// Default time to show a clock face
unsigned long modeSwitch;
unsigned long updateCTime;		// 24hr timer for resyncing cloud time



/************  FFT definitions **********/
#define useFFT			// Uncomment to enable FFT clock

#if defined useFFT

// Define MIC input pin
#define MIC A5			// A7 for Core, A5 for Photon

int8_t im[128];
int8_t fftdata[128];
int8_t spectrum[32];

byte
peak[32],		// Peak level of each column; used for falling dots
dotCount = 0,	// Frame counter for delaying dot-falling speed
colCount = 0;	// Frame counter for storing past column data

int8_t
col[32][10],	// Column levels for the prior 10 frames
minLvlAvg[32],	// For dynamic adjustment of low & high ends of graph,
maxLvlAvg[32];	// pseudo rolling averages for the prior few frames.
#endif
/***************************************/

/************  PLASMA definitions **********/

static const int8_t PROGMEM sinetab[256] = {
     0,   2,   5,   8,  11,  15,  18,  21,
    24,  27,  30,  33,  36,  39,  42,  45,
    48,  51,  54,  56,  59,  62,  65,  67,
    70,  72,  75,  77,  80,  82,  85,  87,
    89,  91,  93,  96,  98, 100, 101, 103,
   105, 107, 108, 110, 111, 113, 114, 116,
   117, 118, 119, 120, 121, 122, 123, 123,
   124, 125, 125, 126, 126, 126, 126, 126,
   127, 126, 126, 126, 126, 126, 125, 125,
   124, 123, 123, 122, 121, 120, 119, 118,
   117, 116, 114, 113, 111, 110, 108, 107,
   105, 103, 101, 100,  98,  96,  93,  91,
    89,  87,  85,  82,  80,  77,  75,  72,
    70,  67,  65,  62,  59,  56,  54,  51,
    48,  45,  42,  39,  36,  33,  30,  27,
    24,  21,  18,  15,  11,   8,   5,   2,
     0,  -3,  -6,  -9, -12, -16, -19, -22,
   -25, -28, -31, -34, -37, -40, -43, -46,
   -49, -52, -55, -57, -60, -63, -66, -68,
   -71, -73, -76, -78, -81, -83, -86, -88,
   -90, -92, -94, -97, -99,-101,-102,-104,
  -106,-108,-109,-111,-112,-114,-115,-117,
  -118,-119,-120,-121,-122,-123,-124,-124,
  -125,-126,-126,-127,-127,-127,-127,-127,
  -128,-127,-127,-127,-127,-127,-126,-126,
  -125,-124,-124,-123,-122,-121,-120,-119,
  -118,-117,-115,-114,-112,-111,-109,-108,
  -106,-104,-102,-101, -99, -97, -94, -92,
   -90, -88, -86, -83, -81, -78, -76, -73,
   -71, -68, -66, -63, -60, -57, -55, -52,
   -49, -46, -43, -40, -37, -34, -31, -28,
   -25, -22, -19, -16, -12,  -9,  -6,  -3
};

const float radius1  = 65.2, radius2  = 92.0, radius3  = 163.2, radius4  = 176.8,
            centerx1 = 64.4, centerx2 = 46.4, centerx3 =  93.6, centerx4 =  16.4, 
            centery1 = 34.8, centery2 = 26.0, centery3 =  56.0, centery4 = -11.6;
float       angle1   =  0.0, angle2   =  0.0, angle3   =   0.0, angle4   =   0.0;
long        hueShift =  0;
/*******************************************/



/************ PROTOTYPES **************/
int setMode(String command);
void quickWeather();
void getWeather();
void processWeather(const char *name, const char *data);
void showWeather();
void drawWeatherIcon(uint8_t x, uint8_t y, int id);
void scrollBigMessage(char *m);
void scrollMessage(char* top, char* bottom ,uint8_t top_font_size,uint8_t bottom_font_size, uint16_t top_color, uint16_t bottom_color);
void pacClear();
void pacMan();
void drawPac(int x, int y, int z);
void drawGhost( int x, int y, int color);
void drawScaredGhost( int x, int y);
void cls();
void pong();
byte pong_get_ball_endpoint(float tempballpos_x, float  tempballpos_y, float  tempballvel_x, float tempballvel_y);
void normal_clock();
void vectorNumber(int n, int x, int y, int color, float scale_x, float scale_y);
void word_clock();
void jumble();
void display_date();
void flashing_cursor(byte xpos, byte ypos, byte cursor_width, byte cursor_height, byte repeats);
void drawString(int x, int y, char* c,uint8_t font_size, uint16_t color);
void drawChar(int x, int y, char c, uint8_t font_size, uint16_t color);
int calc_font_displacement(uint8_t font_size);
void spectrumDisplay();
void plasma();
void marquee();
/*************************************/




void setup() {

	unsigned long resetTime;

	// !!!!! Add timeout code here if can't connect
	Spark.connect();
	while (!Spark.connected()) Spark.process();
	
	// May need to move to loop() if can't connect
	Spark.publish("RGBPongClock", RGBPCversion, 60, PRIVATE);
	Spark.process();	// Force processing of Spark.publish()
	// ************************************************

	
#if defined (DEBUGME)
	Serial.begin(115200);
#endif

	Spark.variable("city", city, STRING);	// !!! FOR DEBUGGING ONLY !!!
	Spark.variable("cmode", &clock_mode, INT);
	
	Spark.function("setMode", setMode);		// Receive mode commands
	Spark.subscribe(HOOK_RESP, processWeather, MY_DEVICES);	// Lets listen for the hook response
	
	
	// !!!! May need to copy to loop() if disconnect/connect
	do {
		resetTime = Time.now();        // the current time = time of last reset
		delay(10);
	} while (resetTime < 1000000 && millis() < 20000); // wait for a reasonable epoc time, but not longer than 20 seconds

	if(resetTime < 1000000) 
		DEBUGpln("Unable to sync time");
	else
		DEBUGpln("RTC has set been synced");  

	// Needs to be set via Spark.function and store in EEPROM!!
	Time.zone(-4);

	matrix.begin();
	matrix.setTextWrap(false); // Allow text to run off right edge
	matrix.setTextSize(1);
	matrix.setTextColor(matrix.Color333(210, 210, 210));

#if defined useFFT
	memset(peak, 0, sizeof(peak));
	memset(col , 0, sizeof(col));

	for(uint8_t i=0; i<32; i++) {
		minLvlAvg[i] = 0;
		maxLvlAvg[i] = 255;
	}
#endif

	randomSeed(analogRead(A7));
	
	//*** RESTORE CITY FROM EEPROM - IF NOT PREVIOUSLY FLASHED, STORE DEFAULT CITY

	pacMan();
	quickWeather();

	clock_mode = random(0,MAX_CLOCK_MODE-1);
	modeSwitch = millis();
	badWeatherCall = 0;			// counts number of unsuccessful webhook calls, reset after 3 failed calls
	updateCTime = millis();		// Reset 24hr cloud time refresh counter
}


void loop(){

	// Add wifi/cloud connection retry code here

	// !!!  Add code for re-syncing time every 24 hrs  !!!
	if ((millis() - updateCTime) > (24UL * 60UL * 60UL * 1000UL)) {
		Spark.syncTime();
		updateCTime = millis();
	}
	
	
	if (millis() - modeSwitch > 300000UL) {	//Switch modes every 5 mins
		clock_mode++;
		mode_changed = 1;
		modeSwitch = millis();
		if (clock_mode > MAX_CLOCK_MODE-1)
			clock_mode = 0;
		DEBUGp("Switch mode to ");
		DEBUGpln(clock_mode);
	}
	
	DEBUGp("in loop ");
	DEBUGpln(millis());
	//reset clock type clock_mode
	switch (clock_mode){
	case 0: 
		normal_clock(); 
		break; 
	case 1: 
		pong(); 
		break;
	case 2: 
		word_clock(); 
		break;
	case 3: 
		jumble(); 
		break; 
	case 4: 
		spectrumDisplay();
		break;
	case 5: 
		plasma();
		break;
	case 6: 
		marquee();
		break;
	default:
		normal_clock();
		break;
	}

	//if the mode hasn't changed, show the date
	pacClear();
	if (mode_changed == 0) { 
		display_date(); 
		pacClear();
	}
	else {
		//the mode has changed, so don't bother showing the date, just go to the new mode.
		mode_changed = 0; //reset mdoe flag.
	}
}

int setMode(String command)
{
/*
	int p = 0;
	while (p<(int)command.length()) {
		int i = command.indexOf(',',p);
		if (i<0) i = command.length();
		
		int j = command.indexOf('=',p);
		if (j<0) return -1;
		
		String key = command.substring(p,j);
		String value = command.substring(j+1,i);
		int val = value.toInt();
		
		// global params
		if(key == "normal")
		{
			mode_changed = 1;
			clock_mode = 0;
		}
		else if(key == "pong")
		{
			mode_changed = 1;
			clock_mode = 1;
		}
		else if(key == "word")
		{
			mode_changed = 1;
			clock_mode = 2;
		}
		else if(key == "jumble")
		{
			mode_changed = 1;
			clock_mode = 3;
		}
		else if(key == "spectrum")
		{
			mode_changed = 1;
			clock_mode = 4;
		}
		else if(key == "quick")
		{
			mode_quick = true;
			return 1;
		}
		else if(key == "plasma")
		{
			mode_changed = 1;
			clock_mode = 5;
		}
		else if(key == "marquee")
		{
			mode_changed = 1;
			clock_mode = 6;
		}
		else if(key == "city")
		{
			unsigned char tmp[20] = "";
			int p = value.length();
			//command.getBytes(value, 0, j+1);
			command.getBytes(value, 0, p);
			strcpy(city, "{\"mycity\": \"");
			strcat(city, (const char *)tmp);
			strcat(city, "\" }");
			weatherGood = false;
			return 1;
		}
		if (mode_changed == 1) {
			modeSwitch = millis();
			return 1;
		}	  
		else return -1;

		p = i+1;
		}
	return 1;
}
*/	
	
	mode_changed = 0;

	int j = command.indexOf('=',0);
	if (j>0) {	// "=" is used when setting city only
		if(command.substring(0,j) == "city")
		{
			unsigned char tmp[20] = "";
			int p = command.length();
			command.getBytes(tmp, (p-j), j+1);
			strcpy(city, "\"mycity\": \"");
			strcat(city, (const char *)tmp);
			strcat(city, "\" ");
			weatherGood = false;
			return 1;
		}		
	}
	else if(command == "normal")
	{
		mode_changed = 1;
		clock_mode = 0;
	}
	else if(command == "pong")
	{
		mode_changed = 1;
		clock_mode = 1;
	}
	else if(command == "word")
	{
		mode_changed = 1;
		clock_mode = 2;
	}
	else if(command == "jumble")
	{
		mode_changed = 1;
		clock_mode = 3;
	}
	else if(command == "spectrum")
	{
		mode_changed = 1;
		clock_mode = 4;
	}
	else if(command == "quick")
	{
		mode_quick = true;
		return 1;
	}
	else if(command == "plasma")
	{
		mode_changed = 1;
		clock_mode = 5;
	}
	else if(command == "marquee")
	{
		mode_changed = 1;
		clock_mode = 6;
	}
	if (mode_changed == 1) {
		modeSwitch = millis();
		return 1;
	}	  
	else return -1;
	
}


//*****************Weather Stuff*********************

void quickWeather(){
	getWeather();
	if(weatherGood){
		showWeather();
		//*** If city has changed, then since weatherGood then store city in EEPROM ***
	}
	else{
		cls();
		matrix.drawPixel(0,0,matrix.Color333(1,0,0));
		matrix.swapBuffers(true);
		Spark.process();
		delay(1000);
	}
}

void getWeather(){
	DEBUGpln("in getWeather");
	char vars[90];
	
	strcpy(vars, "{");
	strcat(vars, city);
	strcat(vars, ",");
	strcat(vars, UNITS);
	strcat(vars, ",");
	strcat(vars, API_KEY);
	strcat(vars, "}");
	
	weatherGood = false;
	// publish the event with city data that will trigger the webhook
	//Spark.publish(HOOK_PUB, city, 60, PRIVATE);
	Spark.publish(HOOK_PUB, vars, 60, PRIVATE);

	unsigned long wait = millis();
	while(!weatherGood && (millis() < wait + 5000UL))	//wait for subscribe to kick in or 5 secs
		Spark.process();

	if (!weatherGood) {
		DEBUGpln("Weather update failed");
		badWeatherCall++;
		if (badWeatherCall > 4)		//If 3 webhook call fail in a row, do a system reset
			System.reset();
	}
	else
		badWeatherCall = 0;
}


void processWeather(const char *name, const char *data){
	weatherGood = true;
	lastWeatherTime = millis();
	stringPos = strlen((const char *)data);
	DEBUGpln("in process weather");

	memset(&w_temp,0,8*7);
	memset(&w_id,0,8*4);
	int dayCounter =0;
	int itemCounter = 0;
	int tempStringLoc=0;
	boolean dropChar = false;
	
	for (int i=0; i<stringPos; i++){
		if(data[i]=='~'){
			itemCounter++;
			tempStringLoc = 0;
			dropChar = false;
			if(itemCounter>1){
				dayCounter++;
				itemCounter=0;
			}
		}
		else if(data[i]=='.' || data[i]=='"'){
			//if we get a . we want to drop all characters until the next ~
			dropChar=true;
		}
		else{
			if(!dropChar){
				switch(itemCounter){
				case 0:
					w_temp[dayCounter][tempStringLoc++] = data[i];
					break;
				case 1:
					w_id[dayCounter][tempStringLoc++] = data[i];
					break;
				}
			}
		}
	}
}

void showWeather(){
	byte dow = Time.weekday()-1;
	char daynames[7][4]={
		"Sun", "Mon","Tue", "Wed", "Thu", "Fri", "Sat"
	};
	DEBUGpln("in showWeather");
	for(int i = 0 ; i<7; i++){

		int numTemp = atoi(w_temp[i]);
		//fix within range to generate colour value
		if (numTemp<-14) numTemp=-10;
		if (numTemp>34) numTemp =30;
		//add 14 so it falls between 0 and 48
		numTemp = numTemp +14;
		//divide by 3 so value between 0 and 16
		numTemp = numTemp / 3;

		int tempColor;
		if(numTemp<8){
			tempColor = matrix.Color444(0,tempColor/2,7);
		}
		else{
			tempColor = matrix.Color444(7,(7-numTemp/2) ,0); 
		} 

		cls();

		//Display the day on the top line.
		if(i==0){
			drawString(2,2,(char*)"Now",51,matrix.Color444(1,1,1));
		}
		else{
			drawString(2,2,daynames[(dow+i-1) % 7],51,matrix.Color444(0,1,0));
			DEBUGpln(daynames[(dow+i-1)%7]);
		}

		//put the temp underneath
		boolean positive = !(w_temp[i][0]=='-');
		for(int t=0; t<7; t++){
			if(w_temp[i][t]=='-'){
				matrix.drawLine(3,10,4,10,tempColor);
			}
			else if(!(w_temp[i][t]==0)){
				vectorNumber(w_temp[i][t]-'0',t*4+2+(positive*2),8,tempColor,1,1);
			}
		}

		matrix.swapBuffers(true);
		drawWeatherIcon(16,0,atoi(w_id[i]));

		Spark.process();	//Give the background process some lovin'

	}
}

void drawWeatherIcon(uint8_t x, uint8_t y, int id){
	unsigned long start = millis();
	static int rain[12];
	
	for(int r=0; r<13; r++){
		//rain[r]=random(9,18);
		rain[r]=random(9,15);
	}
	int rainColor = matrix.Color333(0,0,1);
	byte intensity=id-(id/10)*10 + 1;

	int deep =0;
	boolean raining = false;
	DEBUGpln(id);
	DEBUGpln(intensity);

	while(millis()<start+5000){
		switch(id/100){
		case 2:
			//Thunder
			matrix.fillRect(x,y,16,16,matrix.Color333(0,0,0));
			matrix.drawBitmap(x,y,cloud_outline,16,16,matrix.Color333(1,1,1));
			if(random(0,10)==3){
				int pos = random(-5,5);
				matrix.drawBitmap(pos+x,y,lightning,16,16,matrix.Color333(1,1,1));
			}
			raining = true;
			break;
		case 3:  
			//drizzle
			matrix.fillRect(x,y,16,16,matrix.Color333(0,0,0));
			matrix.drawBitmap(x,y,cloud,16,16,matrix.Color333(1,1,1));
			raining=true;
			break;
		case 5:
			//rain was 5
			matrix.fillRect(x,y,16,16,matrix.Color333(0,0,0));
			
			if(intensity<3){
				matrix.drawBitmap(x,y,cloud,16,16,matrix.Color333(1,1,1));
			}
			else{
				matrix.drawBitmap(x,y,cloud_outline,16,16,matrix.Color333(1,1,1));
			}
			raining = true;
			break;
		case 6:
			//snow was 6
			rainColor = matrix.Color333(4,4,4);
			matrix.fillRect(x,y,16,16,matrix.Color333(0,0,0));
			
			deep = (millis()-start)/500;
			if(deep>6) deep=6;

			if(intensity<3){
				matrix.drawBitmap(x,y,cloud,16,16,matrix.Color333(1,1,1));
				matrix.fillRect(x,y+16-deep/2,16,deep/2,rainColor);
			}
			else{
				matrix.drawBitmap(x,y,cloud_outline,16,16,matrix.Color333(1,1,1));
				matrix.fillRect(x,y+16-(deep),16,deep,rainColor);
			}
			raining = true;
			break;  
		case 7:
			//atmosphere
			matrix.drawRect(x,y,16,16,matrix.Color333(1,0,0));
			drawString(x+2,y+6,(char*)"FOG",51,matrix.Color333(1,1,1));
			break;
		case 8:
			//cloud
			matrix.fillRect(x,y,16,16,matrix.Color333(0,0,1));
			if(id==800){
				matrix.drawBitmap(x,y,big_sun,16,16,matrix.Color333(2,2,0));
			}
			else{
				if(id==801){
					matrix.drawBitmap(x,y,big_sun,16,16,matrix.Color333(2,2,0));
					matrix.drawBitmap(x,y,cloud,16,16,matrix.Color333(1,1,1));
				}
				else{
					if(id==802 || id ==803){
						matrix.drawBitmap(x,y,small_sun,16,16,matrix.Color333(1,1,0));
					}
					matrix.drawBitmap(x,y,cloud,16,16,matrix.Color333(1,1,1));
					matrix.drawBitmap(x,y,cloud_outline,16,16,matrix.Color333(0,0,0));
				}
			}
			break;
		case 9:
			//extreme
			matrix.fillRect(x,y,16,16,matrix.Color333(0,0,0));
			matrix.drawRect(x,y,16,16,matrix.Color333(7,0,0));
			if(id==906){
				raining =true; 
				intensity=3;
				matrix.drawBitmap(x,y,cloud,16,16,matrix.Color333(1,1,1));
			};
			break;
		default:
			matrix.fillRect(x,y,16,16,matrix.Color333(0,1,1));
			matrix.drawBitmap(x,y,big_sun,16,16,matrix.Color333(2,2,0));
			break;    
		}
		if(raining){
			for(int r = 0; r<13; r++){
				matrix.drawPixel(x+r+2, rain[r]++, rainColor);
				if(rain[r]==16) rain[r]=9;
				//if(rain[r]==20) rain[r]=9;
			}
		} 
		matrix.swapBuffers(false);
		Spark.process();	//Give the background process some lovin'
		delay(( 50 -( intensity * 10 )) < 0 ? 0: 50-intensity*10);
	}
}
//*****************End Weather Stuff*********************


void scrollBigMessage(char *m){
	matrix.setTextSize(1);
	int l = (strlen(m)*-6) - 32;
	for(int i = 32; i > l; i--){
		cls();
		matrix.setCursor(i,1);
		matrix.setTextColor(matrix.Color444(1,1,1));
		matrix.print(m);
		matrix.swapBuffers(false);
		delay(50);
		Spark.process();
	}

}

void scrollMessage(char* top, char* bottom ,uint8_t top_font_size,uint8_t bottom_font_size, uint16_t top_color, uint16_t bottom_color){

	int l = ((strlen(top)>strlen(bottom)?strlen(top):strlen(bottom))*-5) - 32;
	
	for(int i=32; i > l; i--){
		
		if (mode_changed == 1 || mode_quick)
			return;

		cls();
		
		drawString(i,1,top,top_font_size, top_color);
		drawString(i,9,bottom, bottom_font_size, bottom_color);
		matrix.swapBuffers(false);
		delay(50);
		Spark.process();
	}

}


//Runs pacman or other animation, refreshes weather data
void pacClear(){
	DEBUGpln("in pacClear");
	//refresh weather if we havent had it for 30 mins
	//or the last time we had it, it was bad, 
	//or weve never had it before.
	if((millis()>lastWeatherTime+1800000) || lastWeatherTime==0 || !weatherGood) getWeather();

	if(!wasWeatherShownLast && weatherGood){
		showWeather();
		wasWeatherShownLast = true;
	}
	else{  
		wasWeatherShownLast = false;

		pacMan();
	}
}  


void pacMan(){
#if defined (usePACMAN)
	DEBUGpln("in pacMan");
	if(powerPillEaten>0){
		for(int i =32+(powerPillEaten*17); i>-17; i--){
			long nowish = millis();
			cls();

			drawPac(i,0,-1);
			if(powerPillEaten>0) drawScaredGhost(i-17,0);
			if(powerPillEaten>1) drawScaredGhost(i-34,0);
			if(powerPillEaten>2) drawScaredGhost(i-51,0);
			if(powerPillEaten>3) drawScaredGhost(i-68,0);

			matrix.swapBuffers(false);    
			while(millis()-nowish<50) Spark.process();	//Give the background process some lovin'
		}
		powerPillEaten = 0;
	}
	else{  

		int hasEaten = 0;

		int powerPill = random(0,5);
		int numGhosts=random(0,4);
		if(powerPill ==0){
			if(numGhosts==0) numGhosts++;
			powerPillEaten = numGhosts;
		}

		for(int i=-17; i<32+(numGhosts*17); i++){
			cls();
			long nowish = millis();
			for(int j = 0; j<6;j++){

				if( j*5> i){
					if(powerPill==0 && j==4){
						matrix.fillCircle(j*5,8,2,matrix.Color333(7,3,0));
					}
					else{
						matrix.fillRect(j*5,8,2,2,matrix.Color333(7,3,0));
					}
				}
			}

			if(i==19 && powerPill == 0) hasEaten=1;
			drawPac(i,0,1);
			if(hasEaten == 0){
				if(numGhosts>0) drawGhost(i-17,0,matrix.Color333(3,0,3));
				if(numGhosts>1) drawGhost(i-34,0,matrix.Color333(3,0,0));
				if(numGhosts>2) drawGhost(i-51,0,matrix.Color333(0,3,3));
				if(numGhosts>3) drawGhost(i-68,0,matrix.Color333(7,3,0));
			}
			else{
				if(numGhosts>0) drawScaredGhost(i-17-(i-19)*2,0);
				if(numGhosts>1) drawScaredGhost(i-34-(i-19)*2,0);
				if(numGhosts>2) drawScaredGhost(i-51-(i-19)*2,0);
				if(numGhosts>3) drawScaredGhost(i-68-(i-19)*2,0);
			}
			matrix.swapBuffers(false);
			while(millis()-nowish<50) Spark.process();	//Give the background process some lovin'
		}
	}
#endif //usePACMAN
}

#if defined (usePACMAN)
void drawPac(int x, int y, int z){
	int c = matrix.Color333(3,3,0);
	if(x>-16 && x<32){
		if(abs(x)%4==0){
			matrix.drawBitmap(x,y,(z>0?pac:pac_left),16,16,c);
		}
		else if(abs(x)%4==1 || abs(x)%4==3){
			matrix.drawBitmap(x,y,(z>0?pac2:pac_left2),16,16,c);
		}
		else{
			matrix.drawBitmap(x,y,(z>0?pac3:pac_left3),16,16,c);
		}
	}
}

void drawGhost( int x, int y, int color){
	if(x>-16 && x<32){
		if(abs(x)%8>3){
			matrix.drawBitmap(x,y,blinky,16,16,color);
		}
		else{
			matrix.drawBitmap(x,y,blinky2,16,16,color);
		}
		matrix.drawBitmap(x,y,eyes1,16,16,matrix.Color333(3,3,3));
		matrix.drawBitmap(x,y,eyes2,16,16,matrix.Color333(0,0,7));
	}
}  

void drawScaredGhost( int x, int y){
	if(x>-16 && x<32){
		if(abs(x)%8>3){
			matrix.drawBitmap(x,y,blinky,16,16,matrix.Color333(0,0,7));
		}
		else{
			matrix.drawBitmap(x,y,blinky2,16,16,matrix.Color333(0,0,7));
		}
		matrix.drawBitmap(x,y,scared,16,16,matrix.Color333(7,3,2));
	}
}  
#endif  //usePACMAN


void cls(){
	matrix.fillScreen(0);
}

void pong(){
	DEBUGpln("in Pong");
	matrix.setTextSize(1);
	matrix.setTextColor(matrix.Color333(2, 2, 2));

	float ballpos_x, ballpos_y;
	float ballvel_x, ballvel_y;
	int bat1_y = 5;  //bat starting y positions
	int bat2_y = 5;  
	int bat1_target_y = 5;  //bat targets for bats to move to
	int bat2_target_y = 5;
	byte bat1_update = 1;  //flags - set to update bat position
	byte bat2_update = 1;
	byte bat1miss, bat2miss; //flags set on the minute or hour that trigger the bats to miss the ball, thus upping the score to match the time.
	byte restart = 1;   //game restart flag - set to 1 initially to setup 1st game

	cls();

//	for(int i=0; i< SHOWCLOCK; i++) {
	int showTime = Time.now();
	
	while((Time.now() - showTime) < showClock) {
		cls();
		//draw pitch centre line
		int adjust = 0;
		if(Time.second()%2==0)adjust=1;
		for (byte i = 0; i <16; i++) {
			if ( i % 2 == 0 ) { //plot point if an even number
				matrix.drawPixel(16,i+adjust,matrix.Color333(0,4,0));
			}
		} 

		//main pong game loop
		if (mode_changed == 1)
		return;
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			pong();
			return;
		}	

		int ampm=0;
		//update score / time
		byte mins = Time.minute();
		byte hours = Time.hour();
		if (hours > 12) {
			hours = hours - ampm * 12;
		}
		if (hours < 1) {
			hours = hours + ampm * 12;
		}

		char buffer[3];

		itoa(hours,buffer,10);
		//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
		if (hours < 10) {
			buffer[1] = buffer[0];
			buffer[0] = '0';
		}
		vectorNumber(buffer[0]-'0',8,1,matrix.Color333(1,1,1),1,1);
		vectorNumber(buffer[1]-'0',12,1,matrix.Color333(1,1,1),1,1);

		itoa(mins,buffer,10); 
		if (mins < 10) {
			buffer[1] = buffer[0];
			buffer[0] = '0';
		} 
		vectorNumber(buffer[0]-'0',18,1,matrix.Color333(1,1,1),1,1);
		vectorNumber(buffer[1]-'0',22,1,matrix.Color333(1,1,1),1,1);

		//if restart flag is 1, setup a new game
		if (restart) {
			//set ball start pos
			ballpos_x = 16;
			ballpos_y = random (4,12);

			//pick random ball direction
			if (random(0,2) > 0) {
				ballvel_x = 1; 
			} 
			else {
				ballvel_x = -1;
			}
			if (random(0,2) > 0) {
				ballvel_y = 0.5; 
			} 
			else {
				ballvel_y = -0.5;
			}
			//draw bats in initial positions
			bat1miss = 0; 
			bat2miss = 0;
			//reset game restart flag
			restart = 0;
		}

		//if coming up to the minute: secs = 59 and mins < 59, flag bat 2 (right side) to miss the return so we inc the minutes score
		if (Time.second() == 59 && Time.minute() < 59){
			bat1miss = 1;
		}
		// if coming up to the hour: secs = 59  and mins = 59, flag bat 1 (left side) to miss the return, so we inc the hours score.
		if (Time.second() == 59 && Time.minute() == 59){
			bat2miss = 1;
		}

		//AI - we run 2 sets of 'AI' for each bat to work out where to go to hit the ball back 
		//very basic AI...
		// For each bat, First just tell the bat to move to the height of the ball when we get to a random location.
		//for bat1
		if (ballpos_x == random(18,32)){
			bat1_target_y = ballpos_y;
		}
		//for bat2
		if (ballpos_x == random(4,16)){
			bat2_target_y = ballpos_y;
		}

		//when the ball is closer to the left bat, run the ball maths to find out where the ball will land
		if (ballpos_x == 15 && ballvel_x < 0) {

			byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

			//if the miss flag is set,  then the bat needs to miss the ball when it gets to end_ball_y
			if (bat1miss == 1){
				bat1miss = 0;
				if ( end_ball_y > 8){
					bat1_target_y = random (0,3); 
				} 
				else {
					bat1_target_y = 8 + random (0,3);              
				}      
			} 
			//if the miss flag isn't set,  set bat target to ball end point with some randomness so its not always hitting top of bat
			else {
				bat1_target_y = end_ball_y - random (0, 6);        
				//check not less than 0
				if (bat1_target_y < 0){
					bat1_target_y = 0;
				}
				if (bat1_target_y > 10){
					bat1_target_y = 10;
				} 
			}
		}

		//right bat AI
		//if positive velocity then predict for right bat - first just match ball height
		//when the ball is closer to the right bat, run the ball maths to find out where it will land
		if (ballpos_x == 17 && ballvel_x > 0) {

			byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

			//if flag set to miss, move bat out way of ball
			if (bat2miss == 1){
				bat2miss = 0;
				//if ball end point above 8 then move bat down, else move it up- so either way it misses
				if (end_ball_y > 8){
					bat2_target_y = random (0,3); 
				} 
				else {
					bat2_target_y = 8 + random (0,3);
				}      
			} 
			else {
				//set bat target to ball end point with some randomness 
				bat2_target_y =  end_ball_y - random (0,6);
				//ensure target between 0 and 15
				if (bat2_target_y < 0){
					bat2_target_y = 0;
				} 
				if (bat2_target_y > 10){
					bat2_target_y = 10;
				} 
			}
		}

		//move bat 1 towards target    
		//if bat y greater than target y move down until hit 0 (dont go any further or bat will move off screen)
		if (bat1_y > bat1_target_y && bat1_y > 0 ) {
			bat1_y--;
			bat1_update = 1;
		}

		//if bat y less than target y move up until hit 10 (as bat is 6)
		if (bat1_y < bat1_target_y && bat1_y < 10) {
			bat1_y++;
			bat1_update = 1;
		}

		//draw bat 1
		if (bat1_update){
			matrix.fillRect(BAT1_X-1,bat1_y,2,6,matrix.Color333(0,0,4));
		}

		//move bat 2 towards target (dont go any further or bat will move off screen)
		//if bat y greater than target y move down until hit 0
		if (bat2_y > bat2_target_y && bat2_y > 0 ) {
			bat2_y--;
			bat2_update = 1;
		}

		//if bat y less than target y move up until hit max of 10 (as bat is 6)
		if (bat2_y < bat2_target_y && bat2_y < 10) {
			bat2_y++;
			bat2_update = 1;
		}

		//draw bat2
		if (bat2_update){
			matrix.fillRect(BAT2_X+1,bat2_y,2,6,matrix.Color333(0,0,4));
		}

		//update the ball position using the velocity
		ballpos_x =  ballpos_x + ballvel_x;
		ballpos_y =  ballpos_y + ballvel_y;

		//check ball collision with top and bottom of screen and reverse the y velocity if either is hit
		if (ballpos_y <= 0 ){
			ballvel_y = ballvel_y * -1;
			ballpos_y = 0; //make sure value goes no less that 0
		}

		if (ballpos_y >= 15){
			ballvel_y = ballvel_y * -1;
			ballpos_y = 15; //make sure value goes no more than 15
		}

		//check for ball collision with bat1. check ballx is same as batx
		//and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c) 
		if ((int)ballpos_x == BAT1_X+1 && (bat1_y <= (int)ballpos_y && (int)ballpos_y <= bat1_y + 5) ) { 

			//random if bat flicks ball to return it - and therefor changes ball velocity
			if(!random(0,3)) { //not true = no flick - just straight rebound and no change to ball y vel
				ballvel_x = ballvel_x * -1;
			} 
			else {
				bat1_update = 1;
				byte flick;  //0 = up, 1 = down.

				if (bat1_y > 1 || bat1_y < 8){
					flick = random(0,2);   //pick a random dir to flick - up or down
				}

				//if bat 1 or 2 away from top only flick down
				if (bat1_y <=1 ){
					flick = 0;   //move bat down 1 or 2 pixels 
				} 
				//if bat 1 or 2 away from bottom only flick up
				if (bat1_y >=  8 ){
					flick = 1;  //move bat up 1 or 2 pixels 
				}

				switch (flick) {
					//flick up
				case 0:
					bat1_target_y = bat1_target_y + random(1,3);
					ballvel_x = ballvel_x * -1;
					if (ballvel_y < 2) {
						ballvel_y = ballvel_y + 0.2;
					}
					break;

					//flick down
				case 1:   
					bat1_target_y = bat1_target_y - random(1,3);
					ballvel_x = ballvel_x * -1;
					if (ballvel_y > 0.2) {
						ballvel_y = ballvel_y - 0.2;
					}
					break;
				}
			}
		}

		//check for ball collision with bat2. check ballx is same as batx
		//and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c) 
		if ((int)ballpos_x == BAT2_X && (bat2_y <= (int)ballpos_y && (int)ballpos_y <= bat2_y + 5) ) { 

			//random if bat flicks ball to return it - and therefor changes ball velocity
			if(!random(0,3)) {
				ballvel_x = ballvel_x * -1;    //not true = no flick - just straight rebound and no change to ball y vel
			} 
			else {
				bat1_update = 1;
				byte flick;  //0 = up, 1 = down.

				if (bat2_y > 1 || bat2_y < 8){
					flick = random(0,2);   //pick a random dir to flick - up or down
				}
				//if bat 1 or 2 away from top only flick down
				if (bat2_y <= 1 ){
					flick = 0;  //move bat up 1 or 2 pixels 
				} 
				//if bat 1 or 2 away from bottom only flick up
				if (bat2_y >=  8 ){
					flick = 1;   //move bat down 1 or 2 pixels 
				}

				switch (flick) {
					//flick up
				case 0:
					bat2_target_y = bat2_target_y + random(1,3);
					ballvel_x = ballvel_x * -1;
					if (ballvel_y < 2) {
						ballvel_y = ballvel_y + 0.2;
					}
					break;

					//flick down
				case 1:   
					bat2_target_y = bat2_target_y - random(1,3);
					ballvel_x = ballvel_x * -1;
					if (ballvel_y > 0.2) {
						ballvel_y = ballvel_y - 0.2;
					}
					break;
				}
			}
		}

		//plot the ball on the screen
		byte plot_x = (int)(ballpos_x + 0.5f);
		byte plot_y = (int)(ballpos_y + 0.5f);

		matrix.drawPixel(plot_x,plot_y,matrix.Color333(4, 0, 0));

		//check if a bat missed the ball. if it did, reset the game.
		if ((int)ballpos_x == 0 ||(int) ballpos_x == 32){
			restart = 1; 
		}

		Spark.process();	//Give the background process some lovin'
		delay(40);
		matrix.swapBuffers(false);
	} 
}
byte pong_get_ball_endpoint(float tempballpos_x, float  tempballpos_y, float  tempballvel_x, float tempballvel_y) {

	//run prediction until ball hits bat
	while (tempballpos_x > BAT1_X && tempballpos_x < BAT2_X  ){
		tempballpos_x = tempballpos_x + tempballvel_x;
		tempballpos_y = tempballpos_y + tempballvel_y;
		//check for collisions with top / bottom
		if (tempballpos_y <= 0 || tempballpos_y >= 15){
			tempballvel_y = tempballvel_y * -1;
		}    
	}  
	return tempballpos_y; 
}

void normal_clock()
{
	DEBUGpln("in normal_clock");
	matrix.setTextWrap(false); // Allow text to run off right edge
	matrix.setTextSize(2);
	matrix.setTextColor(matrix.Color333(2, 3, 2));

	cls();
	byte hours = Time.hour();
	byte mins = Time.minute();

	int  msHourPosition = 0;
	int  lsHourPosition = 0;
	int  msMinPosition = 0;
	int  lsMinPosition = 0;      
	int  msLastHourPosition = 0;
	int  lsLastHourPosition = 0;
	int  msLastMinPosition = 0;
	int  lsLastMinPosition = 0;      

	//Start with all characters off screen
	int c1 = -17;
	int c2 = -17;
	int c3 = -17;
	int c4 = -17;

	float scale_x =2.5;
	float scale_y =3.0;


	char lastHourBuffer[3]="  ";
	char lastMinBuffer[3] ="  ";

	//loop to display the clock for a set duration of SHOWCLOCK
	//for (int show = 0; show < SHOWCLOCK ; show++) {
	int showTime = Time.now();
	
	while((Time.now() - showTime) < showClock) {
		cls();

		if (mode_changed == 1)
		return;
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			normal_clock();
			return;
		}

		//udate mins and hours with the new time
		mins = Time.minute();
		hours = Time.hour();

		char buffer[3];

		itoa(hours,buffer,10);
		//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
		if (hours < 10) {
			buffer[1] = buffer[0];
			buffer[0] = '0';
		}

		if(lastHourBuffer[0]!=buffer[0] && c1==0) c1= -17;
		if( c1 < 0 )c1++;
		msHourPosition = c1;
		msLastHourPosition = c1 + 17;

		if(lastHourBuffer[1]!=buffer[1] && c2==0) c2= -17;
		if( c2 < 0 )c2++;
		lsHourPosition = c2;
		lsLastHourPosition = c2 + 17;

		//update the display
		//shadows first
		vectorNumber((lastHourBuffer[0]-'0'), 2, 2+msLastHourPosition, matrix.Color444(0,0,1),scale_x,scale_y);
		vectorNumber((lastHourBuffer[1]-'0'), 9, 2+lsLastHourPosition, matrix.Color444(0,0,1),scale_x,scale_y);
		vectorNumber((buffer[0]-'0'), 2, 2+msHourPosition, matrix.Color444(0,0,1),scale_x,scale_y);
		vectorNumber((buffer[1]-'0'), 9, 2+lsHourPosition, matrix.Color444(0,0,1),scale_x,scale_y); 

		vectorNumber((lastHourBuffer[0]-'0'), 1, 1+msLastHourPosition, matrix.Color444(1,1,1),scale_x,scale_y);
		vectorNumber((lastHourBuffer[1]-'0'), 8, 1+lsLastHourPosition, matrix.Color444(1,1,1),scale_x,scale_y);
		vectorNumber((buffer[0]-'0'), 1, 1+msHourPosition, matrix.Color444(1,1,1),scale_x,scale_y);
		vectorNumber((buffer[1]-'0'), 8, 1+lsHourPosition, matrix.Color444(1,1,1),scale_x,scale_y);    

		if(c1==0) lastHourBuffer[0]=buffer[0];
		if(c2==0) lastHourBuffer[1]=buffer[1];

		matrix.fillRect(16,5,2,2,matrix.Color444(0,0,Time.second()%2));
		matrix.fillRect(16,11,2,2,matrix.Color444(0,0,Time.second()%2));

		matrix.fillRect(15,4,2,2,matrix.Color444(Time.second()%2,Time.second()%2,Time.second()%2));
		matrix.fillRect(15,10,2,2,matrix.Color444(Time.second()%2,Time.second()%2,Time.second()%2));

		itoa (mins, buffer, 10);
		if (mins < 10) {
			buffer[1] = buffer[0];
			buffer[0] = '0';
		}

		if(lastMinBuffer[0]!=buffer[0] && c3==0) c3= -17;
		if( c3 < 0 )c3++;
		msMinPosition = c3;
		msLastMinPosition= c3 + 17;

		if(lastMinBuffer[1]!=buffer[1] && c4==0) c4= -17;
		if( c4 < 0 )c4++;
		lsMinPosition = c4;
		lsLastMinPosition = c4 + 17;

		vectorNumber((buffer[0]-'0'), 19, 2+msMinPosition, matrix.Color444(0,0,1),scale_x,scale_y);
		vectorNumber((buffer[1]-'0'), 26, 2+lsMinPosition, matrix.Color444(0,0,1),scale_x,scale_y);
		vectorNumber((lastMinBuffer[0]-'0'), 19, 2+msLastMinPosition, matrix.Color444(0,0,1),scale_x,scale_y);
		vectorNumber((lastMinBuffer[1]-'0'), 26, 2+lsLastMinPosition, matrix.Color444(0,0,1),scale_x,scale_y);

		vectorNumber((buffer[0]-'0'), 18, 1+msMinPosition, matrix.Color444(1,1,1),scale_x,scale_y);
		vectorNumber((buffer[1]-'0'), 25, 1+lsMinPosition, matrix.Color444(1,1,1),scale_x,scale_y);
		vectorNumber((lastMinBuffer[0]-'0'), 18, 1+msLastMinPosition, matrix.Color444(1,1,1),scale_x,scale_y);
		vectorNumber((lastMinBuffer[1]-'0'), 25, 1+lsLastMinPosition, matrix.Color444(1,1,1),scale_x,scale_y);

		if(c3==0) lastMinBuffer[0]=buffer[0];
		if(c4==0) lastMinBuffer[1]=buffer[1];

		matrix.swapBuffers(false); 
		Spark.process();	//Give the background process some lovin'
	}
}

//Draw number n, with x,y as top left corner, in chosen color, scaled in x and y.
//when scale_x, scale_y = 1 then character is 3x5
void vectorNumber(int n, int x, int y, int color, float scale_x, float scale_y){

	switch (n){
	case 0:
		matrix.drawLine(x ,y , x , y+(4*scale_y) , color);
		matrix.drawLine(x , y+(4*scale_y) , x+(2*scale_x) , y+(4*scale_y), color);
		matrix.drawLine(x+(2*scale_x) , y , x+(2*scale_x) , y+(4*scale_y) , color);
		matrix.drawLine(x ,y , x+(2*scale_x) , y , color);
		break; 
	case 1: 
		matrix.drawLine( x+(1*scale_x), y, x+(1*scale_x),y+(4*scale_y), color);  
		matrix.drawLine(x , y+4*scale_y , x+2*scale_x , y+4*scale_y,color);
		matrix.drawLine(x,y+scale_y, x+scale_x, y,color);
		break;
	case 2:
		matrix.drawLine(x ,y , x+2*scale_x , y , color);
		matrix.drawLine(x+2*scale_x , y , x+2*scale_x , y+2*scale_y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x , y+2*scale_y, color);
		matrix.drawLine(x , y+2*scale_y, x , y+4*scale_y,color);
		matrix.drawLine(x , y+4*scale_y , x+2*scale_x , y+4*scale_y,color);
		break; 
	case 3:
		matrix.drawLine(x ,y , x+2*scale_x , y , color);
		matrix.drawLine(x+2*scale_x , y , x+2*scale_x , y+4*scale_y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x+scale_x , y+2*scale_y, color);
		matrix.drawLine(x , y+4*scale_y , x+2*scale_x , y+4*scale_y,color);
		break;
	case 4:
		matrix.drawLine(x+2*scale_x , y , x+2*scale_x , y+4*scale_y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x , y+2*scale_y, color);
		matrix.drawLine(x ,y , x , y+2*scale_y , color);
		break;
	case 5:
		matrix.drawLine(x ,y , x+2*scale_x , y , color);
		matrix.drawLine(x , y , x , y+2*scale_y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x , y+2*scale_y, color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y, x+2*scale_x , y+4*scale_y,color);
		matrix.drawLine( x , y+4*scale_y , x+2*scale_x , y+4*scale_y,color);
		break; 
	case 6:
		matrix.drawLine(x ,y , x , y+(4*scale_y) , color);
		matrix.drawLine(x ,y , x+2*scale_x , y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x , y+2*scale_y, color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y, x+2*scale_x , y+4*scale_y,color);
		matrix.drawLine(x+2*scale_x , y+4*scale_y , x, y+(4*scale_y) , color);
		break;
	case 7:
		matrix.drawLine(x ,y , x+2*scale_x , y , color);
		matrix.drawLine( x+2*scale_x, y, x+scale_x,y+(4*scale_y), color);
		break;
	case 8:
		matrix.drawLine(x ,y , x , y+(4*scale_y) , color);
		matrix.drawLine(x , y+(4*scale_y) , x+(2*scale_x) , y+(4*scale_y), color);
		matrix.drawLine(x+(2*scale_x) , y , x+(2*scale_x) , y+(4*scale_y) , color);
		matrix.drawLine(x ,y , x+(2*scale_x) , y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x , y+2*scale_y, color);
		break;
	case 9:
		matrix.drawLine(x ,y , x , y+(2*scale_y) , color);
		matrix.drawLine(x , y+(4*scale_y) , x+(2*scale_x) , y+(4*scale_y), color);
		matrix.drawLine(x+(2*scale_x) , y , x+(2*scale_x) , y+(4*scale_y) , color);
		matrix.drawLine(x ,y , x+(2*scale_x) , y , color);
		matrix.drawLine(x+2*scale_x , y+2*scale_y , x , y+2*scale_y, color);
		break;    
	}
}


//print a clock using words rather than numbers
void word_clock() {
	DEBUGpln("in word_clock");
	cls();

	char numbers[19][10]   = { 
		"one", "two", "three", "four","five","six","seven","eight","nine","ten",
		"eleven","twelve", "thirteen","fourteen","fifteen","sixteen","7teen","8teen","nineteen"                  };              
	char numberstens[5][7] = { 
		"ten","twenty","thirty","forty","fifty"                   };

//	byte hours_y, mins_y; //hours and mins and positions for hours and mins lines  

	byte hours = Time.hour();
	byte mins  = Time.minute();

	//loop to display the clock for a set duration of SHOWCLOCK
	for (int show = 0; show < SHOWCLOCK ; show++) {
		
		if (mode_changed == 1)
			return;
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			word_clock();
			return;
		}

		//print the time if it has changed or if we have just come into the subroutine
		if ( show == 0 || mins != Time.minute() ) {  

			//reset these for comparison next time
			mins = Time.minute();   
			hours = Time.hour();

			//make hours into 12 hour format
			if (hours > 12){ 
				hours = hours - 12; 
			}
			if (hours == 0){ 
				hours = 12; 
			} 

			//split mins value up into two separate digits 
			int minsdigit = mins % 10;
			byte minsdigitten = (mins / 10) % 10;

			char str_top[8];
			char str_bot[8];
			char str_mid[8];

			//if mins <= 10 , then top line has to read "minsdigti past" and bottom line reads hours
			if (mins < 10) {     
				strcpy (str_top,numbers[minsdigit - 1]);
				strcpy (str_mid,"PAST");
				strcpy (str_bot,numbers[hours - 1]);
			}
			//if mins = 10, cant use minsdigit as above, so soecial case to print 10 past /n hour.
			if (mins == 10) {     
				strcpy (str_top,numbers[9]);
				strcpy (str_mid,"PAST");
				strcpy (str_bot,numbers[hours - 1]);
			}

			//if time is not on the hour - i.e. both mins digits are not zero, 
			//then make top line read "hours" and bottom line ready "minstens mins" e.g. "three /n twenty one"
			else if (minsdigitten != 0 && minsdigit != 0  ) {

				strcpy (str_top,numbers[hours - 1]); 

				//if mins is in the teens, use teens from the numbers array for the bottom line, e.g. "three /n fifteen"
				if (mins >= 11 && mins <= 19) {
					strcpy (str_bot, numbers[mins - 1]);
					strcpy(str_mid," ");
					//else bottom line reads "minstens mins" e.g. "three \n twenty three"
				} 
				else {     
					strcpy (str_mid, numberstens[minsdigitten - 1]);
					strcpy (str_bot, numbers[minsdigit -1]);
				}
			}
			// if mins digit is zero, don't print it. read read "hours" "minstens" e.g. "three /n twenty"
			else if (minsdigitten != 0 && minsdigit == 0  ) {
				strcpy (str_top, numbers[hours - 1]);     
				strcpy (str_bot, numberstens[minsdigitten - 1]);
				strcpy (str_mid, " " );
			}

			//if both mins are zero, i.e. it is on the hour, the top line reads "hours" and bottom line reads "o'clock"
			else if (minsdigitten == 0 && minsdigit == 0  ) {
				strcpy (str_top,numbers[hours - 1]);     
				strcpy (str_bot, "O'CLOCK");
				strcpy (str_mid, " ");
			}

			//work out offset to center top line on display. 
			byte lentop = 0;
			while(str_top[lentop]) { 
				lentop++; 
			}; //get length of message
			byte offset_top;
			if(lentop<6){
				offset_top = (X_MAX - ((lentop*6)-1)) / 2; //
			}
			else{
				offset_top = (X_MAX - ((lentop - 1)*4)) / 2; //
			}

			//work out offset to center bottom line on display. 
			byte lenbot = 0;
			while(str_bot[lenbot]) { 
				lenbot++; 
			}; //get length of message
			byte offset_bot;
			if(lenbot<6){
				offset_bot = (X_MAX - ((lenbot*6)-1)) / 2; //
			}
			else{
				offset_bot = (X_MAX - ((lenbot - 1)*4)) / 2; //
			}

			byte lenmid = 0;
			while(str_mid[lenmid]) { 
				lenmid++; 
			}; //get length of message
			byte offset_mid;
			if(lenmid<6){
				offset_mid = (X_MAX - ((lenmid*6)-1)) / 2; //
			}
			else{
				offset_mid = (X_MAX - ((lenmid - 1)*4)) / 2; //
			}

			cls();
			drawString(offset_top,(lenmid>1?0:2),str_top,(lentop<6?53:51),matrix.Color333(0,1,5));
			if(lenmid>1){
				drawString(offset_mid,5,str_mid,(lenmid<6?53:51),matrix.Color333(1,1,5));
			}
			drawString(offset_bot,(lenmid>1?10:8),str_bot,(lenbot<6?53:51),matrix.Color333(0,5,1));    
			matrix.swapBuffers(false);
		}
		Spark.process();	//Give the background process some lovin'
		delay (50); 
	}
}


//show time and date and use a random jumble of letters transition each time the time changes.
void jumble() {

	char days[7][4] = {
		"SUN","MON","TUE", "WED", "THU", "FRI", "SAT"                  }; //DS1307 outputs 1-7
	char allchars[37] = {
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"                  };
	char endchar[16];
	byte counter[16];
	byte mins = Time.minute();
	byte seq[16];

	DEBUGpln("in Jumble");
	cls();

	for (int show = 0; show < SHOWCLOCK ; show++) {
		
		if (mode_changed == 1)
		return;
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			jumble();
			return;
		}

		if ( show == 0 || mins != Time.minute()  ) {  
			//fill an arry with 0-15 and randomize the order so we can plot letters in a jumbled pattern rather than sequentially
			for (int i=0; i<16; i++) {
				seq[i] = i;  // fill the array in order
			}
			//randomise array of numbers 
			for (int i=0; i<(16-1); i++) {
				int r = i + (rand() % (16-i)); // Random remaining position.
				int temp = seq[i]; 
				seq[i] = seq[r]; 
				seq[r] = temp;
			}

			//reset these for comparison next time
			mins = Time.minute();
			byte hours = Time.hour();   
			byte dow   = Time.weekday() - 1; // the DS1307 outputs 1 - 7. 
			byte date  = Time.day();

			byte alldone = 0;

			//set counters to 50
			for(byte c=0; c<16 ; c++) {
				counter[c] = 3 + random (0,20);
			}

			//set final characters
			char buffer[3];
			itoa(hours,buffer,10);

			//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
			if (hours < 10) {
				buffer[1] = buffer[0];
				buffer[0] = '0';
			}

			endchar[0] = buffer[0];
			endchar[1] = buffer[1];
			endchar[2] = ':';

			itoa (mins, buffer, 10);
			if (mins < 10) {
				buffer[1] = buffer[0];
				buffer[0] = '0';
			}

			endchar[3] = buffer[0];
			endchar[4] = buffer[1];

			itoa (date, buffer, 10);
			if (date < 10) {
				buffer[1] = buffer[0];
				buffer[0] = '0';
			}

			//then work out date 2 letter suffix - eg st, nd, rd, th etc
			char suffix[4][3]={
				"st", "nd", "rd", "th"                                                      };
			byte s = 3; 
			if(date == 1 || date == 21 || date == 31) {
				s = 0;
			} 
			else if (date == 2 || date == 22) {
				s = 1;
			} 
			else if (date == 3 || date == 23) {
				s = 2;
			}
			//set topline
			endchar[5] = ' ';
			endchar[6] = ' ';
			endchar[7] = ' ';

			//set bottom line
			endchar[8] = days[dow][0];
			endchar[9] = days[dow][1];
			endchar[10] = days[dow][2];
			endchar[11] = ' ';
			endchar[12] = buffer[0];
			endchar[13] = buffer[1];
			endchar[14] = suffix[s][0];
			endchar[15] = suffix[s][1];

			byte x = 0;
			byte y = 0;

			//until all counters are 0
			while (alldone < 16){

				//for each char    
				for(byte c=0; c<16 ; c++) {

					if (seq[c] < 8) { 
						x = 0;
						y = 0; 
					} 
					else {
						x = 8;
						y = 8;   
					}

					//if counter > 1 then put random char
					if (counter[ seq[c] ] > 1) {
						matrix.fillRect((seq[c]-x)*4,y,3,5,matrix.Color333(0,0,0));
						drawChar((seq[c] - x) *4, y, allchars[random(0,36)],51,matrix.Color444(1,0,0));
						counter[ seq[c] ]--;
						matrix.swapBuffers(true);
					}

					//if counter == 1 then put final char 
					if (counter[ seq[c] ] == 1) {
						matrix.fillRect((seq[c]-x)*4,y,3,5,matrix.Color444(0,0,0));
						drawChar((seq[c] - x) *4, y, endchar[seq[c]],51,matrix.Color444(0,0,1));
						counter[seq[c]] = 0;
						alldone++;
						matrix.swapBuffers(true);
					} 

					//if counter == 0 then just pause to keep update rate the same
					if (counter[seq[c]] == 0) {
						delay(4);
					}

					if (mode_changed == 1)
					return;
				}
				Spark.process();//Give the background process some lovin'
			}
		}
		delay(50);
	} //showclock
}


void display_date()
{
	DEBUGpln("in display_date");
	uint16_t color = matrix.Color333(0,1,0);
	cls();
	matrix.swapBuffers(true);
	//read the date from the DS1307
	//it returns the month number, day number, and a number representing the day of week - 1 for Tue, 2 for Wed 3 for Thu etc.
	byte dow = Time.weekday()-1;		//we  take one off the value the DS1307 generates, as our array of days is 0-6 and the DS1307 outputs  1-7.
	byte date = Time.day();
	byte mont = Time.month()-1; 

	//array of day and month names to print on the display. Some are shortened as we only have 8 characters across to play with 
	char daynames[7][9]={
		"Sunday", "Monday","Tuesday", "Wed", "Thursday", "Friday", "Saturday"                  };
	char monthnames[12][9]={
		"January", "February", "March", "April", "May", "June", "July", "August", "Sept", "October", "November", "December"                  };

	//call the flashing cursor effect for one blink at x,y pos 0,0, height 5, width 7, repeats 1
	flashing_cursor(0,0,3,5,1);

	//print the day name
	int i = 0;
	while(daynames[dow][i])
	{
		flashing_cursor(i*4,0,3,5,0);
		drawChar(i*4,0,daynames[dow][i],51,color);
		matrix.swapBuffers(true);
		i++;

		if (mode_changed == 1)
		return;
	}

	//pause at the end of the line with a flashing cursor if there is space to print it.
	//if there is no space left, dont print the cursor, just wait.
	if (i*4 < 32){
		flashing_cursor(i*4,0,3,5,1);  
	} 
	else {
		Spark.process();	//Give the background process some lovin'
		delay(300);
	}

	//flash the cursor on the next line  
	flashing_cursor(0,8,3,5,0);

	//print the date on the next line: First convert the date number to chars
	char buffer[3];
	itoa(date,buffer,10);

	//then work out date 2 letter suffix - eg st, nd, rd, th etc
	char suffix[4][3]={
		"st", "nd", "rd", "th"                    };
	byte s = 3; 
	if(date == 1 || date == 21 || date == 31) {
		s = 0;
	} 
	else if (date == 2 || date == 22) {
		s = 1;
	} 
	else if (date == 3 || date == 23) {
		s = 2;
	} 

	//print the 1st date number
	drawChar(0,8,buffer[0],51,color);
	matrix.swapBuffers(true);

	//if date is under 10 - then we only have 1 digit so set positions of sufix etc one character nearer
	byte suffixposx = 4;

	//if date over 9 then print second number and set xpos of suffix to be 1 char further away
	if (date > 9){
		suffixposx = 8;
		flashing_cursor(4,8,3,5,0); 
		drawChar(4,8,buffer[1],51,color);
		matrix.swapBuffers(true);
	}

	//print the 2 suffix characters
	flashing_cursor(suffixposx, 8,3,5,0);
	drawChar(suffixposx,8,suffix[s][0],51,color);
	matrix.swapBuffers(true);

	flashing_cursor(suffixposx+4,8,3,5,0);
	drawChar(suffixposx+4,8,suffix[s][1],51,color);
	matrix.swapBuffers(true);

	//blink cursor after 
	flashing_cursor(suffixposx + 8,8,3,5,1);  

	//replace day name with date on top line - effectively scroll the bottom line up by 8 pixels
	for(int q = 8; q>=0; q--){
		cls();
		int w =0 ;
		while(daynames[dow][w])
		{
			drawChar(w*4,q-8,daynames[dow][w],51,color);

			w++;
		}

		matrix.swapBuffers(true);
		//date first digit
		drawChar(0,q,buffer[0],51,color);
		//date second digit - this may be blank and overwritten if the date is a single number
		drawChar(4,q,buffer[1],51,color);
		//date suffix
		drawChar(suffixposx,q,suffix[s][0],51,color);
		//date suffix
		drawChar(suffixposx+4,q,suffix[s][1],51,color);
		matrix.swapBuffers(true);
		delay(50);
	}
	//flash the cursor for a second for effect
	flashing_cursor(suffixposx + 8,0,3,5,0);  

	//print the month name on the bottom row
	i = 0;
	while(monthnames[mont][i])
	{  
		flashing_cursor(i*4,8,3,5,0);
		drawChar(i*4,8,monthnames[mont][i],51,color);
		matrix.swapBuffers(true);
		i++; 

	}

	//blink the cursor at end if enough space after the month name, otherwise juts wait a while
	if (i*4 < 32){
		flashing_cursor(i*4,8,3,5,2);  
	} 
	else {
		delay(1000);
	}

	for(int q = 8; q>=-8; q--){
		cls();
		int w =0 ;
		while(monthnames[mont][w])
		{
			drawChar(w*4,q,monthnames[mont][w],51,color);

			w++;
		}

		matrix.swapBuffers(true);
		//date first digit
		drawChar(0,q-8,buffer[0],51,color);
		//date second digit - this may be blank and overwritten if the date is a single number
		drawChar(4,q-8,buffer[1],51,color);
		//date suffix
		drawChar(suffixposx,q-8,suffix[s][0],51,color);
		//date suffix
		drawChar(suffixposx+4,q-8,suffix[s][1],51,color);
		matrix.swapBuffers(true);
		delay(50);
	}
}


/*
* flashing_cursor
* print a flashing_cursor at xpos, ypos and flash it repeats times 
*/
void flashing_cursor(byte xpos, byte ypos, byte cursor_width, byte cursor_height, byte repeats)
{
	for (byte r = 0; r <= repeats; r++) {
		matrix.fillRect(xpos,ypos,cursor_width, cursor_height, matrix.Color333(0,3,0));
		matrix.swapBuffers(true);

		if (repeats > 0) {
			delay(400);
		} 
		else {
			delay(70);
		}

		matrix.fillRect(xpos,ypos,cursor_width, cursor_height, matrix.Color333(0,0,0));
		matrix.swapBuffers(true);

		//if cursor set to repeat, wait a while
		if (repeats > 0) {
			delay(400); 
		}
		Spark.process();	//Give the background process some lovin'
	}
}


void drawString(int x, int y, char* c,uint8_t font_size, uint16_t color)
{
	// x & y are positions, c-> pointer to string to disp, update_s: false(write to mem), true: write to disp
	//font_size : 51(ascii value for 3), 53(5) and 56(8)
	for(uint16_t i=0; i< strlen(c); i++)
	{
		drawChar(x, y, c[i],font_size, color);
		x+=calc_font_displacement(font_size); // Width of each glyph
	}
}

int calc_font_displacement(uint8_t font_size)
{
	switch(font_size)
	{
	case 51:
		return 4;  //5x3 hence occupies 4 columns ( 3 + 1(space btw two characters))
		break;

	case 53:
		return 6;
		break;
		//case 56:
		//return 6;
		//break;
	default:
		return 6;
		break;
	}
}

void drawChar(int x, int y, char c, uint8_t font_size, uint16_t color)  // Display the data depending on the font size mentioned in the font_size variable
{

	uint8_t dots;
	if ((c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z')) {
		c &= 0x1F;   // A-Z maps to 1-26
	} 
	else if (c >= '0' && c <= '9') {
		c = (c - '0') + 27;
	} 
	else if (c == ' ') {
		c = 0; // space
	}
	else if (c == '#'){
		c=37;
	}
	else if (c=='/'){
		c=37;
	}

	switch(font_size)
	{
	case 51:  // font size 3x5  ascii value of 3: 51

		if(c==':'){
			matrix.drawPixel(x+1,y+1,color);
			matrix.drawPixel(x+1,y+3,color);
		}
		else if(c=='-'){
			matrix.drawLine(x,y+2,3,0,color);
		}
		else if(c=='.'){
			matrix.drawPixel(x+1,y+2,color);
		}
		else if(c==39 || c==44){
			matrix.drawLine(x+1,y,2,0,color);
			matrix.drawPixel(x+2,y+1,color);
		}
		else{
			for (uint8_t row=0; row< 5; row++) {
				dots = font3x5[(uint8_t)c][row];
				for (uint8_t col=0; col < 3; col++) {
					int x1=x;
					int y1=y;
					if (dots & (4>>col))
					matrix.drawPixel(x1+col, y1+row, color);
				}    
			}
		}
		break;

	case 53:  // font size 5x5   ascii value of 5: 53

		if(c==':'){
			matrix.drawPixel(x+2,y+1,color);
			matrix.drawPixel(x+2,y+3,color);
		}
		else if(c=='-'){
			matrix.drawLine(x+1,y+2,3,0,color);
		}
		else if(c=='.'){
			matrix.drawPixel(x+2,y+2,color);
		}
		else if(c==39 || c==44){
			matrix.drawLine(x+2,y,2,0,color);
			matrix.drawPixel(x+4,y+1,color);
		}
		else{
			for (uint8_t row=0; row< 5; row++) {
				dots = font5x5[(uint8_t)c][row];
				for (uint8_t col=0; col < 5; col++) {
					int x1=x;
					int y1=y;
					if (dots & (64>>col))  // For some wierd reason I have the 5x5 font in such a way that.. last two bits are zero.. 
					matrix.drawPixel(x1+col, y1+row, color);        
				}
			}
		}          

		break;
	default:
		break;
	}
}


//Spectrum Analyser stuff
void spectrumDisplay(){
#if defined (useFFT)

	uint8_t static i = 0;
	//static unsigned long tt = 0;
	int16_t val;

	uint8_t  c;
	uint16_t x,minLvl, maxLvl;
	int      level, y, off;

	DEBUGpln("in Spectrum");

	off = 0;

	cls();
	//for (int show = 0; show < SHOWCLOCK ; show++) {
	int showTime = Time.now();
	
	while((Time.now() - showTime) < showClock) {
		if (mode_changed == 1)
		return;	
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			spectrumDisplay();
			return;
		}

		if (i < 128){
			val = map(analogRead(MIC),0,4095,0,1023);
			fftdata[i] = (val / 4) - 128;
			im[i] = 0;
			i++;   
		}
		else {
			//this could be done with the fix_fftr function without the im array.
			fix_fft(fftdata,im,7,0);
			//fix_fftr(fftdata,7,0);
			
			// I am only interessted in the absolute value of the transformation
			for (i=0; i< 64;i++){
				fftdata[i] = sqrt(fftdata[i] * fftdata[i] + im[i] * im[i]); 
				//ftdata[i] = sqrt(fftdata[i] * fftdata[i] + fftdata[i+64] * fftdata[i+64]); 
			}

			for (i=0; i< 32;i++){
				spectrum[i] = fftdata[i*2] + fftdata[i*2 + 1];   // average together 
			}

			for(int l=0; l<16;l++){
				int col = matrix.Color444(16-l,0,l);
				matrix.drawLine(0,l,31,l,col);
			}

			// Downsample spectrum output to 32 columns:
			for(x=0; x<32; x++) {
				col[x][colCount] = spectrum[x];
				
				minLvl = maxLvl = col[x][0];
				int colsum=col[x][0];
				for(i=1; i<10; i++) { // Get range of prior 10 frames
					if(i<10)colsum = colsum + col[x][i];
					if(col[x][i] < minLvl)      minLvl = col[x][i];
					else if(col[x][i] > maxLvl) maxLvl = col[x][i];
				}
				// minLvl and maxLvl indicate the extents of the FFT output, used
				// for vertically scaling the output graph (so it looks interesting
				// regardless of volume level).  If they're too close together though
				// (e.g. at very low volume levels) the graph becomes super coarse
				// and 'jumpy'...so keep some minimum distance between them (this
				// also lets the graph go to zero when no sound is playing):
				if((maxLvl - minLvl) < 16) maxLvl = minLvl + 8;
				minLvlAvg[x] = (minLvlAvg[x] * 7 + minLvl) >> 3; // Dampen min/max levels
				maxLvlAvg[x] = (maxLvlAvg[x] * 7 + maxLvl) >> 3; // (fake rolling average)

				level = col[x][colCount];
				// Clip output and convert to byte:
				if(level < 0L)      c = 0;
				else if(level > 18) c = 18; // Allow dot to go a couple pixels off top
				else                c = (uint8_t)level;

				if(c > peak[x]) peak[x] = c; // Keep dot on top

				if(peak[x] <= 0) { // Empty column?
					matrix.drawLine(x, 0, x, 15, off);
					continue;
				}
				else if(c < 15) { // Partial column?
					matrix.drawLine(x, 0, x, 15 - c, off);
				}

				// The 'peak' dot color varies, but doesn't necessarily match
				// the three screen regions...yellow has a little extra influence.
				y = 16 - peak[x];
				matrix.drawPixel(x,y,matrix.Color444(peak[x],0,16-peak[x]));
			}
			i=0;
		}

		int mins = Time.minute();
		int hours = Time.hour();

		char buffer[3];

		itoa(hours,buffer,10);
		//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
		if (hours < 10) {
			buffer[1] = buffer[0];
			buffer[0] = '0';
		}
		vectorNumber(buffer[0]-'0',8,1,matrix.Color333(0,1,0),1,1);
		vectorNumber(buffer[1]-'0',12,1,matrix.Color333(0,1,0),1,1);

		itoa(mins,buffer,10);
		//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
		if (mins < 10) {
			buffer[1] = buffer[0];
			buffer[0] = '0';
		}
		vectorNumber(buffer[0]-'0',18,1,matrix.Color333(0,1,0),1,1);
		vectorNumber(buffer[1]-'0',22,1,matrix.Color333(0,1,0),1,1);

		matrix.drawPixel(16,2,matrix.Color333(0,1,0));
		matrix.drawPixel(16,4,matrix.Color333(0,1,0));

		matrix.swapBuffers(true);
		//delay(10);


		// Every third frame, make the peak pixels drop by 1:
		if(++dotCount >= 3) {
			dotCount = 0;
			for(x=0; x<32; x++) {
				if(peak[x] > 0) peak[x]--;
			}
		}

		if(++colCount >= 10) colCount = 0;
		
		Spark.process();	//Give the background process some lovin'
	}

#endif
}


void plasma()
{
	int           x1, x2, x3, x4, y1, y2, y3, y4, sx1, sx2, sx3, sx4;
	unsigned char x, y;
	long          value;
	unsigned long slowFrameRate = millis();
	
	cls();
	
	//for (int show = 0; show < SHOWCLOCK ; show++) {
	int showTime = Time.now();
	
	while((Time.now() - showTime) < showClock) {		
		if (mode_changed == 1)
		return;	
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			spectrumDisplay();
			return;
		}
		
		if (millis() - slowFrameRate >= 150) {
			
			sx1 = (int)(cos(angle1) * radius1 + centerx1);
			sx2 = (int)(cos(angle2) * radius2 + centerx2);
			sx3 = (int)(cos(angle3) * radius3 + centerx3);
			sx4 = (int)(cos(angle4) * radius4 + centerx4);
			y1  = (int)(sin(angle1) * radius1 + centery1);
			y2  = (int)(sin(angle2) * radius2 + centery2);
			y3  = (int)(sin(angle3) * radius3 + centery3);
			y4  = (int)(sin(angle4) * radius4 + centery4);

			for(y=0; y<(matrix.height()); y++) {
				x1 = sx1; x2 = sx2; x3 = sx3; x4 = sx4;
				for(x=0; x<matrix.width(); x++) {
					value = hueShift
					+ (int8_t)pgm_read_byte(sinetab + (uint8_t)((x1 * x1 + y1 * y1) >> 4))
					+ (int8_t)pgm_read_byte(sinetab + (uint8_t)((x2 * x2 + y2 * y2) >> 4))
					+ (int8_t)pgm_read_byte(sinetab + (uint8_t)((x3 * x3 + y3 * y3) >> 5))
					+ (int8_t)pgm_read_byte(sinetab + (uint8_t)((x4 * x4 + y4 * y4) >> 5));
					matrix.drawPixel(x, y, matrix.ColorHSV(value * 3, 255, 255, true));
					x1--; x2--; x3--; x4--;
				}
				y1--; y2--; y3--; y4--;
			}

			angle1 += 0.03;
			angle2 -= 0.07;
			angle3 += 0.13;
			angle4 -= 0.15;
			hueShift += 2;

			matrix.fillRect(7, 0, 19, 7, 0);
			
			int mins = Time.minute();
			int hours = Time.hour();
			char buffer[3];

			itoa(hours,buffer,10);
			//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
			if (hours < 10) {
				buffer[1] = buffer[0];
				buffer[0] = '0';
			}
			vectorNumber(buffer[0]-'0',8,1,matrix.Color333(229,0,0),1,1);
			vectorNumber(buffer[1]-'0',12,1,matrix.Color333(229,0,0),1,1);

			itoa(mins,buffer,10);
			//fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ". 
			if (mins < 10) {
				buffer[1] = buffer[0];
				buffer[0] = '0';
			}
			vectorNumber(buffer[0]-'0',18,1,matrix.Color333(229,0,0),1,1);
			vectorNumber(buffer[1]-'0',22,1,matrix.Color333(229,0,0),1,1);

			matrix.drawPixel(16,2,matrix.Color333(229,0,0));
			matrix.drawPixel(16,4,matrix.Color333(229,0,0));
			
			matrix.swapBuffers(false);
			
			slowFrameRate = millis();
		}
		Spark.process();	//Give the background process some lovin'
		delay(30);
	}
}

void marquee()
{
	char topLine[40] = {""};
	char botmLine[40] = "BOTTOM LINE GOES HERE";
	String tFull;
	
	//for (int show = 0; show < SHOWCLOCK ; show++) {
	int showTime = Time.now();
	
	while((Time.now() - showTime) < showClock) {

		if (mode_changed == 1)
			return;
		if(mode_quick){
			mode_quick = false;
			display_date();
			quickWeather();
			marquee();
			return;
		}
		
		tFull = Time.timeStr();
		tFull.toUpperCase();
		tFull.toCharArray(topLine, tFull.length()+1);
		tFull = "";

		//scrollBigMessage(topLine);
		scrollMessage(topLine, botmLine, 53, 53, Green, Navy);
		
		delay(50);
		
		Spark.process();
	}
}










