//Comb Filter, based on Julius O Smaith design

#pragma once

//#include "Filter.hpp"

#include <cmath>
#include "../../DelayLine.hpp"
#include <simd/vector.hpp>
#include <simd/sse_mathfun.h>
#include <simd/sse_mathfun_extension.h>

using rack::simd::float_4;



template <typename T> class CombFilter : public Filter<T> {
public:
  CombFilter() {
    feedforwardAmount = 0;
    feedbackAmount = 0;
    feedforwardGain = 0;
    feedbackGain = 0;
  }

  CombFilter(int type, T feedforwardAmount, T feedbackAmount, T feedfowardGain, T feedbackGain) {
    setFilterParameters(type, feedforwardAmount, feedbackAmount, feedfowardGain, feedbackGain, drive);
  }

  virtual ~CombFilter() { }



  void setFilterParameters(int type, T feedforwardAmount, T feedbackAmount, T feedforwardGain, T feedbackGain, T drive) override {
    this->feedforwardAmount = feedforwardAmount;
    this->feedbackAmount = feedbackAmount;
    this->feedforwardGain = feedforwardGain;
    this->feedbackGain = feedbackGain;
    this->drive = drive;
  }

  void setNonLinearType (NLType type) override {
    nlType = type;

    switch(type) {
        case NLBQ_ALL:
            processFunc = &CombFilter::process_ALL;
            return;

        case NLBQ_NLFB:
            processFunc = &CombFilter::process_NLFB;
            return;

        case NLBQ_NLState:
            processFunc = &CombFilter::process_NLState;
            return;
        case NLBQ_NONE:
        default:
            processFunc = &CombFilter::process_NONE;
    }
}

void setNonLinearFunction(NLFunction nlfunction) override {
    switch(nlfunction) {
        case NLFC_DOUBLE_SOFT_CLIP:
            nlFunc = &doubleSoftClip;
            break;
        case NLFC_TANH_CLIP:
            nlFunc = &tanhClip;
            break;
        case NLFC_HARD_CLIP:
            nlFunc = &hardClip;
            break;
        case NLFC_CUBIC_SOFT_CLIP:
        default:
            nlFunc = &cubicSoftClip;
            break;
    }
}

  T process(T in) override {
      return (this->*processFunc)(in);
  }

  T process_NONE(T in) {
    T out;
    fbDelay.write(in);
    out = in + (fbDelay.getNonInterpolatedDelay(feedbackAmount) * feedbackGain) - (ffDelay.getNonInterpolatedDelay(feedforwardAmount) * feedforwardGain);
    ffDelay.write(out);
    return out;
  }

  T process_NLState(T in) {
    T out;
    fbDelay.write(in);
    out = in + (fbDelay.getNonInterpolatedDelay(feedbackAmount) * feedbackGain) - nonlinearity(ffDelay.getNonInterpolatedDelay(feedforwardAmount) * feedforwardGain);
    ffDelay.write(out);
    return out;
  }

  T process_NLFB(T in) {
    T out;
    fbDelay.write(in);
    out = in + nonlinearity(fbDelay.getNonInterpolatedDelay(feedbackAmount) * feedbackGain) - (ffDelay.getNonInterpolatedDelay(feedforwardAmount) * feedforwardGain);
    ffDelay.write(out);
    return out;
  }

  T process_ALL(T in) {
    T out;
    fbDelay.write(in);
    out = in + nonlinearity(fbDelay.getNonInterpolatedDelay(feedbackAmount) * feedbackGain) - nonlinearity(ffDelay.getNonInterpolatedDelay(feedforwardAmount) * feedforwardGain);
    ffDelay.write(out);
    return out;
  }

  inline T nonlinearity(T x) {
      return (this->nlFunc)(x,drive);
  }


  using ProcessFunc = T (CombFilter::*) (T);
  using NonlinearFunc = T (*) (T, T);
  
  ProcessFunc processFunc = &CombFilter::process;

  virtual T frequencyResponse(T frequency) override {
    T w = 2.0*M_PI*frequency;  
    T numerator = 1 + feedbackGain * cos(feedbackAmount * w / 2.0);
    T denominator = 1 + feedforwardGain * cos(feedforwardAmount * w / 2.0);
    T magnitude = numerator / denominator;

    // T magnitude = 1.0;


    return magnitude;
  }

protected:
    T feedforwardAmount,feedforwardGain,feedbackAmount,feedbackGain;
    NonlinearFunc nlFunc = &cubicSoftClip;

    NLType nlType = NLType::NLBQ_NONE;
    T drive = 1.0;
  
  DelayLine<T> fbDelay,ffDelay;

};
