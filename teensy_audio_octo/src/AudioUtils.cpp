#include "AudioUtils.h"

#include <Audio.h>
#include "Config.h"

// This array specifies how many of the FFT frequency bin
// to use for each horizontal pixel.  Because humans hear
// in octaves and FFT bins are linear, the low frequencies
// use a small number of bins, higher frequencies use more.
int frequencyBinsHorizontal[numberOfFrequencies] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
    3, 3, 3, 3, 4, 4, 4, 4, 4, 5,
    5, 5, 6, 6, 6, 7, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 12, 12, 13, 14, 15,
    15, 16, 17, 18, 19, 20, 22, 23, 24, 25};

// this array maps from fft bins, to frequencies in the range [0, numberOfFrequencies].
// each index is a single frequency, and the value is the corespending startBin.
// we calculate it once and access it fast by index
int frequencyBinsIndices[numberOfFrequencies + 1] = {0};

// Audio library objects
AudioInputAnalog adc1;   // xy=99,55
AudioAnalyzeFFT1024 fft; // xy=265,75
AudioOutputI2S2 i2s1;     // xy=378,99
AudioConnection patchCord1(adc1, 0, i2s1, 0);
AudioConnection patchCord2(adc1, 0, i2s1, 1);
AudioConnection patchCord3(adc1, fft);
AudioControlSGTL5000 sgtl5000_1; // xy=265,161

void fillFrequencyBinsIndices()
{
    int sum = 0;
    frequencyBinsIndices[0] = 0;
    for (unsigned int frequencyIndex = 0; frequencyIndex < numberOfFrequencies; frequencyIndex++)
    {
        sum += frequencyBinsHorizontal[frequencyIndex];
        frequencyBinsIndices[frequencyIndex + 1] = sum;
    }
}

void setupAudio()
{
    AudioMemory(30);
    sgtl5000_1.enable();
    sgtl5000_1.volume(1.0);

    fillFrequencyBinsIndices();
}

bool isAudioAvailable()
{
    return fft.available();
}

float readFrequencyLevel(int frequency)
{
    unsigned int binFirst = frequencyBinsIndices[frequency];
    unsigned int binLast = frequencyBinsIndices[frequency + 1] - 1; // binLast is included, therefor we need to subtract 1
    return fft.read(binFirst, binLast);
}

int DominantFrequencyBucket(int levelArr[], int arrSize,int bucketSize)
{
    int max = 0;
    int maxIndex = 0;
    int frequenciesResultSize = arrSize / bucketSize;
    for (int i = 0; i < frequenciesResultSize; i++)
    {
        int sum = 0;
        for (int j = 0; j < bucketSize; j++)
        {
            if (i*bucketSize + j < arrSize)
            {
                sum += levelArr[i*bucketSize + j];
            }
        }

        if (max < sum)
        {
            max = sum;
            maxIndex = i;
        }
    }

    return maxIndex;
}