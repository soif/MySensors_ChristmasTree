/*
	MySensors ChristmasTree
	Copyright 2016 François Déchery

	Compilation:
		- needs MySensors version 2.0
		- works with Arduino IDE 1.6.9, 
		- Fails with Arduino IDE 1.6.10 and 1.6.12
*/

// debug #################################################################################
#define MY_DEBUG	// Comment/uncomment to remove/show debug (May overflow ProMini memory when set)


// Define ################################################################################
#define INFO_NAME "ChristmasTree"
#define INFO_VERS "1.50"

#define MY_RADIO_NRF24
#define MY_NODE_ID 21
//#define MY_REPEATER_FEATURE

#define NUM_LEDS			50 		// FASTLED : How many leds in the strip?
#define HUE_DEF_DELTA		2		// Default Hue Delta

#define POT_READ_COUNT 		100		// potentiometer reading count
#define POT_READ_PERC		2.0		// new POT read
#define POT_DEBOUNCE 		100		// potentiometer debounce time

#define S_ATIME_MIN			1		// Strip Anim Minimum ON time (ms) 
#define S_ATIME_MAX			200		// Strip Anim Maximum ON time (ms) 
#define S_ATIME_OFF 		1		// Strip Anim OFF time (ms) 

#define R_ATIME_MIN			40		// Relay Anim Minimum ON time (ms) 
#define R_ATIME_MAX			100		// Relay Anim Maximum ON time (ms) 
#define R_ATIME_OFF 		1		// Relay Anim OFF time (ms) 

#define SPEED_STEP			5		// Speed scale quantize

#define STRIP_SPEED_DEF		40		// Default Strip Speed (0-100)
#define RELAY_SPEED_DEF		40		// Default Strip Speed (0-100)

#define BUT_DEBOUNCE_TIME	100		// Button Debounce Time
#define BUT_HOLD_TIME 		1500	// Time to hold button before activating the Potentiometer (> DEBOUNCE_TIME)

#define MODE_OFF 			0		// mode Light Off
#define MODE_ON 			1		// mode Light On
#define MODE_ANIM 			2		// mode Anim

#define LAST_STRIP_MODE 	2		// last Strip mode
#define LAST_RELAY_MODE 	2		// last Relay mode

#define BLINK_PATTERN_LENGTH	3	// Status led: Blink Pattern Length
#define BLINK_PATTERN_DIVISOR	3	// Status led: Blink Pattern Divisor
#define RELAY_PATTERN_LENGTH	12	// Relay Pattern bit length

#define CHILD_ID_STRIP		0
#define CHILD_ID_STRIP_ANIM	1
#define CHILD_ID_RELAY		2
#define CHILD_ID_RELAY_ANIM	3


// includes ##############################################################################
#include <SPI.h>
#include <MySensors.h>

#include "debug.h"
#include <FastLED.h>	// https://github.com/FastLED/FastLED
#include <Button.h>		// https://github.com/JChristensen/Button
#include <SyncLED.h>	// https://github.com/martin-podlubny/arduino-library-syncled/

#include <SCoop.h>  	// https://github.com/fabriceo/SCoop
#if defined(SCoopANDROIDMODE) && (SCoopANDROIDMODE == 1)
#else
#error " --> You must set de parameter SCoopANDROIDMODE to 1 at the begining of "SCoop.h"
#endif

// Pins ##################################################################################
#define DATA_PIN		2		// FASTLED : Data Pin
#define BUT_STRIP_PIN	8		// Anim Switch Pin (to gnd)
#define BUT_RELAY_PIN	7		// Relay Switch Pin (to gnd)

#define RELAY_PIN		4		// Relay Pin
#define STRIP_LED_PIN 	5		// Strip Led Pin
#define RELAY_LED_PIN	6		// Relay Led Pin

#define POT_PIN 		A0		// Potentiomenter Pin

// Variables #############################################################################
float			pot_read 			= 0;
byte			pot_readings[POT_READ_COUNT];			// all potentiometer reading
byte			pot_read_index 		= 0;				// the index of the current potentiometer reading
unsigned long 	pot_read_total 		= 0; 				// total potentiometer reading     
unsigned long	pot_last_update 	= millis() +1000;	// potentiometer update time (start delayed)
boolean			pot_ready 			= false;

