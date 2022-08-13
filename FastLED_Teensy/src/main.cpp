#include "Fast_octo_LED.h"

CTeensy4Controller<RGB, WS2811_800kHz> *pcontroller;

const int numPins = 2;
// byte pinList[numPins] = {33, 34, 35, 36, 37, 38, 39, 40};
byte pinList[numPins] = {19, 2};
const int ledsPerStrip = 300;
const int NUM_LEDS = numPins * ledsPerStrip;
CRGB leds[NUM_LEDS];

// These buffers need to be large enough for all the pixels.
// The total number of pixels is "ledsPerStrip * numPins".
// Each pixel needs 3 bytes, so multiply by 3.  An "int" is
// 4 bytes, so divide by 4.  The array is created using "int"
// so the compiler will align it to 32 bit memory.
DMAMEM int displayMemory[ledsPerStrip * numPins * 3 / 4];
int drawingMemory[ledsPerStrip * numPins * 3 / 4];
OctoWS2811 octo(ledsPerStrip, displayMemory, drawingMemory, WS2811_RGB | WS2811_800kHz, numPins, pinList);


int analogVal, analogValMap;

void setup()
{
	Serial.begin(115200);
	octo.begin();
	pcontroller = new CTeensy4Controller<RGB, WS2811_800kHz>(&octo);

	FastLED.addLeds(pcontroller, leds, numPins * ledsPerStrip);
	FastLED.setBrightness(84);
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void loop() {
	static uint8_t hue = 0;
	// Serial.print("x");
	// First slide the led in one direction
	for(int i = 0; i < NUM_LEDS; i++) {
		// Set the i'th led to red
		leds[i] = CHSV(hue++, 255, analogValMap);
		// Show the leds
		FastLED.show();
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
		// Wait a little bit before we loop around and do it again
		delay(20);
		analogVal = analogRead(3);
		analogValMap = map(analogVal,0,1023,0,255);
		Serial.print("analog 3 is: ");
		Serial.println(analogVal);
	}
	// Serial.print("x");

	// Now go in the other direction.
	for(int i = (NUM_LEDS)-1; i >= 0; i--) {
		// Set the i'th led to red
		leds[i] = CHSV(hue++, 255, analogVal);
		// Show the leds
		FastLED.show();
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
		// Wait a little bit before we loop around and do it again
		delay(20);
		analogVal = analogRead(3);
		analogValMap = map(analogVal,0,1023,0,255);
		Serial.print("analog 3 is: ");
		Serial.println(analogVal);
	}
}