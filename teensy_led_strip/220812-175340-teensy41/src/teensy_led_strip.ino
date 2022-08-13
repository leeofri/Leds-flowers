#include "Fast_octo_LED.h"
#include <Audio.h>
#include <math.h>
#define COLOR_ORDER RGB
CTeensy4Controller<COLOR_ORDER, WS2811_800kHz> *pcontroller;

const int numPins = 1;
// byte pinList[numPins] = {33, 34, 35, 36, 37, 38, 39, 40};
byte pinList[numPins] = {19};
const int ledsPerStrip = 64;

#define OCTAVE 1   //   // Group buckets into octaves  (use the log output function LOG_OUT 1)
#define OCT_NORM 0 // Don't normalise octave intensities by number of bins
#define FHT_N 256  // set to 256 point fht

// int noise[] = {204, 188, 68, 73, 150, 98, 88, 68}; // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}
int noise[] = {100, 95, 54, 43, 43, 43, 38, 30}; // noise for mega adk
// int noise[] = {204, 190, 108, 85, 65, 65, 55, 60}; // noise for mega adk
// int noise[] = {204,195,100,90,85,80,75,75}; // noise for NANO
// int noise[] = {204, 198, 100, 85, 85, 80, 80, 80};
float noise_fact[] = {15, 7, 1.5, 1, 1.2, 1.4, 1.7, 3};     // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}
float noise_fact_adj[] = {15, 7, 1.5, 1, 1.2, 1.4, 1.7, 3}; // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}

#define LED_PIN 19
#define LED_TYPE WS2811_800kHz

// Params for width and height
const uint8_t kMatrixWidth = numPins;
const uint16_t kMatrixHeight = ledsPerStrip;
//#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define NUM_LEDS 64

CRGB leds[NUM_LEDS];

// These buffers need to be large enough for all the pixels.
// The total number of pixels is "ledsPerStrip * numPins".
// Each pixel needs 3 bytes, so multiply by 3.  An "int" is
// 4 bytes, so divide by 4.  The array is created using "int"
// so the compiler will align it to 32 bit memory.
DMAMEM int displayMemory[ledsPerStrip * numPins * 3 / 4];
int drawingMemory[ledsPerStrip * numPins * 3 / 4];
OctoWS2811 octo(ledsPerStrip, displayMemory, drawingMemory, WS2811_RGB | WS2811_800kHz, numPins, pinList);

int counter2 = 0;

// GUItool: begin automatically generated code
AudioInputAnalog adc3;       // xy=197,73
AudioAnalyzeFFT256 fft256_1; // xy=361,47
AudioOutputI2S i2s1;         // xy=378,99
AudioConnection patchCord1(adc3, 0, i2s1, 0);
AudioConnection patchCord2(adc3, 0, i2s1, 1);
AudioConnection patchCord3(adc3, fft256_1);
AudioControlSGTL5000 sgtl5000_1; // xy=265,161
// GUItool: end automatically generated code

void setup()
{

    AudioMemory(30);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);

    Serial.begin(115200);
    while (!Serial)
        ; // wait for Arduino Serial Monitor
    Serial.println("FFT test");
    octo.begin();
    pcontroller = new CTeensy4Controller<COLOR_ORDER, WS2811_800kHz>(&octo);

    FastLED.addLeds(pcontroller, leds, numPins * ledsPerStrip);
    FastLED.setBrightness(84);

    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
}