uint8_t 		gHue 				= 0;
uint8_t			gHueDelta			= HUE_DEF_DELTA;

byte			strip_mode			= MODE_ANIM;	//strip starting state mode (0=off, 1=on, 2=anim)
boolean			strip_anim_on		= true;			//strip starting anim state
boolean			strip_anim_stopping	= 0;
byte			strip_speed			= STRIP_SPEED_DEF;
unsigned int	strip_time			= 0;

byte			relay_mode			= MODE_OFF;	//relay starting state mode (0=off, 1=on, 2=anim)
boolean			relay_anim_on		= false;			//relay starting anim state
byte			relay_speed			= RELAY_SPEED_DEF;
unsigned int	relay_time			= 0;

boolean			but_strip_held		= false;		//when strip button is held
boolean			but_relay_held		= false;		//when relay button is held

CRGB 			leds[NUM_LEDS];						// FASTLED : Define the array of leds
CRGB 			current_color		= CRGB::Red;	//starting color

Button ButStrip	(BUT_STRIP_PIN,	true,	true,	BUT_DEBOUNCE_TIME);
Button ButRelay	(BUT_RELAY_PIN,	true,	true,	BUT_DEBOUNCE_TIME);

MyMessage 		msgStrip	(	CHILD_ID_STRIP,			V_STATUS);
MyMessage 		msgStripAnim(	CHILD_ID_STRIP_ANIM,	V_PERCENTAGE);
MyMessage 		msgRelay	(	CHILD_ID_RELAY,			V_STATUS);
MyMessage 		msgRelayAnim(	CHILD_ID_RELAY_ANIM,	V_PERCENTAGE);

SyncLED StripLed(STRIP_LED_PIN);
SyncLED RelayLed(RELAY_LED_PIN);

SyncLED RelayOut(RELAY_PIN);


// #######################################################################################
// ## MAIN  ##############################################################################
// #######################################################################################

// --------------------------------------------------------------------
void before() { 
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("+iS");

    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);

	// Init -----------------------
	InitPot();

	// Setup Output Pins -----------------------
	pinMode(RELAY_PIN,		OUTPUT);
	pinMode(RELAY_LED_PIN,	OUTPUT);
	//pinMode(STRIP_LED_PIN,	OUTPUT);

	DEBUG_PRINTLN("+iE");
}

// --------------------------------------------------------------------
void setup() { 
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("=sS");

	// init -------------------------
	InitSpeed();

	// leds
	StripLed.blinkPattern( 0B100UL, strip_time / BLINK_PATTERN_DIVISOR, BLINK_PATTERN_LENGTH );
	RelayLed.blinkPattern( 0B110000000000UL, relay_time, RELAY_PATTERN_LENGTH );
	// relay
	RelayOut.blinkPattern( 0B110000000000UL, relay_time, RELAY_PATTERN_LENGTH );

	// multi Tasking
	Scheduler.startLoop(SequenceStrip);
	Scheduler.startLoop(SequenceRelay);
	Scheduler.start();

	//let time to register
	Scheduler.delay(5000);
	//modes
	SetMode(strip_mode, true, 0);
	SetMode(relay_mode, true, 1);
	
	DEBUG_PRINTLN("=sE");
}


// --------------------------------------------------------------------
void loop() {
	UpdateButtonLeds();
	ProcessButtons();
	yield();
}



// FUNCTIONS #############################################################################

// --------------------------------------------------------------------
void presentation(){
	DEBUG_PRINTLN("_pS");
	sendSketchInfo(INFO_NAME , INFO_VERS );

	present(CHILD_ID_STRIP,			S_RGB_LIGHT);
	present(CHILD_ID_STRIP_ANIM,	S_DIMMER);
	present(CHILD_ID_RELAY, 		S_LIGHT);
	present(CHILD_ID_RELAY_ANIM,	S_DIMMER);

	DEBUG_PRINTLN("_pE");
}

