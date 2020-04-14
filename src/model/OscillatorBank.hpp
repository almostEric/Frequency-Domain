#pragma once

#include <cmath>
#include "Oscillator.hpp"
#include "RingModulator.hpp"
#include "Interpolate.hpp"

#define NUM_OSCILLATORS 36

struct BankOutput {
  float outputMono;
  float outputLeft;
  float outputRight;
};

struct OscillatorBank {
  OscillatorBank() {
    blurAmount = 50; // Manually Adjust
    // set currentBank to 1 so that it changes to 0 on first run
    currentBank = 1;
  }

  void setFrequency(float *frequencies, float *volumes, uint8_t count) {
    numOscillators[currentBank] = count;

    // update the current bank values
    for (uint8_t i = 0; i < NUM_OSCILLATORS; i++) {
      if (i < count) {
        currentFrequencies[i] = frequencies[i];
        currentVolumes[i] = volumes[i];
      } else {
        currentFrequencies[i] = 0;
        currentVolumes[i] = 0;
      }
    }
  }

  void setFM(uint8_t *fmMatrix, float *fmAmount, float *fmIn, uint8_t count) {
    for (uint8_t i = 0; i < NUM_OSCILLATORS / 4; i++) {
      if (count == 0) {
        this->frequencyModulation[i] = 0;
      } else {        
        uint8_t j = fmMatrix[i];
        while (j >= count) {
          j -= count;
        }
        this->frequencyModulation[i] = fmIn[j] * fmAmount[i];
      }
    }
  }

  void setRM(bool rmActive, uint8_t *rmMatrix, float *rmMix) {
    this->ringModulationActive = rmActive;
    for (uint8_t i = 0; i < NUM_OSCILLATORS;i++) {
      this->rmMatrix[i]  = rmMatrix[i];
      this->rmMix[i] = rmMix[i];
    }
  }


  void setVoiceShift(int16_t shift) {
    this->voiceShift = shift;
  }


  void switchBanks() {
    currentBank = !currentBank;
    currentBlur = 0;
    // update the current bank values
    for (uint8_t i = 0; i < NUM_OSCILLATORS; i++) {
      oldFrequencies[i] = currentFrequencies[i];
      oldVolumes[i] = currentVolumes[i];
    }
  }


  BankOutput process(uint8_t waveShape, float sampleTime,float *panning) {
    uint8_t currentVoice = 0;
    float_4 oscFreq = 0;
    float volumes[NUM_OSCILLATORS];

    currentBlur++;
    if (currentBlur > blurAmount) {
      currentBlur = blurAmount;
    }

    float pct = blurAmount ? (float(currentBlur) / float(blurAmount)) : 1;
    for (uint8_t i = 0, c = 0; i < NUM_OSCILLATORS; ) {
      for (c = 0; c < 4; c++) {
        if (i + c < numOscillators[currentBank]) {
          oscFreq[c] = currentFrequencies[i + c] + frequencyModulation[i + c];
          volumes[i + c] = (oldVolumes[i + c] * (1 - pct)) + (currentVolumes[i + c] * pct);
        }
      }


      // set up the current bank and step it
      banks[0][currentVoice].setFrequency(oscFreq);
      banks[0][currentVoice].step(sampleTime);

      currentVoice++;
      i += c;
    }

    currentVoice = 0;
    float outputMono = 0;
    float outputLeft = 0;
    float outputRight = 0;

    for (uint8_t i = 0, c = 0; i < NUM_OSCILLATORS; ) {
      float_4 shaped;
      switch (waveShape) {
        case SIN_WAVESHAPE :
          shaped = banks[0][currentVoice].sin();
          break;
        case TRIANGLE_WAVESHAPE :
          shaped = banks[0][currentVoice].tri();
          break;
        case SAWTOOTH_WAVESHAPE :
          shaped = banks[0][currentVoice].saw();
          break;
        case SQUARE_WAVESHAPE :
          shaped = banks[0][currentVoice].sqr();
          break;
        case RECTANGLE_WAVESHAPE :
          shaped = banks[0][currentVoice].rect();
          break;
      }

      for (c = 0; c < 4 && i + c < NUM_OSCILLATORS; c++) {
        if (i + c < numOscillators[currentBank]) {
          if (numOscillators[currentBank]) {
            int8_t magnitudeVoice = (i + c + voiceShift) % numOscillators[currentBank];
            if (magnitudeVoice < 0) {
              magnitudeVoice += numOscillators[currentBank];
            }
            currentVoiceOutput[i+c] = (shaped[c] * volumes[magnitudeVoice]);
          } else {
            currentVoiceOutput[i+c] = (shaped[c] * volumes[(i + c)]);
          }
        }
      }
      currentVoice++;
      i += c;
    }

    for (uint8_t i = 0; i < NUM_OSCILLATORS && i < numOscillators[currentBank]; i++) {
      if(ringModulationActive) {        
        float rmValue = interpolate(currentVoiceOutput[i],ringModulator.processModel(currentVoiceOutput[i],currentVoiceOutput[rmMatrix[i]]),rmMix[i],0.0f,1.0f);
        outputMono += rmValue;
        outputLeft += rmValue * std::max(1.0 - panning[i],0.0) ;
        outputRight += rmValue * std::max(panning[i]+1.0,1.0);
      } else {
        outputMono += currentVoiceOutput[i];
        outputLeft += currentVoiceOutput[i] * std::max(1.0 - panning[i],0.0);
        outputRight += currentVoiceOutput[i] * std::max(panning[i]+1.0,1.0);
      }
    }

    BankOutput bankOutput;
    bankOutput.outputMono = outputMono;
    bankOutput.outputLeft = outputLeft;
    bankOutput.outputRight = outputRight;

    return bankOutput;
  }


  uint8_t numOscillators[2];
  Oscillator<float_4> banks[2][NUM_OSCILLATORS / 4];
  RingModulator ringModulator;
  bool ringModulationActive;
  float currentFrequencies[NUM_OSCILLATORS]{0};
  float currentVolumes[NUM_OSCILLATORS]{0};
  float currentVoiceOutput[NUM_OSCILLATORS];
  uint16_t blurAmount;
  uint16_t currentBlur;
  float oldFrequencies[NUM_OSCILLATORS]{0};
  float oldVolumes[NUM_OSCILLATORS]{0};
  uint8_t currentBank;
  int8_t voiceShift = 0;
  float frequencyModulation[NUM_OSCILLATORS + 4]{0};
  uint8_t rmMatrix[NUM_OSCILLATORS] {0};
  float rmMix[NUM_OSCILLATORS]{0};
};
