#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <cstdio>
#include "../Interpolate.hpp"
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

    freqDiffThreshold = binSize * 4;
    //freqDiffThreshold = binSize;

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


  //Skip Band 0 as that is DC offset
  void topMorphedN(uint16_t count, uint16_t size1, uint16_t size2, float *inMagData1, float *inPhaseData1, float *inMagData2, float *inPhaseData2, float *morphData, Result *outData, FFTSortMode mode) {

    uint16_t morphBinWidth = size / 2 / count;

    uint8_t b1n1SizeAdjust = size / size1;
    uint8_t b1n2SizeAdjust = size / size2;

    //fprintf(stderr, "%hu , %hu  %hu\n", morphBinWidth, b1n1SizeAdjust,b1n2SizeAdjust);

    freqDiffThreshold = sampleRate / float(size) * 2;
    float binSize1 = sampleRate / float(size1); 
    float binSize2 = sampleRate / float(size2); 

    //Make sure we start fresh
    for (uint16_t c = 0; c < count; c++) {
      outData[c].frequency = 0;
      outData[c].magnitude = 0;
    }

    // minimum magnitude for replacement
    float minMagnitude = std::numeric_limits<double>::infinity();
    inMagData1[0] = -minMagnitude;
    inMagData2[0] = -minMagnitude;

    for (uint16_t i = 0; i <= size / 2; i++) { 
      //Use phases shift to alter frequency
      float freqAdjustment1 = binSize1 * inPhaseData1[i/b1n1SizeAdjust] / M_PI;
      float freqAdjustment2 = binSize2 * inPhaseData2[i/b1n2SizeAdjust] / M_PI;

      float freq1 = (i * binSize1) + freqAdjustment1;
      float freq2 = (i * binSize2) + freqAdjustment2;
      float magnitude1 = inMagData1[i/b1n1SizeAdjust];
      float magnitude2 = inMagData2[i/b1n2SizeAdjust];

      float freq = interpolate(freq1,freq2,morphData[i / morphBinWidth],0.0f,1.0f);
      float magnitude = interpolate(magnitude1,magnitude2,morphData[i / morphBinWidth],0.0f,1.0f);


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