// --------------------------------------------------------------------
void receive(const MyMessage &msg){
	boolean to_relay;
	DEBUG_PRINTLN("");

	// Strip On/Off
	if		(msg.sensor==CHILD_ID_STRIP && msg.type == V_STATUS){
		int r_state = msg.getBool();
		to_relay = 0;
		SetMode(r_state, false, to_relay);
	}
	// Strip On/Off RVB
	else if (msg.sensor==CHILD_ID_STRIP && msg.type == V_RGB){
		String color_string = msg.getString();
		DEBUG_PRINT(" -> Set Color: ");
		DEBUG_PRINTLN(color_string);
		current_color = (long) strtol( &color_string[0], NULL, 16);
		SetStripPixels(current_color);
		to_relay = 0;
		SetMode(MODE_ON , false, to_relay);
	}
	// Strip Anim On/Off
	else if (msg.sensor==CHILD_ID_STRIP_ANIM && msg.type == V_STATUS){
		boolean r_state = msg.getBool();
		to_relay = 0;
		if(r_state){
			SetMode(MODE_ANIM, false, to_relay);
		}
		else{
			SetMode(MODE_OFF, false, to_relay);
		}
	}
	// Strip Anim Speed
	else if (msg.sensor==CHILD_ID_STRIP_ANIM && msg.type == V_PERCENTAGE){
		unsigned int r_speed= msg.getUInt();
		to_relay = 0;
		SetAnimSpeed(r_speed, false, to_relay);
		SetMode(MODE_ANIM , false, to_relay);
	}

	// Relay On/Off
	else if (msg.sensor==CHILD_ID_RELAY){
		int r_state = msg.getBool();
		to_relay = 1;
		SetMode(r_state, false, to_relay);
	}
	// Relay Anim On/Off
	else if (msg.sensor==CHILD_ID_RELAY_ANIM && msg.type == V_STATUS){
		boolean r_state = msg.getBool();
		to_relay = 1;
		if(r_state){
			SetMode(MODE_ANIM, false, to_relay);
		}
		else{
			SetMode(MODE_OFF, false, to_relay);
		}
	}
	// Relay Anim Speed
	else if (msg.sensor==CHILD_ID_RELAY_ANIM && msg.type == V_PERCENTAGE){
		unsigned int r_speed= msg.getUInt();
		to_relay = 1;
		SetAnimSpeed(r_speed, false, to_relay);
		SetMode(MODE_ANIM , false, to_relay);
	}

	else{
		DEBUG_PRINTLN("# Msg IGNORE");
	}
}

// --------------------------------------------------------------------
void SendOnOffStatus(unsigned int state, boolean to_relay){
	switch (to_relay){
    	case 0:
			msgStrip.setType(V_STATUS);
			send(msgStrip.set(state));
			break;
    	case 1:
			msgRelay.setType(V_STATUS);
			send(msgRelay.set(state));
			break;
	}
}

// --------------------------------------------------------------------
void SendStripColor(){
}

// --------------------------------------------------------------------
void SendAnimStatus(unsigned int state, boolean to_relay){
	switch (to_relay){
    	case 0:
			msgStripAnim.setType(V_STATUS);
			send(msgStripAnim.set(state));
			break;
    	case 1:
			msgRelayAnim.setType(V_STATUS);
			send(msgRelayAnim.set(state));    		
			break;
	}
}

// --------------------------------------------------------------------
void SendAnimSpeed(unsigned int speed, boolean to_relay){
	switch (to_relay){
    	case 0:
			msgStripAnim.setType(V_PERCENTAGE);
			send(msgStripAnim.set(speed));
			break;
    	case 1:
			msgRelayAnim.setType(V_PERCENTAGE);
			send(msgRelayAnim.set(speed));    		
			break;
	}
}

