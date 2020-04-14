#pragma once

#include "../fft/kiss_fft.h"
#define KISS_FFT

struct FFT {
  FFT(uint16_t size) {
    in = (kiss_fft_cpx *) malloc(sizeof(kiss_fft_cpx) * size);
    out = (kiss_fft_cpx *) malloc(sizeof(kiss_fft_cpx) * size);
    memset(in, 0, sizeof(kiss_fft_cpx) * size);
    memset(out, 0, sizeof(kiss_fft_cpx) * size);

    size_t memSize = 1024 + sizeof(kiss_fft_cpx) * (size - 1);
    fwdWorkMemory = malloc(memSize);
    forward_cfg = kiss_fft_alloc(size, false, fwdWorkMemory, &memSize);

    //forward_cfg = kiss_fft_alloc(size, false, NULL, NULL);

    memSize = 1024 + sizeof(kiss_fft_cpx) * (size - 1);
    bwdWorkMemory = malloc(memSize);
    backward_cfg = kiss_fft_alloc(size, true, bwdWorkMemory, &memSize);
    this->size = size;
    scale = 1.0f / size;
  }

  ~FFT() {
    free(in);
    free(out);
    //free(forward_cfg);
    free(fwdWorkMemory);
    free(bwdWorkMemory);
  }

  void fft(float *inData) {
    for (uint32_t i = 0; i < size; i++) {
      in[i].r = inData[i];
      in[i].i = 0;
    }

    kiss_fft(forward_cfg, in, out);
  }

  void ifft(float *outData) {
    kiss_fft(backward_cfg, in, out);

    for (uint32_t i = 0; i < size; i++) {
      outData[i] = out[i].r * scale;
    }
  }

  kiss_fft_cpx *out;
  kiss_fft_cpx *in;
  kiss_fft_cfg forward_cfg;
  kiss_fft_cfg backward_cfg;
  uint16_t size;
  void *fwdWorkMemory;
  void *bwdWorkMemory;
  float scale;
};
