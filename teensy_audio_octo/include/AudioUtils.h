#ifndef __AUDIO_UTILS_H__
#define __AUDIO_UTILS_H__

void setupAudio();
bool isAudioAvailable();

// read and return a single level for a givin frequency index.
// the frequency index is an aggregation of multiple fft bins.
// frequency value must be in range [0, numberOfFrequencies - 1]
float readFrequencyLevel(int frequency);

#endif //__AUDIO_UTILS_H__