// --------------------------------------------------------------------
void UpdateButtonLeds(){
	if (strip_mode == MODE_OFF){
		StripLed.Off();
	}
	else if (strip_mode == MODE_ON){
		StripLed.On();
	}
	else if (strip_mode == MODE_ANIM){
		StripLed.resumePattern();
		StripLed.update();
	}

	if (relay_mode == MODE_OFF){
		RelayLed.Off();
	}
	else if (relay_mode == MODE_ON){
		RelayLed.On();
	}
	else if (relay_mode == MODE_ANIM){
		RelayLed.resumePattern();
		RelayLed.update();
		RelayOut.resumePattern();
		RelayOut.update();
	}
}

// --------------------------------------------------------------------
void InitSpeed(){
	SetAnimSpeed(200,			false,	0);
	SetAnimSpeed(strip_speed,	true,	0);

	SetAnimSpeed(200,			false,	1);
	SetAnimSpeed(relay_speed,	true,	1);
}


// --------------------------------------------------------------------
void ReadSpeedPot(boolean to_relay){
	// remove prev read
	pot_read_total				= pot_read_total - pot_readings[pot_read_index];
	
	// low pass filter
	pot_read					= (1 - POT_READ_PERC / 100) * pot_read + ( POT_READ_PERC / 100) * analogRead(POT_PIN);
	
	// map to byte
	pot_readings[pot_read_index]= map( pot_read,  0, 1023, 1, 255);
	
	// add this read
	pot_read_total				= pot_read_total + pot_readings[pot_read_index];

	pot_read_index				= pot_read_index + 1;
	if (pot_read_index >= POT_READ_COUNT) {
		pot_read_index = 0;
		pot_ready = true;
	}

	// average read
	unsigned int pot_aver	= pot_read_total / POT_READ_COUNT;
	
	// map to desired speed 
	unsigned int speed	= map( pot_aver , 0, 255 , 0 , 100);

	unsigned int last_speed	;
	switch (to_relay){
		case 0:	
    		last_speed=strip_speed;
			break;
		case 1:	
			last_speed=relay_speed;
			break;
	}
	
	if( pot_ready && speed != last_speed && ( speed <= last_speed - SPEED_STEP || speed >= last_speed + SPEED_STEP || speed == 0 || speed == 100 ) ){

		if(millis() > pot_last_update + POT_DEBOUNCE ){
			pot_last_update = millis();

			//quantize to SPEED_STEP
			speed = round( speed / SPEED_STEP * SPEED_STEP );

			DEBUG_PRINTLN(".");
			DEBUG_PRINT("+ Pav=");
			DEBUG_PRINT(pot_aver);
			DEBUG_PRINT(", Speed=");
			DEBUG_PRINT( speed );
			DEBUG_PRINTLN("");
			
			SetAnimSpeed(speed, true, to_relay);
		}
	}
}

// --------------------------------------------------------------------
void InitPot(){
	for (int thisReading = 0; thisReading < POT_READ_COUNT; thisReading++) {
		pot_readings[thisReading] = 0;
	}
}

// --------------------------------------------------------------------
void ProcessButtons(){
	boolean to_relay;

	// Strip Button
	ButStrip.read();
	to_relay=0;
	if (ButStrip.wasReleased()) {
		if(but_strip_held){
			but_strip_held = false;
		}
		else{
			SetMode (strip_mode + 1, true, to_relay);
		}
	}	
	else if(ButStrip.pressedFor(BUT_HOLD_TIME)){
		but_strip_held = true;
		//DEBUG_PRINT("sH.");
		ReadSpeedPot(to_relay);
	}	

	// Relay Button
	ButRelay.read();
	to_relay=1;
	if (ButRelay.wasReleased()) {
		if(but_relay_held){
			but_relay_held = false;
		}
		else{
			SetMode (relay_mode + 1, true, to_relay);
		}
	}	
	else if(ButRelay.pressedFor(BUT_HOLD_TIME)){
		but_relay_held = true;
		//DEBUG_PRINT("rH.");	
		ReadSpeedPot(to_relay);
	}
}

