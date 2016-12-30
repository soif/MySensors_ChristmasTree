/*
	MySensors ChristmasTree - Version 1.0
	Copyright 2016 François Déchery

	https://media.readthedocs.org/pdf/mysensors/latest/mysensors.pdf

	Compilation:
		- needs MySensors version 2.0
		- works with Arduino IDE 1.6.9, 
		- Fails with Arduino IDE 1.6.10 and 1.6.12
*/

// debug #################################################################################
#define MY_DEBUG	// Comment/uncomment to remove/show debug (May overflow ProMini memory when set)


// Define ################################################################################
#define INFO_NAME "ChristmasTree"
#define INFO_VERS "1.0"

#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 21
//#define MY_REPEATER_FEATURE

#define NUM_LEDS		50 		// FASTLED : How many leds in the strip?
#define DEBOUNCE_TIME	100		// Button Debounce Time
#define HUE_DEF_DELTA	2		// Default Hue Delta
#define POT_READ_COUNT 100		// potentiometer reading count
#define POT_READ_PERC	2.0		// new POT read
#define POT_DEBOUNCE 	100		// potentiometer debounce time

#define ATIME_MIN		1		// Anim Minimum ON time (ms) 
#define ATIME_MAX		300		// Anim Maximum ON time (ms) 
#define SPEED_DEF		40		// Default Speed
#define SPEED_STEP		5		// Speed scale quantize
#define ATIME_OFF 		1		// Anim OFF time (ms) 
#define MODES_COUNT 	3		// light modes count

#define CHILD_ID_LIGHT	0
#define CHILD_ID_ANIM	1
#define CHILD_ID_RELAY	2


// includes ##############################################################################
#include <SPI.h>
#include <MySensors.h>

#include "debug.h"
#include <FastLED.h>	// https://github.com/FastLED/FastLED
#include <Bounce2.h>	// https://github.com/thomasfredericks/Bounce-Arduino-Wiring
#include <SyncLED.h>	// https://github.com/martin-podlubny/arduino-library-syncled/

#include <SCoop.h>  	// https://github.com/fabriceo/SCoop
#if defined(SCoopANDROIDMODE) && (SCoopANDROIDMODE == 1)
#else
#error " --> You must set de parameter SCoopANDROIDMODE to 1 at the begining of "SCoop.h"
#endif

// Pins ##################################################################################
#define DATA_PIN 3			// FASTLED : Data Pin
#define BUT_ANIM_PIN 4		// Anim Switch Pin (to gnd)
#define BUT_RELAY_PIN 5		// Relay Switch Pin (to gnd)

#define RELAY_PIN 6			// Relay Pin
#define RELAY_LED_PIN 8		// Relay Led Pin
#define ANIM_LED_PIN 7		// Anim  Led Pin

#define POT_PIN A0			// Potentiomenter Pin

// Variables #############################################################################
float			pot_read 		=0;
byte			pot_readings[POT_READ_COUNT];		// all potentiometer reading
byte			pot_read_index 	= 0;				// the index of the current potentiometer reading
unsigned long 	pot_read_total 	= 0; 				// total potentiometer reading     
unsigned long	pot_last_update = millis() +1000;	// potentiometer update time (start delayed)
boolean			pot_ready 		= false;
boolean			pot_enable 		= false; //starting state

byte			anim_speed		= SPEED_DEF;
unsigned int	anim_time		= 0;
uint8_t 		gHue 			= 0;
uint8_t			gHueDelta		= HUE_DEF_DELTA;
boolean			anim_sw_stopping=0;
byte			light_mode		=2;	//starting state (0=off, 1=on, 2=anim)
boolean			anim_is_on		=1;	//starting state
boolean			relay_is_on		=1;	//starting state

CRGB 			leds[NUM_LEDS];			// FASTLED : Define the array of leds
CRGB 			current_color	= CRGB::Red;

Bounce 			debounceButAnim		= Bounce(); 
Bounce 			debounceButRelay	= Bounce(); 

MyMessage 		msgLight(	CHILD_ID_LIGHT,	V_STATUS);
MyMessage 		msgAnim(	CHILD_ID_ANIM,	V_PERCENTAGE);
MyMessage 		msgRelay(	CHILD_ID_RELAY,	V_STATUS);
//MyMessage 		msgTemp(	CHILD_ID_TEMP,	V_TEMP);

SyncLED AnimLed(ANIM_LED_PIN);

