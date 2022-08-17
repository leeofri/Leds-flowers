#include "FastLED.h"

#include <Audio.h>
#include <math.h>

#define OCTAVE 1   //   // Group buckets into octaves  (use the log output function LOG_OUT 1)
#define OCT_NORM 0 // Don't normalise octave intensities by number of bins
#define FHT_N 256  // set to 256 point fht

int noise[] = {204, 188, 68, 73, 150, 98, 88, 68}; // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}
float noise_pink_weksler[] = {0.001932740741, 0.003002037037, 1.003792037, 1.002415704, 1.001474185, 1.000664852, 1.000250556, 1.000018741};

// int noise[] = {204,190,108,85,65,65,55,60}; // noise for mega adk
// int noise[] = {204,195,100,90,85,80,75,75}; // noise for NANO
// int noise[] = {204, 198, 100, 85, 85, 80, 80, 80};
float noise_fact[] = {15, 7, 1.5, 1, 1.2, 1.4, 1.7, 3};     // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}
float noise_fact_adj[] = {15, 7, 1.5, 1, 1.2, 1.4, 1.7, 3}; // noise level determined by playing pink noise and seeing levels [trial and error]{204,188,68,73,150,98,88,68}

#define LED_PIN 19
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

// Params for width and height
const uint8_t kMatrixWidth = 1;
const uint16_t kMatrixHeight = 300;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
//#define NUM_LEDS    15

// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;
// Set 'kMatrixSerpentineLayout' to false if your pixels are
// laid out all running the same way, like this:

// Set 'kMatrixSerpentineLayout' to true if your pixels are
// laid out back-and-forth, like this:

CRGB leds[NUM_LEDS];

int counter2 = 0;

// GUItool: begin automatically generated code
AudioInputAnalog adc1;       // xy=197,73
AudioAnalyzeFFT256 fft256_1; // xy=361,47
AudioOutputI2S i2s1;         // xy=378,99
AudioConnection patchCord1(adc1, 0, i2s1, 0);
AudioConnection patchCord2(adc1, 0, i2s1, 1);
AudioConnection patchCord3(adc1, fft256_1);
AudioControlSGTL5000 sgtl5000_1; // xy=265,161
AudioAnalyzeFFT1024 adsg;
const float minPink 
// GUItool: end automatically generated code

void setup()
{

    AudioMemory(30);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);

    while (!Serial)
        ; // wait for Arduino Serial Monitor
    Serial.println("FFT test");
    Serial.begin(115200);
    delay(1000);
    // FastLED.addLeds<kMatrixWidth, LED_TYPE, LED_PIN, COLOR_ORDER>(leds, kMatrixHeight).setCorrection(TypicalLEDStrip);

    // FastLED.setBrightness(200);
    // fill_solid(leds, NUM_LEDS, CRGB::Black);
    // FastLED.show();
}