// --------------------------------------------------------------------
void SetAnimSpeed(unsigned int speed, boolean do_send_msg, boolean to_relay){
	switch (to_relay){
    	case 0:
			if( speed != strip_speed ){
				strip_speed = speed ;
				strip_time = map( strip_speed, 0, 100, S_ATIME_MIN, S_ATIME_MAX) ;		
				StripLed.setRate( strip_time / BLINK_PATTERN_DIVISOR );

				DEBUG_PRINT(" -> S.Speed=");
				DEBUG_PRINT(strip_speed);
				DEBUG_PRINT(" , time=");
				DEBUG_PRINTLN(strip_time);
				if(do_send_msg){
					SendAnimSpeed(strip_speed, to_relay);
				}
			}
			break;

    	case 1:
			if( speed != relay_speed ){
				relay_speed = speed ;
				relay_time = map( relay_speed, 0, 100, S_ATIME_MIN, S_ATIME_MAX) ;		
				RelayLed.setRate( relay_time );
				RelayOut.setRate( relay_time );

				DEBUG_PRINT(" -> R.Speed=");
				DEBUG_PRINT(relay_speed);
				DEBUG_PRINT(" , time=");
				DEBUG_PRINTLN(relay_time);
				if(do_send_msg){
					SendAnimSpeed(relay_speed, to_relay);
				}
			}
			break;
	}
}

// --------------------------------------------------------------------
void SetMode(byte mode, boolean do_send_msg, boolean to_relay){
	
	DEBUG_PRINT(" -> Set ");	
	byte last_mode;

	switch (to_relay){

    	case 0:
			//rotate mode
			if(mode > LAST_STRIP_MODE){
				mode=MODE_OFF;
			}
    		last_mode=strip_mode;
			DEBUG_PRINT("Strip Mode: ");
			//DEBUG_PRINT(mode);	

			if(mode == MODE_OFF){
				DEBUG_PRINTLN("OFF");	
				strip_anim_on =false;

				if (last_mode == MODE_ON){
					Pixels_Off();

					if(do_send_msg){
						SendOnOffStatus(false, to_relay);
						//SendAnimStatus(false, to_relay);
					}
				}
				else if (last_mode == MODE_ANIM){
			
					if(! strip_anim_stopping){
						strip_anim_stopping =1;
						Scheduler.delay(S_ATIME_MAX + S_ATIME_OFF + 50);
						Pixels_Off();
						strip_anim_stopping =0;
					}

					if(do_send_msg){
						SendOnOffStatus(false, to_relay);
						SendAnimStatus(false, to_relay);
					}			
				}
			}

			else if (mode == MODE_ON){
				DEBUG_PRINTLN("ON");	
				strip_anim_on =false;

				if(last_mode == MODE_OFF){
					SetStripPixels(current_color);

					if(do_send_msg){
						SendOnOffStatus(true, to_relay);
						//SendAnimStatus(false, to_relay);
					}
				}	
				else if (last_mode == MODE_ANIM){
					if(do_send_msg){
						SendOnOffStatus(true, to_relay);
						SendAnimStatus(false, to_relay);
					}
				}
			}
			
			else if (mode == MODE_ANIM){
				DEBUG_PRINTLN("ANIM");	
				strip_anim_on = true;

				if(last_mode == MODE_OFF){
					Pixels_Off();

					if(do_send_msg){
						//SendOnOffStatus(false, to_relay);
						SendAnimStatus(true, to_relay);
					}
				}
				else if (last_mode == MODE_ON){
					Pixels_Off();

					if(do_send_msg){
						SendOnOffStatus(false, to_relay);
						//SendAnimStatus(true, to_relay);
					}
				}
			}
			strip_mode = mode;
			break;

		// -----------------
    	case 1:
			//rotate mode
			if(mode > LAST_RELAY_MODE){
				mode=MODE_OFF;
			}

    		last_mode=relay_mode;    		
			DEBUG_PRINT("Relay Mode: ");	
			//DEBUG_PRINT(mode);	

			if(mode == MODE_OFF){
				DEBUG_PRINTLN("OFF");	
				relay_anim_on =false;

				if (last_mode == MODE_ON){

					if(do_send_msg){
						SendOnOffStatus(false, to_relay);
						//SendAnimStatus(false, to_relay);
					}
				}
				else if (last_mode == MODE_ANIM){
			
					if(do_send_msg){
						SendOnOffStatus(false, to_relay);
						SendAnimStatus(false, to_relay);
					}			
				}
			}

			else if (mode == MODE_ON){
				DEBUG_PRINTLN("ON");	
				relay_anim_on =false;

				if(last_mode == MODE_OFF){

					if(do_send_msg){
						SendOnOffStatus(true, to_relay);
						//SendAnimStatus(false, to_relay);
					}
				}	
				else if (last_mode == MODE_ANIM){

					if(do_send_msg){
						SendOnOffStatus(true, to_relay);
						SendAnimStatus(false, to_relay);
					}
				}
			}
			else if (mode == MODE_ANIM){
				DEBUG_PRINTLN("ANIM");	
				relay_anim_on = true;

				if(last_mode == MODE_OFF){

					if(do_send_msg){
						//SendOnOffStatus(false, to_relay);
						SendAnimStatus(true, to_relay);
					}
				}
				else if (last_mode == MODE_ON){

					if(do_send_msg){
						SendOnOffStatus(false, to_relay);
						//SendAnimStatus(true, to_relay);
					}
				}
			}
			relay_mode = mode;
			break;
	}
}
/*
// --------------------------------------------------------------------
void SwitchRelay(boolean state){
	if(state){
		DEBUG_PRINTLN(" -> Relay ON");
		digitalWrite(RELAY_PIN,		HIGH);
		digitalWrite(RELAY_LED_PIN,	HIGH);
	}
	else{
		DEBUG_PRINTLN(" -> Relay OFF");
		digitalWrite(RELAY_PIN,		LOW);
		digitalWrite(RELAY_LED_PIN,	LOW);
	}
}
*/
// --------------------------------------------------------------------
void SequenceStrip(){
	if(!strip_anim_on){return;}
	Pixels_Up(0, 1,0);		// red, hold

	StripAnim_Up_Drift(3, 0);	// x3 

	StripAnim_Random(100,0);		// x120

	Pixels_Up(96, 1,0); 	// green, hold
	Pixels_Up(gHue, 0,1);	// off, drift

	StripAnim_Random(40,1);		// x40, hold
	
	StripAnim_Down_Drift(2,0);	//x2

	Pixels_Down(160, 1,0); 	// blue, hold
	Pixels_Down(gHue, 0,1);	// off, drift

	StripAnim_UpDown_Drift(1,1);	// x2, hold

	StripAnim_Blink_Drift(6);	//x6
	StripAnim_Blink_Drift(6);	//x6

	StripAnim_Rainbow(6);		//x10

	Pixels_Up(gHue, 0,1);	// off, drift
}