// #######################################################################################
// ## MAIN  ##############################################################################
// #######################################################################################

// --------------------------------------------------------------------
void before() { 
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("+++ Init Start +++");

    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);

	// Init -----------------------
	InitButtons();
	InitPot();

	// Setup Output Pins -----------------------
	pinMode(RELAY_PIN,		OUTPUT);
	pinMode(RELAY_LED_PIN,	OUTPUT);
	//pinMode(ANIM_LED_PIN,	OUTPUT);

	// -------------------------------
	Scheduler.startLoop(leds_sequence);
	Scheduler.start();

	DEBUG_PRINTLN("+++ Init End +++");
}

// --------------------------------------------------------------------
void setup() { 
}

// --------------------------------------------------------------------
void loop() {
	UpdateAnimLed();
	ProcessButtons();
	ReadSpeedPot();
	yield();
}



// FUNCTIONS #############################################################################

// --------------------------------------------------------------------
void presentation(){
	sendSketchInfo(INFO_NAME , INFO_VERS );

	present(CHILD_ID_LIGHT,	S_RGB_LIGHT);
	present(CHILD_ID_ANIM,	S_DIMMER);
	present(CHILD_ID_RELAY, S_LIGHT);
	//present(CHILD_ID_TEMP,	S_TEMP);

	DEBUG_PRINTLN("+++ Setup +++");

	// init -------------------------
	InitSpeed();
	AnimLed.blinkPattern( 0B100UL, anim_time / 3, 3 );

	SetLightMode(light_mode, true);
	SwitchRelay(relay_is_on, true);
}


// --------------------------------------------------------------------
void receive(const MyMessage &msg){
	DEBUG_PRINTLN("");

	if		(msg.sensor==CHILD_ID_LIGHT && msg.type == V_STATUS){
		int r_state = msg.getBool();
		SetLightMode(r_state, true);
	}
	else if (msg.sensor==CHILD_ID_LIGHT && msg.type == V_RGB){
		String color_string = msg.getString();
		DEBUG_PRINT(" -> Set color : ");
		DEBUG_PRINTLN(color_string);
		current_color = (long) strtol( &color_string[0], NULL, 16);
		SetAllLeds(current_color);
		SetLightMode(1, true);
	}
	else if (msg.sensor==CHILD_ID_ANIM && msg.type == V_STATUS){
		boolean r_state = msg.getBool();
		if(r_state){
			SetLightMode(2, true);
		}
		else{
			SetLightMode(0, true);
		}
	}
	else if (msg.sensor==CHILD_ID_ANIM && msg.type == V_PERCENTAGE){
		unsigned int r_speed= msg.getUInt();
		SetAnimSpeed(r_speed, false);
		SetLightMode(2, true);

		pot_enable = false;
		DEBUG_PRINTLN("- Lock Pot.");
	}
	else if (msg.sensor==CHILD_ID_RELAY){
		int r_state = msg.getBool();
		SwitchRelay(r_state, false);
	}
	else{
		DEBUG_PRINTLN("# Msg IGNORED");
	}
}

// --------------------------------------------------------------------
void SendLightStatus(unsigned int state){
	msgLight.setType(V_STATUS);
	send(msgLight.set(state));
}
// --------------------------------------------------------------------
void SendLightColor(){
}
// --------------------------------------------------------------------
void SendAnimStatus(unsigned int state){
	msgAnim.setType(V_STATUS);
	send(msgAnim.set(state));
}
// --------------------------------------------------------------------
void SendAnimSpeed(unsigned int speed){
	msgAnim.setType(V_PERCENTAGE);
	send(msgAnim.set(speed));
}

// --------------------------------------------------------------------
void UpdateAnimLed(){
	if (light_mode ==0){
		AnimLed.Off();
	}
	else if (light_mode ==1){
		AnimLed.On();
	}
	else if (light_mode ==2){
		AnimLed.resumePattern();
		AnimLed.update();
	}
}

// --------------------------------------------------------------------
void InitSpeed(){
	SetAnimSpeed(200,false);
	SetAnimSpeed(anim_speed,true);
}