void loop()
{
    int prev_octv[8];
    float debug_j[8];
    int beat = 0;
    int prev_oct_j;
    int counter = 0;
    int prev_beat = 0;
    int led_index = 0;
    int saturation = 0;
    int saturation_prev = 0;
    int brightness = 0;
    int brightness_prev = 0;
    float fht_oct_out[8] = {0};
    float oct_normal[8];

    while (1)
    {

        //////////// teensy reads 256 samples
        float master_volume = 1.0; //  float master_volume=(k+0.1)/1000 +.5;
        if (fft256_1.available())
        {
            fht_oct_out[0] = fft256_1.read(0);
            fht_oct_out[1] = fft256_1.read(1);
            fht_oct_out[2] = fft256_1.read(2, 4) / (4 - 2) + 1;
            fht_oct_out[3] = fft256_1.read(5, 8) / (8 - 5) + 1;
            fht_oct_out[4] = fft256_1.read(9, 16) / (16 - 9) + 1;
            fht_oct_out[5] = fft256_1.read(17, 32) / (32 - 17) + 1;
            fht_oct_out[6] = fft256_1.read(33, 64) / (64 - 33) + 1;
            fht_oct_out[7] = fft256_1.read(65, 127) / (127 - 65) + 1;

            for (int i = 0; i < 8; i++)
            {
                oct_normal[i] = fht_oct_out[i] - noise_pink_weksler[i] * master_volume;
                debug_j[i] = fht_oct_out[i];
            }

            // add minMax normelize
            // if (counter >= 5)
            // {
            for (int i = 0; i < 8; i++)
            {
                float added = debug_j[i];
                Serial.print(added, 7);
                Serial.print(" ");
            }

            Serial.println();
            //     counter = 0;
            // }

            // counter++;
        }

        // End of Fourier Transform code - output is stored in fht_oct_out[i].
        // i=0-7 frequency (octave) bins (don't use 0 or 1), fht_oct_out[1]= amplitude of frequency for bin 1
        // for loop a) removes background noise average and takes absolute value b) low / high pass filter as still very noisy
        // c) maps amplitude of octave to a colour between blue and red d) sets pixel colour to amplitude of each frequency (octave)
        // for (int i = 1; i < 8; i++)
        // {
        //     // goes through each octave. skip the first 1, which is not useful
        //     float currOctv = oct_normal[i];

        //     // TODO: set lower bondery to value not sure
        //     if (currOctv > 180) // High limit for octava to be considered
        //     {
        //         currOctv = 180;
        //     }
        //     else if (currOctv < 0) // Low limit for octava to be considered
        //     {
        //         currOctv = 0;
        //     }
        //     else
        //     {
        //         currOctv = currOctv;
        //     }
        //     {
        //         if (i >= 7)
        //         {
        //             beat += 2;
        //         }
        //         else
        //         {
        //             beat += 1;
        //         }
        //     }

        //     // Serial.print(j);
        //     prev_octv[i] = currOctv;

        //     // this fills in 11 LED's with interpolated values between each of the 8 OCT values
        //     int prev_oct_val;
        //     if (i >= 2)
        //     {
        //         led_index = 2 * i - 3;
        //         prev_oct_j = (j + prev_j[i - 1]) / 2;

        //         saturation = constrain(j + 30, 0, 255);
        //         saturation_prev = constrain(prev_oct_j + 30, 0, 255);
        //         brightness = constrain(j, 0, 255);
        //         brightness_prev = constrain(prev_oct_j, 0, 255);

        //         if (brightness == 255)
        //         {
        //             saturation = 50;
        //             brightness = 200;
        //         }
        //         if (brightness_prev == 255)
        //         {
        //             saturation_prev = 50;
        //             brightness_prev = 200;
        //         }

        //         for (uint16_t y = 0; y < kMatrixHeight; y++)
        //         {
        //             leds[XY(led_index - 1, y)] = CHSV(j + y * 30, saturation, brightness);
        //             if (i > 2)
        //             {
        //                 prev_oct_j = (j + prev_j[i - 1]) / 2;
        //                 leds[XY(led_index - 2, y)] = CHSV(prev_oct_j + y * 30, saturation_prev, brightness_prev);
        //             }
        //         }
        //     }
        // }

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
        //             for (int i = 0; i < 12; i++)
        //             {
        //                 Serial.print(debug_j[i]);
        //                 Serial.print(" ");
        //             }

        //             Serial.println();
        //             counter = 0;
        //         }

        //         counter++;
        //     }
        // }
    }
}

float get_min_array(float i[])
{
    float min = 1000;
    for (int j = 1; j < 8; j++)
    {
        if (i[j] < min)
        {
            min = i[j];
        }
    }
    return min;
}

void add_to_all(float arr[], float add)
{
    for (int i = 0; i < 8; i++)
    {
        arr[i] += add;
    }
}

uint16_t XY(uint8_t x, uint8_t y)
{
    uint16_t i;

    if (kMatrixSerpentineLayout == false)
    {
        i = (y * kMatrixWidth) + x;
    }

    if (kMatrixSerpentineLayout == true)
    {
        if (y & 0x01)
        {
            // Odd rows run backwards
            uint8_t reverseX = (kMatrixWidth - 1) - x;
            i = (y * kMatrixWidth) + reverseX;
        }
        else
        {
            // Even rows run forwards
            i = (y * kMatrixWidth) + x;
        }
    }

    i = (i + counter2) % NUM_LEDS;
    return i;
}

float min_max_normalize(float value, float min, float max)
{
    return (value - min) / (max - min);
}