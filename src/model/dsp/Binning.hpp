#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <cstdio>
//#include "FFT.hpp"

#define NUM_SORT_MODES 8

enum FFTSortMode {
  TOP_SORT,
  TOP_MAGNITUDE_SORT,
  TOP_REVERSE_SORT,
  TOP_MAGNITUDE_REVERSE_SORT
};

struct Result {
  float magnitude;
  float frequency;
  bool operator<(const Result& a) const {
    return magnitude > a.magnitude;
  }
};


struct Binning {
  Binning(uint16_t size, float sampleRate) {
    this->size = size;
    this->sampleRate = sampleRate;
  }


  //Skip Band 0 as that is DC offset
  void topN(uint16_t count, float *inMagData, float *inPhaseData, Result *outData, FFTSortMode mode) {

    float binSize = sampleRate / float(size); 

    freqDiffThreshold = binSize * 2;

    //Make sure we start fresh
    for (uint16_t c = 0; c < count; c++) {
      outData[c].frequency = 0;
      outData[c].magnitude = 0;
    }


    
    // minimum magnitude for replacement
    float minMagnitude = std::numeric_limits<double>::infinity();
    inMagData[0] = -minMagnitude;

    for (uint16_t i = 0; i <= size / 2; i++) { 
      //Use phases shift to alter frequency
      float freqAdjustment = binSize * inPhaseData[i] / M_PI;
      //freqAdjustment = 0;
      float freq = (i * binSize) + freqAdjustment;

      float magnitude = inMagData[i];

      bool newFrequency = true;

      //Check for duplicate frequencies
      for (uint16_t c = 0; c < count; c++) {
        if (abs(outData[c].frequency - freq) < freqDiffThreshold) {
          newFrequency = false;
          if(magnitude > outData[c].magnitude) {
            outData[c].frequency = freq;
            outData[c].magnitude = magnitude;
          }
          break;            
        }
      }

      if(newFrequency) {
        if (i < count) {
          // if within the minumum count, start with those
          if (magnitude < minMagnitude) {
            minMagnitude = magnitude;
          }
          outData[i].frequency = freq;
          outData[i].magnitude = magnitude;
        } else if (magnitude > minMagnitude) {
          // otherwise find the lowest slot and replace it
          for (uint16_t c = 0; c < count; c++) {
            if (outData[c].magnitude == minMagnitude) {
              outData[c].frequency = freq;
              outData[c].magnitude = magnitude;
              break;
            }
          }
          //Find new minimum
          minMagnitude = std::numeric_limits<double>::infinity(); // reset 
          for (uint16_t c = 0; c < count; c++) {
            if (outData[c].magnitude < minMagnitude) {
              minMagnitude = outData[c].magnitude;
            }
          }
        }
      }
    }
  

    std::sort(outData, outData + count);

    if (mode == TOP_MAGNITUDE_REVERSE_SORT || mode == TOP_REVERSE_SORT) {
       std::reverse(outData, outData + count);
    }

    if (mode == TOP_MAGNITUDE_REVERSE_SORT || mode == TOP_MAGNITUDE_SORT) {
      for (uint8_t i = 0; i < count / 2; i++) {
        float swap = outData[i].magnitude;
        outData[i].magnitude = outData[count - 1 - i].magnitude;
        outData[count - 1 - i].magnitude = swap;
      }
    }
  }

  uint16_t size;
  float sampleRate;
  float freqDiffThreshold = 25;


};

#if defined(PFFT_FFT) || defined(KISS_FFT)
#ifdef PFFT_FFT
static void sortedResults(Complex *fftData, uint16_t fftSize, Result *outData, uint16_t outCount, float sampleRate, FFTSortMode mode) {
#else
static void sortedResults(kiss_fft_cpx *fftData, uint16_t fftSize, Result *outData, uint16_t outCount, float sampleRate, FFTSortMode mode) {
#endif
  
  // minimum magnitude for replacement
  float minMagnitude = -std::numeric_limits<double>::infinity();

  for (uint16_t i = 0; i < fftSize / 2; i++) {
    float freq = (i * sampleRate) / float(fftSize);
    float magnitude = sqrt(fftData[i].r * fftData[i].r + fftData[i].i * fftData[i].i);

    if (i < outCount) {
        // if within the minumum count, start with those
        if (magnitude < minMagnitude) {
          minMagnitude = magnitude;
        }
        outData[i].frequency = freq;
        outData[i].magnitude = magnitude;
      } else if (magnitude > minMagnitude) {
        // otherwise find the lowest slot and replace it
        for (uint16_t c = 0; c < outCount; c++) {
          if (outData[c].magnitude == minMagnitude) {
            outData[c].frequency = freq;
            outData[c].magnitude = magnitude;
            break;
          }
        }
        //Find new minimum
        minMagnitude = std::numeric_limits<double>::infinity(); // reset 
        for (uint16_t c = 0; c < outCount; c++) {
          if (outData[c].magnitude < minMagnitude) {
            minMagnitude = outData[c].magnitude;
          }
        }
      }
  }


  std::sort(outData, outData + outCount);

  if (mode == TOP_MAGNITUDE_REVERSE_SORT || mode == TOP_REVERSE_SORT) {
    std::reverse(outData, outData + outCount);
  }

  if (mode == TOP_MAGNITUDE_REVERSE_SORT || mode == TOP_MAGNITUDE_SORT) {
    for (uint8_t i = 0; i < outCount / 2; i++) {
      float swap = outData[i].magnitude;
      outData[i].magnitude = outData[outCount - 1 - i].magnitude;
      outData[outCount - 1 - i].magnitude = swap;
    }
  }
}
#endif
