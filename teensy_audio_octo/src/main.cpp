#include <Wire.h>
#include <ColorUtils.h>
#include <AudioUtils.h>
#include <OctoWS2811.h>
#include <Config.h>

// max HSV color ( Blink )
const int FadeMaxH = 300;
const int FadeMaxS = 100;
const int FadeMaxV = 51;

// min HSV color ( Solid / base color )
const int FadeMinH = 10;
const int FadeMinS = 100;
const int FadeMinV = 10;

// current HSV color
float currH = FadeMinH;

// bit trshold for snack anaimation
const int beatThreshold = 24; // 0-31
bool isBeat = false;
bool prevIsBeat = false;
int beatOffset = 0; // 0-nuber of leds

const unsigned int baseVal = 15;

// These parameters adjust the vertical thresholds
float maxLevel = 0.05;           // 1.0 = max, lower is more "sensitive"
double colorRangeFactor = 0.1;  // 0-1 when 1 is the fastest change between colors
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.7;   // useful range is 0 to 0.7
const unsigned int blinkThresholdVerticalFromLevel = 10;

// OctoWS2811 objects
const int bytesPerLed = 3;
const int ledsPerStrip = numberOfFrequencies * levelsPerFrequency / 8;
const int numPins = 8;
const int ledsNumber = ledsPerStrip * numPins;

DMAMEM int displayMemory[ledsPerStrip * numPins * bytesPerLed / 4];
int drawingMemory[ledsPerStrip * numPins * bytesPerLed / 4];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[levelsPerFrequency];
float thresholdVerticalBlink[levelsPerFrequency];

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

  for (y = 0; y < levelsPerFrequency; y++)
  {
    if (y > blinkThresholdVerticalFromLevel)
    {
      thresholdVerticalBlink[y] = thresholdVertical[blinkThresholdVerticalFromLevel];
    }
    else
    {
      thresholdVerticalBlink[y] = thresholdVertical[y];
    }
  }
}

// setting up the code for the 2 potentiometers
const int POT_PIN_Sensitivity = 3;
const int POT_PIN_COLOR = 4;
float prevPotValSensitivity = maxLevel;

void setIsBeatAndOffset(int allLevels[numberOfFrequencies])
{
  int currLevelPassTrashold = 0;
  for (int i = 0; i < numberOfFrequencies; i++)
  {
    if (allLevels[i] > beatThreshold)
    {
      currLevelPassTrashold++;
    }
  }

  prevIsBeat = isBeat;
  if (currLevelPassTrashold > 20 && !prevIsBeat)
  {
    isBeat = true;
  }
  else if (currLevelPassTrashold < 20 && prevIsBeat)
  {
    isBeat = false;
  }

  if (isBeat)
  {
    beatOffset++;
    beatOffset = beatOffset % ledsPerStrip;
  }
}

float analogReadZeroToOne(int pin)
{
  return analogRead(pin) / 1024.0; // 10bit sample
}

void ApplyPotentiometerSettings()
{
  // read the potentiometer values
  float potValSensitivity = 0.04; // analogReadZeroToOne(POT_PIN_Sensitivity);
  float potValColor = 0.08;

  // apply the potentiometer values
  if (fabsf(prevPotValSensitivity - potValSensitivity) > 0.01)
  {
    maxLevel = potValSensitivity;
    computeVerticalLevels();
    prevPotValSensitivity = potValSensitivity;
  }
  colorRangeFactor = potValColor;
  Serial.print("Pot color: ");
  Serial.print(potValColor);
  Serial.print(" Pot sense: ");
  Serial.println(potValSensitivity);
}

// Run setup once
void setup()
{
  Serial.begin(115200);
  setupAudio();

  // the audio library needs to be given memory to start working
  // compute the vertical thresholds before starting
  computeVerticalLevels();

  Serial.print("computeVerticalLevels:");
  for (int i = 0; i < levelsPerFrequency; i++)
  {
    Serial.print(thresholdVertical[i]);
    Serial.print(",");
  }
  Serial.println("Setup yayyyyyy22222!!");
  // turn on the display
  leds.begin();
}

// retun int 0-360
const int octavaNumber = 7;

float minMaxNormalization(float value, float min, float max, float newMin, float newMax)
{
  return (value - min) * (newMax - newMin) / (max - min) + newMin;
}

const int maxAllLevelsValue = levelsPerFrequency * numberOfFrequencies;
const int LevelColorInfluant[numberOfFrequencies] = {
    16, 16,
    15, 15, 15, 15, 15, 15, 15,
    8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8,
    11, 11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 12, 12, 12,
    12, 12};
