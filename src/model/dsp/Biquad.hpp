//
//  Biquad.h
//
//  Created by Nigel Redmon on 11/24/12
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the Biquad code:
//  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code
//  for your own purposes, free or commercial.
//

// NOTE: Some local modifications were made that changes some behavior
// from the above article - most notably: calcBiquad() was moved from
// protected to public, and must be called manually on any changes before
// process() is called.
#pragma once

#include <cmath>
#include <simd/vector.hpp>
#include <simd/sse_mathfun.h>
#include <simd/sse_mathfun_extension.h>

using rack::simd::float_4;


enum {
  bq_type_lowpass = 0,
  bq_type_highpass,
  bq_type_bandpass,
  bq_type_notch,
  bq_type_peak,
  bq_type_lowshelf,
  bq_type_highshelf,
  bq_type_allpass
};

template <typename T> class Biquad {
public:
  Biquad() {
    type = bq_type_lowpass;
    a0 = 1.0;
    a1 = a2 = b1 = b2 = 0.0;
    Fc = 0.50;
    Q = 0.707;
    peakGain = 0.0;
    z1 = z2 = 0.0;
  }

  Biquad(int type, T Fc, T Q, T peakGainDB) {
    setBiquad(type, Fc, Q, peakGainDB);
    z1 = z2 = 0.0;
  }

  virtual ~Biquad() { }

  void setType(int type) {
    this->type = type;
  }

  void setQ(T Q) {
    this->Q = Q;
  }

  void setFc(T Fc) {
    this->Fc = Fc;
  }

  void setPeakGain(T peakGainDB) {
    this->peakGain = peakGainDB;
  }

  void setBiquad(int type, T Fc, T Q, T peakGainDB) {
    this->type = type;
    this->Q = Q;
    this->Fc = Fc;
    setPeakGain(peakGainDB);
    calcBiquad();
    
  }

  T process(T in) {
    T out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return out;
  }

  void calcBiquad() {
    T norm;
    T V = pow(10, fabs(peakGain) / 20.0);
    T K = tan(M_PI * Fc);
    switch (this->type) {
    case bq_type_lowpass:
      norm = 1 / (1 + K / Q + K * K);
      a0 = K * K * norm;
      a1 = 2 * a0;
      a2 = a0;
      b1 = 2 * (K * K - 1) * norm;
      b2 = (1 - K / Q + K * K) * norm;
      break;

    case bq_type_highpass:
      norm = 1 / (1 + K / Q + K * K);
      a0 = 1 * norm;
      a1 = -2 * a0;
      a2 = a0;
      b1 = 2 * (K * K - 1) * norm;
      b2 = (1 - K / Q + K * K) * norm;
      break;

    case bq_type_bandpass:
      norm = 1 / (1 + K / Q + K * K);
      a0 = K / Q * norm;
      a1 = 0;
      a2 = -a0;
      b1 = 2 * (K * K - 1) * norm;
      b2 = (1 - K / Q + K * K) * norm;
      break;

    case bq_type_notch:
      norm = 1 / (1 + K / Q + K * K);
      a0 = (1 + K * K) * norm;
      a1 = 2 * (K * K - 1) * norm;
      a2 = a0;
      b1 = a1;
      b2 = (1 - K / Q + K * K) * norm;

      //fprintf(stderr, "BoR -- Notch Coefficients Calculated  \n");
      break;

    case bq_type_peak:
      if (peakGain >= 0.0) { // boost
        norm = 1 / (1 + 1 / Q * K + K * K);
        a0 = (1 + V / Q * K + K * K) * norm;
        a1 = 2 * (K * K - 1) * norm;
        a2 = (1 - V / Q * K + K * K) * norm;
        b1 = a1;
        b2 = (1 - 1 / Q * K + K * K) * norm;
      } else { // cut
        norm = 1 / (1 + V / Q * K + K * K);
        a0 = (1 + 1 / Q * K + K * K) * norm;
        a1 = 2 * (K * K - 1) * norm;
        a2 = (1 - 1 / Q * K + K * K) * norm;
        b1 = a1;
        b2 = (1 - V / Q * K + K * K) * norm;
      }
      break;
    case bq_type_lowshelf:
      if (peakGain >= 0.0) { // boost
        norm = 1 / (1 + sqrt(2) * K + K * K);
        a0 = (1 + sqrt(2 * V) * K + V * K * K) * norm;
        a1 = 2 * (V * K * K - 1) * norm;
        a2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
        b1 = 2 * (K * K - 1) * norm;
        b2 = (1 - sqrt(2) * K + K * K) * norm;
      } else { // cut
        norm = 1 / (1 + sqrt(2 * V) * K + V * K * K);
        a0 = (1 + sqrt(2) * K + K * K) * norm;
        a1 = 2 * (K * K - 1) * norm;
        a2 = (1 - sqrt(2) * K + K * K) * norm;
        b1 = 2 * (V * K * K - 1) * norm;
        b2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
      }
      break;
    case bq_type_highshelf:
      if (peakGain >= 0.0) { // boost
        norm = 1 / (1 + sqrt(2) * K + K * K);
        a0 = (V + sqrt(2 * V) * K + K * K) * norm;
        a1 = 2 * (K * K - V) * norm;
        a2 = (V - sqrt(2 * V) * K + K * K) * norm;
        b1 = 2 * (K * K - 1) * norm;
        b2 = (1 - sqrt(2) * K + K * K) * norm;
      } else { // cut
        norm = 1 / (V + sqrt(2 * V) * K + K * K);
        a0 = (1 + sqrt(2) * K + K * K) * norm;
        a1 = 2 * (K * K - 1) * norm;
        a2 = (1 - sqrt(2) * K + K * K) * norm;
        b1 = 2 * (K * K - V) * norm;
        b2 = (V - sqrt(2 * V) * K + K * K) * norm;
      }
      break;
    case bq_type_allpass:
      T alpha = sin(Fc) / 2.0 * Q;
      T cs = cos(Fc);
      norm = 1.0 / (1.0 + alpha);
      b1 = -2.0 * cs * norm;
      b2 = (1.0 - alpha) * norm;
      a0 = (1.0 - alpha) * norm;
      a1 = -2.0 * cs * norm;
      a2 = (1.0 + alpha) * norm;

      break;
    }

    return;
  }

  virtual T frequencyResponse(T frequency) {
    T w = 2.0*M_PI*frequency;  
    T numerator = a0*a0 + a1*a1 + a2*a2 + 2.0*(a0*a1 + a1*a2)*cos(w) + 2.0*a0*a2*cos(2.0*w);
    T denominator = 1.0 + b1*b1 + b2*b2 + 2.0*(b1 + b1*b2)*cos(w) + 2.0*b2*cos(2.0*w);
    T magnitude = sqrt(numerator / denominator);

    // magnitude = simd::ifelse(simd::isNan(magnitude),0.0,magnitude);
    

    // fprintf(stderr, "Fc: %F, F:%f w:%f n:%f d:%f m:%f \n",Fc[0],frequency[0],w[0],numerator[0],denominator[0],magnitude[0]);


    return magnitude;
  }

protected:
  int type;
  T a0, a1, a2, b1, b2;
  T Fc, Q, peakGain;
  T z1, z2;
};