// --------------------------------------------------------------------
void ReadSpeedPot(){
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

	// wake up potentiometer control by turning it to min or max
	if(pot_ready && !pot_enable){
		if( (anim_speed > 0 && speed == 0) || (anim_speed < 100 && speed == 100)){
			pot_enable = true;
			DEBUG_PRINTLN("- Unlock Pot.");
		}
	}

	if(pot_enable && pot_ready && speed != anim_speed && ( speed <= anim_speed - SPEED_STEP || speed >= anim_speed + SPEED_STEP || speed == 0 || speed == 100 ) ){

		if(millis() > pot_last_update + POT_DEBOUNCE ){
			pot_last_update = millis();

			//quantize to SPEED_STEP
			speed = round( speed / SPEED_STEP * SPEED_STEP );

			DEBUG_PRINT("++ P_av=");
			DEBUG_PRINT(pot_aver);
			DEBUG_PRINT(", Q_Speed=");
			DEBUG_PRINT( speed );
			DEBUG_PRINTLN("");
			
			SetAnimSpeed(speed, true);
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
void InitButtons(){
	pinMode(BUT_ANIM_PIN,INPUT);
	digitalWrite(BUT_ANIM_PIN,HIGH);   // Activate internal pull-up
	debounceButAnim.attach(BUT_ANIM_PIN);
	debounceButAnim.interval(DEBOUNCE_TIME);

	pinMode(BUT_RELAY_PIN,INPUT);
	digitalWrite(BUT_RELAY_PIN,HIGH);   // Activate internal pull-up
	debounceButRelay.attach(BUT_RELAY_PIN);
	debounceButRelay.interval(DEBOUNCE_TIME);
}


// --------------------------------------------------------------------
void ProcessButtons(){
	boolean but_changed = debounceButRelay.update();
	int but_state 		= debounceButRelay.read();
	if (but_changed && but_state == 0) {
		SwitchRelay (! relay_is_on, true);
	}

	but_changed =debounceButAnim.update();
	but_state	=debounceButAnim.read();
	if (but_changed && but_state == 0) {
		SetLightMode (light_mode + 1, true);
	}
}


// --------------------------------------------------------------------
void SetAnimSpeed(unsigned int speed, boolean do_send_msg){
	if( speed != anim_speed ){
		anim_speed = speed ;
		anim_time = map(anim_speed, 0, 100, ATIME_MIN, ATIME_MAX) ;
		
		AnimLed.setRate( anim_time / 3 );

		if(do_send_msg){
			SendAnimSpeed(anim_speed);
		}
		DEBUG_PRINT(" -> Speed ");
		DEBUG_PRINT(anim_speed);
		DEBUG_PRINT(" to time ");
		DEBUG_PRINTLN(anim_time);
	}
}

// --------------------------------------------------------------------
void SetLightMode(byte mode, boolean do_send_msg){
	//rotate mode
	if(mode > MODES_COUNT){
		mode=0;
	}

	if(mode == 0){
		DEBUG_PRINTLN(" -> Light OFF");	
		anim_is_on =false;

		if (light_mode ==1){
			SetAllLeds(CRGB::Black);

			if(do_send_msg){
				SendLightStatus(false);
				//SendAnimStatus(false);
			}
		}
		else if (light_mode ==2){
			
			if(! anim_sw_stopping){
				anim_sw_stopping =1;
				Scheduler.delay(1000);
				SetAllLeds(CRGB::Black);
				anim_sw_stopping =0;
			}

			if(do_send_msg){
				SendLightStatus(false);
				SendAnimStatus(false);
			}			
		}
	}

	else if (mode == 1){
		DEBUG_PRINTLN(" -> Light ON");	
		anim_is_on =false;

		if(light_mode == 0){
			SetAllLeds(current_color);

			if(do_send_msg){
				SendLightStatus(true);
				//SendAnimStatus(false);
			}
		}	
		else if (light_mode == 2){
			if(do_send_msg){
				SendLightStatus(true);
				SendAnimStatus(false);
			}
		}
	}
	else if (mode == 2){
		DEBUG_PRINTLN(" -> Light ANIM");	
		anim_is_on = true;

		if(light_mode == 0){
			SetAllLeds(CRGB::Black);

			if(do_send_msg){
				//SendLightStatus(false);
				SendAnimStatus(true);
			}
		}
		else if (light_mode == 1){
			SetAllLeds(CRGB::Black);

			if(do_send_msg){
				SendLightStatus(false);
				//SendAnimStatus(true);
			}
		}
	}
	light_mode = mode;
}


// --------------------------------------------------------------------
void SwitchRelay(boolean state, boolean do_send_msg){
	relay_is_on = state;
	if(do_send_msg){
		send(msgRelay.set(relay_is_on));
	}
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


// --------------------------------------------------------------------
void leds_sequence(){
	if(!anim_is_on){return;}

	p_up(0, 1,0);		// red

	loop_up_drift(3, 0);

	loop_rand(120,0);

	p_up(96, 1,0); 		// green
	p_up(gHue, 0,1);	// off

	loop_rand(40,1);
	
	loop_down_drift(3,0);

	p_down(160, 1,0); 	// blue
	p_down(gHue, 0,1);	// off

	loop_updown_drift(3,1);

	loop_blink(6);
	loop_blink(6);

}

// --------------------------------------------------------------------
void SetAllLeds( CRGB color ){
	for(int i=0;i< NUM_LEDS; i++){
		leds[i] = color;
	}
	FastLED.show();
}


// --------------------------------------------------------------------
void loop_up_drift(int count, boolean hold){
	for(int i=1;i <= count; i++){
		p_up(gHue, hold,1);
	}
}


// --------------------------------------------------------------------
void loop_down_drift(int count, boolean hold){
	for(int i=1;i <= count; i++){
		p_down(gHue, hold,1);
	}
}


// --------------------------------------------------------------------
void loop_updown_drift(int count, boolean hold){
	for(int i=1;i <= count; i++){
		p_up_down(gHue, hold,1);
	}
}


// --------------------------------------------------------------------
void loop_rand(int count, boolean hold){
	for(int i=1;i <= count ; i++){
		p_rand(0,hold);
	}
}


// --------------------------------------------------------------------
void loop_blink(int count_blinks){
	gHueDelta = 255 / count_blinks ;
	for(int i=1;i <= count_blinks; i++){
		p_blink_one(gHue, 0,1);
	}
	gHueDelta = HUE_DEF_DELTA;
}


// --------------------------------------------------------------------
void p_up_down(uint8_t hue, boolean hold, boolean drift){
	p_up	(hue, hold, drift);
	p_down	(hue, hold, drift);
}


// --------------------------------------------------------------------
void p_blink_one(uint8_t hue, boolean hold, boolean drift){
	for(int i=0;i< NUM_LEDS; i++){
		if(!anim_is_on){return;}
		leds[i] = CHSV(hue, 255, 255);
	}
	FastLED.show();
	Scheduler.delay(anim_time * 2);
	
	//off --------
	if(! hold){
		for(int i=0;i< NUM_LEDS; i++){
			leds[i] = CRGB::Black;
		}
	}
	FastLED.show();
	Scheduler.delay(anim_time);

	//drift color -------
	if(drift){
		hue += gHueDelta;
		gHue=hue;
	}
}


// --------------------------------------------------------------------
void p_rand(uint8_t hue, boolean hold){
	int i =random(0, NUM_LEDS );
	if(hue == 0){
		hue=random(0, 255 );
	}

	if(!anim_is_on){return;}

	// on --------------------------
	leds[i] = CHSV(hue, 255, 255);
	FastLED.show();
	Scheduler.delay(anim_time);

	//off --------
	if(! hold){
		leds[i] = CRGB::Black;
		FastLED.show();
		Scheduler.delay(ATIME_OFF);
	}
}


// --------------------------------------------------------------------
void p_up(uint8_t hue, boolean hold, boolean drift){

	for(int i=0; i< NUM_LEDS; i++){
		if(!anim_is_on){return;}

		// on --------------------------
		leds[i] = CHSV(hue, 255, 255);
		FastLED.show();
		Scheduler.delay(anim_time);

		//off --------
		if(! hold){
			leds[i] = CRGB::Black;
			FastLED.show();
			Scheduler.delay(ATIME_OFF);
		}

		//drift color -------
		if(drift){
			hue += gHueDelta;
			gHue=hue;
		}
	}
}


// --------------------------------------------------------------------
void p_down(uint8_t hue, boolean hold, boolean drift){

	for(int i=NUM_LEDS - 1; i >=0; i-- ){
		if(!anim_is_on){return;}
		
		// on --------------------------
		leds[i] = CHSV(hue, 255, 255);
		FastLED.show();
		Scheduler.delay(anim_time);

		//off --------
		if(! hold){
			leds[i] = CRGB::Black;
			FastLED.show();
			Scheduler.delay(ATIME_OFF);
		}

		//drift color -------
		if(drift){
			hue += gHueDelta;
			gHue=hue;
		}
	}
}