float calcNextStepColor(int allLevels[numberOfFrequencies], double hue)
{
  long nextHue = 0;
  for (unsigned int i = 1; i < numberOfFrequencies; i++)
  {
    nextHue += (long)allLevels[i]; // minMaxNormalization(allLevels[i], 0, levelsPerFrequency, 0, 6);//((FadeMaxH-FadeMinH)/numberOfFrequencies)+1);
  }

  // Serial.print(nextHue);
  // Serial.print("->");

  float nextHueMul = (float)nextHue * 1.8;

  if (nextHueMul > maxAllLevelsValue)
  {
    nextHue = maxAllLevelsValue - 1;
  }
  else
  {
    nextHue = (long)nextHueMul;
  }

  nextHue = map(nextHue, 0, maxAllLevelsValue, FadeMinH, 359); //((FadeMaxH-FadeMinH)/numberOfFrequencies)+1);
  // Serial.print(nextHue);
  // Serial.print("->");

  nextHue = (colorRangeFactor * (double)nextHue) + (1.0 - colorRangeFactor) * (double)hue;
  // Serial.println(nextHue);
  if (nextHue > FadeMaxH)
  {
    nextHue = FadeMaxH;
  }

  if (nextHue < FadeMinH)
  {
    nextHue = FadeMinH;
  }
  // Serial.println(nextHue);
  return nextHue;
}

const int colorOffsetRange = 40;
const int octavaOnTrashold = 25;// 0 - 32
const int octavaSpred[8] = {0, 1, 2, 4, 8, 15, 30, 60};
float octavaColorSpredOffsets[octavaNumber] = {0};
void calcFlowerOctaveColorOffset(int allLevels[numberOfFrequencies]) {
  int startIndex = 0;
  int totalOCtavasOn = 0;

  for (int i = 1; i <= octavaNumber; i++) {
    float prevOctavaColorSpredOffset = octavaColorSpredOffsets[i];
    octavaColorSpredOffsets[i] = 0;
    for (int j=startIndex; j < octavaSpred[i]; j++) {
      octavaColorSpredOffsets[i] += allLevels[j];
    }
    octavaColorSpredOffsets[i] = octavaColorSpredOffsets[i] / (octavaSpred[i]-startIndex);

    if ( octavaColorSpredOffsets[i] > octavaOnTrashold) {
      totalOCtavasOn++;
    }

    octavaColorSpredOffsets[i] = map(octavaColorSpredOffsets[i],0,40,0,colorOffsetRange);
    octavaColorSpredOffsets[i] = (0.09 * (double)octavaColorSpredOffsets[i]) + (1.0 - 0.09) * (double)prevOctavaColorSpredOffset;
    startIndex = octavaSpred[i];
  }

}

float getFlowerColoroffset(int octoIdex,float hue){
  int currFlower = ((octoIdex / ledsPerStrip) % octavaNumber);
  float newHue = hue + octavaColorSpredOffsets[currFlower];
  if (newHue > FadeMaxH){
    newHue = FadeMaxH;
  }
  return newHue;
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
  unsigned int currPos = ((y * (numPins - 1)) % levelsPerFrequency * numberOfFrequencies) + x;
  // Serial.println(currPos);
  return currPos;
}

unsigned int getBitOffset(unsigned int ledPosition)
{
  return (ledPosition + beatOffset) % ledsNumber;
}

void initArr(int arr[], int size)
{
  for (int i = 0; i < size; i++)
  {
    arr[i] = 0;
  }
}

// Run repetitively
void loop()
{
  if (!isAudioAvailable())
  {
    return;
  }

  // restore to apply potentiometer values once physically connected
  // ApplyPotentiometerSettings();

  int allLevelsPassThreshold[numberOfFrequencies];
  initArr(allLevelsPassThreshold, numberOfFrequencies);

  for (unsigned int frequency = 0; frequency < numberOfFrequencies; frequency++)
  {
    // get the volume for each horizontal pixel position
    float currLevelRead = readFrequencyLevel(frequency);
    // Serial.print(currH);
    // Serial.print("  ");

    // Serial.print(flowerCurrH);
    // Serial.print("  ");
    // Serial.println("");
    // uncomment to see the spectrum in Arduino's Serial Monitor
    // Serial.print(currLevelRead);
    // Serial.print("  ");
    for (unsigned int level = 0; level < levelsPerFrequency; level++)
    {
      // for each vertical pixel, check if above the threshold
      // and turn the LED on or off
      unsigned int octoIndex = xy8FlowersSpred(frequency, level);
      octoIndex = getBitOffset(octoIndex);
      float flowerCurrH =getFlowerColoroffset(octoIndex,currH);// currH;/

      // calc trashold for blink
      if (currLevelRead >= thresholdVerticalBlink[level])
      {
        int color = makeColor(flowerCurrH, FadeMaxS, FadeMaxV);
        leds.setPixel(octoIndex, color);
      }
      else
      {
        int color = makeColor(flowerCurrH, FadeMinS, FadeMinV);
        leds.setPixel(octoIndex, color);
      }

      if (currLevelRead >= thresholdVertical[level])
      {
        allLevelsPassThreshold[frequency] += 1;
      }

      // Serial.println(" ");
    }
    // Serial.println(" ");
  }

  currH = calcNextStepColor(allLevelsPassThreshold, currH);
  setIsBeatAndOffset(allLevelsPassThreshold);
  calcFlowerOctaveColorOffset(allLevelsPassThreshold);

  // for (int i = 0; i < octavaNumber; i++)
  // {
  //   Serial.print(octavaColorSpredOffsets[i]);
  //   Serial.print("  ");
  // }
  // Serial.print(currH);
  Serial.println(" ");
  // after all pixels set, show them all at the same instant
  leds.show();
}
