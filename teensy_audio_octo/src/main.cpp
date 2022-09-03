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
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <ColorUtils.h>
#include <OctoWS2811.h>

// The display size and color to use
const unsigned int matrix_width = 60;
const unsigned int matrix_height = 40;

// max HSV color ( Blink )
const float FadeMaxH = 360;
const float FadeMaxS = 100;
const float FadeMaxV = 100;

// min HSV color ( Solid / base color )
const float FadeMinH = 30;
const float FadeMinS = 100;
const float FadeMinV = 6;

// current HSV color
float currH = FadeMinH;
float currS = FadeMinS;
float currV = FadeMinV;

const unsigned int baseVal = 15;
float colorFadeFactor = 1.0; // 1 is none

// These parameters adjust the vertical thresholds
float maxLevel = 0.001;          // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.5;   // useful range is 0 to 0.7

// OctoWS2811 objects
const int bytesPerLed = 3;
const int ledsPerStrip = matrix_width * matrix_height / 8;
const int numPins = 8;

DMAMEM int displayMemory[ledsPerStrip * numPins * bytesPerLed / 4];
int drawingMemory[ledsPerStrip * numPins * bytesPerLed / 4];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

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

// setting up the code for the 2 potentiometers
const int POT_PIN_SENSATIVITY = 3;
const int POT_PIN_COLOR = 4;
float prevPotValSensativity = maxLevel;
float colorRangeFactor = 1;

int analogReadZeroToOne(int pin)
{
  int val = analogRead(pin);
  float rangeVal = val / 1024; // 10bit sample
  return rangeVal;
}

void ApplyPotentiometerSettings()
{
  // read the potentiometer values
  int potValSensativity = analogReadZeroToOne(POT_PIN_SENSATIVITY);
  int potValColor = analogReadZeroToOne(POT_PIN_COLOR);

  // apply the potentiometer values
  if (fabsf(prevPotValSensativity - potValSensativity) > 0.001)
  {
    maxLevel = potValSensativity;
    computeVerticalLevels();
    prevPotValSensativity = potValSensativity;
  }
  colorRangeFactor = potValColor;
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

const int LevelColorInfluant[matrix_width] = {
  12,12,
  12,12,12,12,12,12,12, 
  10,10,10,10,10,10,10, 
  10,10,10,10,10,10,10, 
  8,8,8,8,8,8,8,
  6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,
  4,4,4,4,4,4,4,
  2,2,2,2,2,2,2,
  1,1
  };
int calcNextStepColor(int allLevels[matrix_width], float hue)
{
  int nextHue = 0;
  for (int i = 0; i < matrix_width; i++)
  {
    nextHue += minMaxNormalization(allLevels[i] * 0.6, 0, matrix_height, 0, LevelColorInfluant[i]);
  }

  nextHue = (colorRangeFactor * nextHue) + (1.0 - colorRangeFactor) * hue;

  if (nextHue > FadeMaxH)
  {
    nextHue = FadeMaxH;
  }

  if (nextHue < FadeMinH)
  {
    nextHue = FadeMinH;
  }

  return nextHue;
}

// A simple xy() function to turn display matrix coordinates
// into the index numbers OctoWS2811 requires.  If your LEDs
// are arranged differently, edit this code...
unsigned int xy(unsigned int x, unsigned int y)
{
  return y * matrix_width + x;
}

// A xy() function to turn display matrix coordinates for 8 flowers into the index numbers OctoWS2811 requires.
// for spred close level in similar high on the flower
const unsigned int FLOWER_NUMBER = 8;
unsigned int xy8FlowersSpred(unsigned int x, unsigned int y)
{
  unsigned int currPos = ((y * (FLOWER_NUMBER-1))%matrix_height * matrix_width)+ x;
  Serial.println(currPos);
  return currPos;
}


// Run repetitively
void loop()
{
  if (!fft.available())
  {
    return;
  }

  float level;
  int allLevelsPassThreshold[matrix_width] = {0};

  // freqBin counts which FFT frequency data has been used,
  // starting at low frequency
  unsigned int freqBin = 0;
  allLevelsPassThreshold[0] = 0; // TODO: reset all array

  for (unsigned int x = 0; x < matrix_width; x++)
  {
    // get the volume for each horizontal pixel position
    level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);

    // level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);
    // uncomment to see the spectrum in Arduino's Serial Monitor
    // Serial.print(level);
    // Serial.print("  ");
    for (unsigned int y = 0; y < matrix_height; y++)
    {
      // for each vertical pixel, check if above the threshold
      // and turn the LED on or off
      if (level >= thresholdVertical[y])
      {
        allLevelsPassThreshold[x] += 1;
        currV = FadeMaxV;
        int color = HSVtoRGB(currH, FadeMaxS, FadeMaxV);
        // Serial.print(currH);
        // Serial.print(" ");
        leds.setPixel(xy(x, y), color);
      }
      else
      {
        currV = currV - (FadeMaxV - FadeMinV) / colorFadeFactor;
        if (currV < FadeMinV)
        {
          currV = FadeMinV;
        }

        int color = HSVtoRGB(currH, FadeMinS, FadeMinV);
        // Serial.print(color);
        // Serial.print(" ");
        leds.setPixel(xy(x, y), color);
      }
      // Serial.println(" ");
    }
    // Serial.println(" ");
    // increment the frequency bin count, so we display
    // low to higher frequency from left to right
    freqBin = freqBin + frequencyBinsHorizontal[x];
  }

  currH = calcNextStepColor(allLevelsPassThreshold, currH);
  Serial.print(currH);
  Serial.println(" ");
  // after all pixels set, show them all at the same instant
  leds.show();
}
