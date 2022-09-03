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
#include <Wire.h>
#include <ColorUtils.h>
#include <AudioUtils.h>
#include <OctoWS2811.h>
#include <Config.h>

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

const unsigned int baseVal = 15;
float colorFadeFactor = 1.0; // 1 is none

// These parameters adjust the vertical thresholds
float maxLevel = 0.001;          // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.5;   // useful range is 0 to 0.7

// OctoWS2811 objects
const int bytesPerLed = 3;
const int ledsPerStrip = numberOfFrequencies * levelsPerFrequency / 8;
const int numPins = 8;

DMAMEM int displayMemory[ledsPerStrip * numPins * bytesPerLed / 4];
int drawingMemory[ledsPerStrip * numPins * bytesPerLed / 4];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[levelsPerFrequency];

// Run once from setup, the compute the vertical levels
void computeVerticalLevels()
{
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y = 0; y < levelsPerFrequency; y++)
  {
    n = (float)y / (float)(levelsPerFrequency - 1);
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
  setupAudio();

  // the audio library needs to be given memory to start working
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

const int LevelColorInfluant[numberOfFrequencies] = {
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
int calcNextStepColor(int allLevels[numberOfFrequencies], float hue)
{
  int nextHue = 0;
  for (unsigned int i = 0; i < numberOfFrequencies; i++)
  {
    nextHue += minMaxNormalization(allLevels[i] * 0.6, 0, levelsPerFrequency, 0, LevelColorInfluant[i]);
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
  return y * numberOfFrequencies + x;
}

// A xy() function to turn display matrix coordinates for 8 flowers into the index numbers OctoWS2811 requires.
// for spred close level in similar high on the flower
unsigned int xy8FlowersSpred(unsigned int x, unsigned int y)
{
  unsigned int currPos = ((y * (numPins-1))%levelsPerFrequency * numberOfFrequencies)+ x;
  // Serial.println(currPos);
  return currPos;
}


// Run repetitively
void loop()
{
  if (!isAudioAvailable())
  {
    return;
  }

  int allLevelsPassThreshold[numberOfFrequencies] = {0};

  allLevelsPassThreshold[0] = 0; // TODO: reset all array

  for (unsigned int frequency = 0; frequency < numberOfFrequencies; frequency++)
  {
    // get the volume for each horizontal pixel position
    float currLevelRead = readFrequencyLevel(frequency);

    // uncomment to see the spectrum in Arduino's Serial Monitor
    // Serial.print(currLevelRead);
    // Serial.print("  ");
    for (unsigned int level = 0; level < levelsPerFrequency; level++)
    {
      // for each vertical pixel, check if above the threshold
      // and turn the LED on or off
      unsigned int octoIndex = xy(frequency, level);
      if (currLevelRead >= thresholdVertical[level])
      {
        allLevelsPassThreshold[frequency] += 1;
        int color = HSVtoRGB(currH, FadeMaxS, FadeMaxV);
        leds.setPixel(octoIndex, color);
      }
      else
      {
        int color = HSVtoRGB(currH, FadeMinS, FadeMinV);
        leds.setPixel(octoIndex, color);
      }
      // Serial.println(" ");
    }
    // Serial.println(" ");
  }

  currH = calcNextStepColor(allLevelsPassThreshold, currH);
  Serial.print(currH);
  Serial.println(" ");
  // after all pixels set, show them all at the same instant
  leds.show();
}