// --------------------------------------------------------------------
void SequenceRelay(){
	RelayPattern(0B111100111100UL, 4);
	RelayPattern(0B111011101110UL, 2);
	RelayPattern(0B000011000011UL, 4);
	RelayPattern(0B101010101010UL, 1);
}

// --------------------------------------------------------------------
void RelayPattern(unsigned long pattern, byte count){
	if(!relay_anim_on){
		return;
	}
	RelayLed.setPattern(pattern, RELAY_PATTERN_LENGTH);
	RelayOut.setPattern(pattern, RELAY_PATTERN_LENGTH);
	Scheduler.delay(ceil(relay_time * RELAY_PATTERN_LENGTH * count) );
}

// --------------------------------------------------------------------
void SetStripPixels( CRGB color ){
	for(int i=0;i< NUM_LEDS; i++){
		leds[i] = color;
	}
	FastLED.show();
}

// --------------------------------------------------------------------
void StripAnim_Up_Drift(int count, boolean hold){
	for(int i=1;i <= count; i++){
		Pixels_Up(gHue, hold,1);
	}
}

// --------------------------------------------------------------------
void StripAnim_Down_Drift(int count, boolean hold){
	for(int i=1;i <= count; i++){
		Pixels_Down(gHue, hold,1);
	}
}

// --------------------------------------------------------------------
void StripAnim_UpDown_Drift(int count, boolean hold){
	for(int i=1;i <= count; i++){
		StripAnim_UpDown(gHue, hold,1);
	}
}

