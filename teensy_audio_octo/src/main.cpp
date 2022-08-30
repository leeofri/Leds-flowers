// LED Audio Spectrum Analyzer Display
//
// Creates an impressive LED light show to music input
//   using Teensy 3.1 with the OctoWS2811 adaptor board
//   http://www.pjrc.com/store/teensy31.html
//   http://www.pjrc.com/store/octo28_adaptor.html
//
// Line Level Audio Input connects to analog pin A3
//   Recommended input circuit:
//   http://www.pjrc.com/teensy/gui/?info=AudioInputAnalog
//
// This example code is in the public domain.

// #include <OctoWS2811.h>
#include <Fast_octo_LED.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <makeColor.h>

// The display size and color to use
const unsigned int matrix_width = 60;
const unsigned int matrix_height = 32;

// max HSV color ( Blink )
const float FadeMaxH = 360;
const float FadeMaxS = 100;
const float FadeMaxV = 60;

// min HSV color ( Solid / base color )
const float FadeMinH = 60;
const float FadeMinS = 100;
const float FadeMinV = 10;

// current HSV color
float currH = FadeMinH;
float currS = FadeMinS;
float currV = FadeMinV;

const unsigned int baseVal = 15;
const unsigned int MyBlinkColor = HSVtoRGB(FadeMaxH, FadeMaxS, FadeMaxV); // 0xe60073; // makeColor(330, 100, maxVal);//;
const unsigned int myBaseColor = HSVtoRGB(FadeMinH, FadeMinS, FadeMinV);  // 0x000000;// 0x1a000d;//makeColor(330, 100, baseVal);//0x1a000d;
float colorFadeFactor = 1.0;                                              // 1 is none
float colorChangeFactor = 50;                                             // 1 is none

// These parameters adjust the vertical thresholds
const float maxLevel = 0.06;     // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.3;   // useful range is 0 to 0.7

// OctoWS2811 objects
const int ledsPerPin = matrix_width * matrix_height / 8;
DMAMEM int displayMemory[ledsPerPin * 6];
int drawingMemory[ledsPerPin * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerPin, displayMemory, drawingMemory, config);
CRGB fast_leds[ledsPerPin * 6];
CTeensy4Controller<RGB, WS2811_800kHz> *pcontroller;

// Audio library objects
AudioInputAnalog adc1;   // xy=99,55
AudioAnalyzeFFT1024 fft; // xy=265,75
AudioOutputI2S i2s1;     // xy=378,99
AudioConnection patchCord1(adc1, 0, i2s1, 0);
AudioConnection patchCord2(adc1, 0, i2s1, 1);
AudioConnection patchCord3(adc1, fft);
AudioControlSGTL5000 sgtl5000_1; // xy=265,161

// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[matrix_height];

// This array specifies how many of the FFT frequency bin
// to use for each horizontal pixel.  Because humans hear
// in octaves and FFT bins are linear, the low frequencies
// use a small number of bins, higher frequencies use more.
int frequencyBinsHorizontal[matrix_width] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
    3, 3, 3, 3, 4, 4, 4, 4, 4, 5,
    5, 5, 6, 6, 6, 7, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 12, 12, 13, 14, 15,
    15, 16, 17, 18, 19, 20, 22, 23, 24, 25};

// Run once from setup, the compute the vertical levels
void computeVerticalLevels()
{
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y = 0; y < matrix_height; y++)
  {
    n = (float)y / (float)(matrix_height - 1);
    logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
    linearLevel = 1.0 - n;
    linearLevel = linearLevel * linearBlend;
    logLevel = logLevel * (1.0 - linearBlend);
    thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
  }
}

// Run setup once
void setup()
{
  Serial.begin(115200);
  // the audio library needs to be given memory to start working
  AudioMemory(30);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  // compute the vertical thresholds before starting
  computeVerticalLevels();

  Serial.println("Setup yayyyyyy22222!!");
  // turn on the display
  leds.begin();
  pcontroller = new CTeensy4Controller<RGB, WS2811_800kHz>(&leds);
}

// retun int 0-360
const int octavaNumber = 3;
float octavaArr[octavaNumber] = {0, 0, 0};
float prevOctavaArr[octavaNumber] = {0, 0, 0};
const int octavaNumbersRange[octavaNumber + 1] = {
    0,
    31,
    134,
    450};

int minMaxNormalization(int value, int min, int max, int newMin, int newMax)
{
  return (value - min) * (newMax - newMin) / (max - min) + newMin;
}

int calcNextStepColor(int allLevels[matrix_width],float hue)
{
  int nextHue=0;
  for (int i=0; i<matrix_width; i++)
  {
   nextHue += minMaxNormalization(allLevels[i], 0, matrix_height, 0, 6);
  }

  return nextHue;
}

void getHueFromOctava()
{
  // for(int i = 0; i < octavaNumber; i++){
  //   prevOctavaArr[i] = octavaArr[i];
  // octavaArr[i] = fft.read(9, 10);
  float levelOct = fft.read(1, 6);
  // amir
  currH += levelOct / 5.0;
  if (currH >= 360.0)
  {
    currH -= 360.0;
  }
  // Serial.println(currH, 8);
  return;
  // end amir

  // Serial.print(prevOctavaArr[i]);
  // Serial.print(" ");
  if (levelOct > thresholdVertical[10])
  {
    currH = currH + 0.001;
    if (currH >= FadeMaxH)
    {
      currH = FadeMaxH;
    }
  }
  else
  {
    currH = currH - 0.001;
    if (currH <= FadeMinH)
    {
      currH = FadeMinH;
    }
  }
  // Serial.println(currH);
  // }
}

// A simple xy() function to turn display matrix coordinates
// into the index numbers OctoWS2811 requires.  If your LEDs
// are arranged differently, edit this code...
unsigned int xy(unsigned int x, unsigned int y)
{
  return y * matrix_width + x;
}

// Run repetitively
void loop()
{
  unsigned int x, y, freqBin;
  float level;
  int allLevelsPassThreshold[matrix_width] = {0};

  if (fft.available())
  {
    // freqBin counts which FFT frequency data has been used,
    // starting at low frequency
    freqBin = 0;
    allLevelsPassThreshold[0] = 0; // TODO: reset all array

    for (x = 0; x < matrix_width; x++)
    {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);

      // getHueFromOctava();
      // level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);
      // uncomment to see the spectrum in Arduino's Serial Monitor
      // Serial.print(level);
      // Serial.print("  ");
      for (y = 0; y < matrix_height; y++)
      {
        // for each vertical pixel, check if above the threshold
        // and turn the LED on or off
        int color;
        if (level >= thresholdVertical[15])
        {
          allLevelsPassThreshold[x] += 1;
          currV = FadeMaxV;
          color = HSVtoRGB(currH, FadeMaxS, FadeMaxV);
          Serial.print(color);
          Serial.print(" ");
          leds.setPixel(xy(x, y), color);
        }
        else
        {

          currV = currV - (FadeMaxV - FadeMinV) / colorFadeFactor;
          if (currV < FadeMinV)
          {
            currV = FadeMinV;
          }

          color = HSVtoRGB(currH, FadeMinS, FadeMinV);
          Serial.print(color);
          Serial.print(" ");
          leds.setPixel(xy(x, y), color);
        }
        Serial.println(" ");
      }
      // Serial.println(" ");
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + frequencyBinsHorizontal[x];
    }

    currH = calcNextStepColor(allLevelsPassThreshold, currH);
    // after all pixels set, show them all at the same instant
    leds.show();
    // FastLED.show();
    //  Serial.println();
  }
}