void loop()
{
    static uint8_t hue = 0;
    int analogVal;  
    int analogValMap;
    // Serial.print("x");
    // First slide the led in one direction
    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Set the i'th led to red
        leds[i] = CHSV(hue++, 255, analogValMap);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        // Wait a little bit before we loop around and do it again
        delay(20);
        analogVal = analogRead(3);
        analogValMap = map(analogVal, 0, 1023, 0, 255);
        Serial.print("analog 3 is: ");
        Serial.println(analogVal);
    }
    // Serial.print("x");
    // Now go in the other direction.
    for (int i = (NUM_LEDS)-1; i >= 0; i--)
    {

        // Set the i'th led to red
        leds[i] = CHSV(hue++, 255, analogVal);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        // Wait a little bit before we loop around and do it again
        delay(20);
        analogVal = analogRead(3);
        analogValMap = map(analogVal, 0, 1023, 0, 255);
        Serial.print("analog 3 is: ");
        Serial.println(analogVal);
    }
    //---------------
    //     int prev_j[8];
    //     float debug_j[8];
    //     int beat = 0;
    //     int prev_oct_j;
    //     int counter = 0;
    //     int prev_beat = 0;
    //     int led_index = 0;
    //     int saturation = 0;
    //     int saturation_prev = 0;
    //     int brightness = 0;
    //     int brightness_prev = 0;
    //     float fht_oct_out[8] = {0};

    //     while (1)
    //     { // reduces jitter

    //         //////////////////////////////// arduino
    //         // cli();  // UDRE interrupt slows this way down on arduino1.0
    //         // for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
    //         //   while (!(ADCSRA & 0x10)); // wait for adc to be ready
    //         //   ADCSRA = 0xf5; // restart adc
    //         //   byte m = ADCL; // fetch adc data
    //         //   byte j = ADCH;
    //         //   int k = (j << 8) | m; // form into an int
    //         //   k -= 0x0200; // form into a signed int
    //         //   k <<= 6; // form into a 16b signed int
    //         //   fht_input[i] = k; // put real data into bins
    //         // }
    //         // fht_window(); // window the data for better frequency response
    //         // fht_reorder(); // reorder the data before doing the fht
    //         // fht_run(); // process the data in the fht
    //         // fht_mag_octave(); // take the output of the fht  fht_mag_log()

    //         //////////// teensy reads 256 samples
    //         if (fft256_1.available())
    //         {
    //             // fht_oct_out[0] = fft256_1.read(0);
    //             // fht_oct_out[1] = fft256_1.read(1);
    //             // fht_oct_out[2] = fft256_1.read(2, 4) / (4-2)+1;
    //             // fht_oct_out[3] = fft256_1.read(5, 8) / (8-5)+1;
    //             // fht_oct_out[4] = fft256_1.read(9, 16) / (16-9)+1;
    //             // fht_oct_out[5] = fft256_1.read(17, 32) / (32-17)+1;
    //             // fht_oct_out[6] = fft256_1.read(33, 64) / (64-33)+1;
    //             // fht_oct_out[7] = fft256_1.read(65, 127) / (127-65)+1;

    //             fht_oct_out[0] = fft256_1.read(0);
    //             fht_oct_out[1] = fft256_1.read(1);
    //             fht_oct_out[2] = fft256_1.read(2, 4);
    //             fht_oct_out[3] = fft256_1.read(5, 8);
    //             fht_oct_out[4] = fft256_1.read(9, 16);
    //             fht_oct_out[5] = fft256_1.read(17, 32);
    //             fht_oct_out[6] = fft256_1.read(33, 64);
    //             fht_oct_out[7] = fft256_1.read(65, 127);

    //             for (int i = 0; i < 7; i++)
    //             {
    //                 // fht_oct_out[i] = 16*log(sqrt(fft256_1.read(i*15,i*15+15)));
    //                 // fht_oct_out[i] = 16*log(sqrt(fht_oct_out[i]));

    //                 // Serial.print("bef:");
    //                 // Serial.print(fht_oct_out[i]);
    //                 // Serial.print(" ");
    //                 // fht_oct_out[i] = 16*log(sqrt(fht_oct_out[i]));
    //                 fht_oct_out[i] = fht_oct_out[i]*1000;
    //                 // Serial.print("aft:");
    //                 // Serial.print(fht_oct_out[i]);
    //                 // Serial.println(" ");

    //                 // fht_oct_out[i] = 16*log(sqrt(read));
    //             }
    //         }

    //         // every 50th loop, adjust the volume accourding to the value on A2 (Pot)
    //         // if (counter >= 200)
    //         // {
    //         //     // for (int i = 0; i < 7; i++)
    //         //     // {
    //         //     //     Serial.print(fht_oct_out[i]);
    //         //     //     Serial.print(" ");
    //         //     // }
    //         //     // Serial.println();
    //         //     counter=0;
    //         // }

    //         // counter++;
    //         // End of Fourier Transform code - output is stored in fht_oct_out[i].

    //         int analogVal = analogRead(3);
    //         float master_volume = (analogVal + 0.1) / 1000 + .5;
    //         // float master_volume  = 0.8;
    //         // Serial.print("master_volume: ");
    //         // Serial.print(master_volume);

    //         for (int i = 1; i < 8; i++)
    //         {
    //             noise_fact_adj[i] = noise_fact[i] * master_volume;
    //         }
    //         // i=0-7 frequency (octave) bins (don't use 0 or 1), fht_oct_out[1]= amplitude of frequency for bin 1
    //         // for loop a) removes background noise average and takes absolute value b) low / high pass filter as still very noisy
    //         // c) maps amplitude of octave to a colour between blue and red d) sets pixel colour to amplitude of each frequency (octave)
    //         for (int i = 1; i < 8; i++)
    //         { // goes through each octave. skip the first 1, which is not useful

    //             int j;
    //             // Serial.print("noise stuff:");
    //             debug_j[i] = fht_oct_out[i];
    //             j = (fht_oct_out[i] - noise[i]); // take the pink noise average level out, take the asbolute value to avoid negative numbers
    //             if (j < 10)
    //             {
    //                 j = 0;
    //             }
    //             j = j * noise_fact_adj[i];

    //             if (j < 10)
    //             {
    //                 j = 0;
    //             }
    //             else
    //             {
    //                 j = j * noise_fact_adj[i];
    //                 if (j > 180)
    //                 {
    //                     if (i >= 7)
    //                     {
    //                         beat += 2;
    //                     }
    //                     else
    //                     {
    //                         beat += 1;
    //                     }
    //                 }
    //                 j = j / 30;
    //                 j = j * 30; // (force it to more discrete values)
    //             }

    //             // Serial.print(j);
    //             prev_j[i] = j;
    //             // debug_j[i] = j;

    //             // Serial.print(" ");

    //             // this fills in 11 LED's with interpolated values between each of the 8 OCT values
    //             if (i >= 2)
    //             {
    //                 led_index = 2 * i - 3;
    //                 prev_oct_j = (j + prev_j[i - 1]) / 2;

    //                 saturation = constrain(j + 30, 0, 255);
    //                 saturation_prev = constrain(prev_oct_j + 30, 0, 255);
    //                 brightness = constrain(j, 0, 255);
    //                 brightness_prev = constrain(prev_oct_j, 0, 255);

    //                 if (brightness == 255)
    //                 {
    //                     saturation = 50;
    //                     brightness = 200;
    //                 }
    //                 if (brightness_prev == 255)
    //                 {
    //                     saturation_prev = 50;
    //                     brightness_prev = 200;
    //                 }

    //                 for (uint16_t y = 0; y < kMatrixHeight; y++)
    //                 {
    //                     leds[XY(led_index - 1, y)] = CHSV(j + y * 30, saturation, brightness);
    //                     if (i > 2)
    //                     {
    //                         prev_oct_j = (j + prev_j[i - 1]) / 2;
    //                         leds[XY(led_index - 2, y)] = CHSV(prev_oct_j + y * 30, saturation_prev, brightness_prev);
    //                     }
    //                 }
    //             }
    //         }

    //         // Serial.print("beat");
    //         // Serial.print(beat);
    //         if (beat >= 7)
    //         {
    //             fill_solid(leds, NUM_LEDS, CRGB::Gray);
    //             FastLED.setBrightness(120);

    //             //    FastLED.setBrightness(200);
    //         }
    //         else
    //         {
    //             if (prev_beat != beat)
    //             {
    //                 FastLED.setBrightness(40 + beat * beat * 5);
    //                 prev_beat = beat;
    //             }
    //         }

    //         FastLED.show();
    //         if (beat)
    //         {
    //             counter2 += ((beat + 4) / 2 - 2);
    //             if (counter2 < 0)
    //             {
    //                 counter2 = 1000;
    //             }
    //             if (beat > 3 && beat < 7)
    //             {
    //                 FastLED.delay(20);
    //             }
    //             beat = 0;
    //         }

    //         if (counter >= 50)
    //         {
    //             for (int i = 0; i < 8; i++)
    //             {
    //                 Serial.print(debug_j[i]);
    //                 Serial.print(" ");
    //             }
    //             Serial.print("  |  ");
    //              for (int i = 0; i < 8; i++)
    //             {
    //                 Serial.print(prev_j[i]);
    //                 Serial.print(" ");
    //             }
    //             Serial.println();
    //             counter = 0;
    //         }

    //         counter++;
    //         // End of Fourier Transform code - output is stored in fht_oct_out[i].
    //     }
    // }

    // // Param for different pixel layouts
    // const bool kMatrixSerpentineLayout = true;
    // // Set 'kMatrixSerpentineLayout' to false if your pixels are
    // // laid out all running the same way, like this:

    // // Set 'kMatrixSerpentineLayout' to true if your pixels are
    // // laid out back-and-forth, like this:

    // uint16_t XY(uint8_t x, uint8_t y)
    // {
    //     uint16_t i;

    //     if (kMatrixSerpentineLayout == false)
    //     {
    //         i = (y * kMatrixWidth) + x;
    //     }

    //     if (kMatrixSerpentineLayout == true)
    //     {
    //         if (y & 0x01)
    //         {
    //             // Odd rows run backwards
    //             uint8_t reverseX = (kMatrixWidth - 1) - x;
    //             i = (y * kMatrixWidth) + reverseX;
    //         }
    //         else
    //         {
    //             // Even rows run forwards
    //             i = (y * kMatrixWidth) + x;
    //         }
    //     }

    //     i = (i + counter2) % NUM_LEDS;
    //     return i;
}