// --------------------------------------------------------------------
void StripAnim_Random(int count, boolean hold){
	for(int i=1;i <= count ; i++){
		Pixels_Random(0,hold);
	}
}

// --------------------------------------------------------------------
void StripAnim_Blink_Drift(int count_blinks){
	gHueDelta = 255 / count_blinks ;
	for(int i=1;i <= count_blinks; i++){
		Pixels_Blink_One(gHue, 0,1);
	}
	gHueDelta = HUE_DEF_DELTA;
}

// --------------------------------------------------------------------
void StripAnim_UpDown(uint8_t hue, boolean hold, boolean drift){
	Pixels_Up	(hue, hold, drift);
	Pixels_Down	(hue, hold, drift);
}

// --------------------------------------------------------------------
void StripAnim_Rainbow(unsigned int count){
	unsigned long hue=0;
	const byte step= ceil(255 / NUM_LEDS );
	for(int i=0; i< (count * 255 / step) ; i++){
		if(!strip_anim_on){return;}

		if(hue > 4294967295){
			hue=0;
		}
		Pixels_Rainbow(hue);
		Scheduler.delay(ceil(strip_time / 8) );			
		hue = hue + step;	
	}
}

// --------------------------------------------------------------------
void Pixels_Blink_One(uint8_t hue, boolean hold, boolean drift){
	for(int i=0;i< NUM_LEDS; i++){
		if(!strip_anim_on){return;}
		leds[i] = CHSV(hue, 255, 255);
	}
	FastLED.show();
	Scheduler.delay(strip_time * 2);
	
	//off --------
	if(! hold){
		Pixels_Off();
	}
	Scheduler.delay(strip_time);

	//drift color -------
	if(drift){
		hue += gHueDelta;
		gHue=hue;
	}
}

// --------------------------------------------------------------------
void Pixels_Random(uint8_t hue, boolean hold){
	int i =random(0, NUM_LEDS );
	if(hue == 0){
		hue=random(0, 255 );
	}

	if(!strip_anim_on){return;}

	// on --------------------------
	leds[i] = CHSV(hue, 255, 255);
	FastLED.show();
	Scheduler.delay(strip_time);

	//off --------
	if(! hold){
		leds[i] = CRGB::Black;
		FastLED.show();
		Scheduler.delay(S_ATIME_OFF);
	}
}

// --------------------------------------------------------------------
void Pixels_Up(uint8_t hue, boolean hold, boolean drift){

	for(int i=0; i< NUM_LEDS; i++){
		if(!strip_anim_on){return;}

		// on --------------------------
		leds[i] = CHSV(hue, 255, 255);
		FastLED.show();
		Scheduler.delay(strip_time);

		//off --------
		if(! hold){
			leds[i] = CRGB::Black;
			FastLED.show();
			Scheduler.delay(S_ATIME_OFF);
		}

		//drift color -------
		if(drift){
			hue += gHueDelta;
			gHue=hue;
		}
	}
}

// --------------------------------------------------------------------
void Pixels_Down(uint8_t hue, boolean hold, boolean drift){

	for(int i=NUM_LEDS - 1; i >=0; i-- ){
		if(!strip_anim_on){return;}
		
		// on --------------------------
		leds[i] = CHSV(hue, 255, 255);
		FastLED.show();
		Scheduler.delay(strip_time);

		//off --------
		if(! hold){
			leds[i] = CRGB::Black;
			FastLED.show();
			Scheduler.delay(S_ATIME_OFF);
		}

		//drift color -------
		if(drift){
			hue += gHueDelta;
			gHue=hue;
		}
	}
}

// --------------------------------------------------------------------
void Pixels_Rainbow(uint8_t hue){
	const byte step= ceil(255 / NUM_LEDS );
	for(int i=0; i< NUM_LEDS; i++){
		leds[i] = CHSV(hue + step * i, 255, 255);
	}
	FastLED.show();
}

// --------------------------------------------------------------------
void Pixels_Off(){
	SetStripPixels(CRGB::Black);
}
