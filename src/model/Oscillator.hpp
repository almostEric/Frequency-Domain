#pragma once

using simd::float_4;

enum OscillatorWaveShapes {
    SIN_WAVESHAPE,
    TRIANGLE_WAVESHAPE,
    SAWTOOTH_WAVESHAPE,
    SQUARE_WAVESHAPE,
    RECTANGLE_WAVESHAPE,
};

template <typename T>
struct Oscillator {
  Oscillator() {
    reset();
  }

  void reset() {
    bias = -1.0;
    frequency = 0;
    scale = 2.0;
    delta = 0.5;
    pw50 = 0.5;
    pw25 = 0.25;
  }

  void setFrequency(T frequency) {
    this->frequency = frequency;
  }

  void setPhase(T phase) {
    this->delta = phase;
  }

  void setPw(T pw) {
    this->pw = pw;
  }

  void resync() {
    this->delta = 0.5;
  }

  void setDelta(T delta) {
    this->delta = delta;
  }

  void step(T sampleTime) {
    T frequencyTime = frequency * sampleTime;

    delta += frequencyTime;
    if (delta[0] >= 1.0f) {
      delta[0] -= 1.0f;
    }
    if (delta[1] >= 1.0f) {
      delta[1] -= 1.0f;
    }
    if (delta[2] >= 1.0f) {
      delta[2] -= 1.0f;
    }
    if (delta[3] >= 1.0f) {
      delta[3] -= 1.0f;
    }
  }

  T saw() {
    return (delta * scale) + bias;
  }

  T tri() {
    return (simd::ifelse(delta > 0.5, 2.0 * delta, 2.0 * (1.0 - delta)) * scale) + (bias * 3.0);
  }

  T sin() {
    return -simd::sin(2.0 * M_PI * delta);
  }

  T sqr() {
    return (simd::ifelse(delta < pw50, 1.0, 0.0) * scale) + bias;
  }

  T rect() {
    return (simd::ifelse(delta < pw25, 1.0, 0.0) * scale) + bias;
  }

  // percentage through the current waveform
  T frequency;
  T scale;
  // amount to add or subtract from the final output
  T bias;

  T delta;
  T pw50;
  T pw25;
};
