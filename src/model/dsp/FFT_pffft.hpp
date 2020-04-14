#pragma once

#include <pffft.h>

#define PFFT_FFT

struct Complex {
  float r;
  float i;
};

struct FFT {
  FFT(uint16_t size, float sampleRate) {
    workMemory = new float[size * 2];
    in = new Complex[size];
    out = new Complex[size];
    inMemory = new float[size * 2];
    outMemory = new float[size * 2];

    this->sampleRate = sampleRate;
    this->size = size;

    cfg = pffft_new_setup(size, PFFFT_REAL);
  }

  ~FFT() {
    pffft_destroy_setup(cfg);
    delete[] workMemory;
    delete[] in;
    delete[] out;
    delete[] inMemory;
    delete[] outMemory;
  }

  void fft(float *inData) {
    pffft_transform_ordered(cfg, inData, outMemory, workMemory, PFFFT_FORWARD);
    for (uint32_t i = 0; i < size / 2; i++) {
      out[i].r = outMemory[i];
      out[i].i = outMemory[size / 2 + i];
      in[i].r = inData[i];
      in[i].i = 0;
    }
  }

  void ifft(float *outData) {
    for (uint32_t i = 0; i < size / 2; i++) {
      inMemory[i] = in[i].r;
      inMemory[size / 2 + i] = in[i].i;
    }

    pffft_transform_ordered(cfg, inMemory, outData, workMemory, PFFFT_BACKWARD);

    for (uint32_t i = 0; i < size; i++) {
      out[i].r = outData[i];
      out[i].i = outData[size / 2 + i];
      outData[i] = outData[i] * (1.0f / size);
    }
  }

  PFFFT_Setup *cfg;
  uint16_t size;
  float sampleRate;
  float *workMemory;
  float *outMemory;
  float *inMemory;
  Complex *in;
  Complex *out;
};
