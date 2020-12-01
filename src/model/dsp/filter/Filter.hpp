#pragma once

#include "NonlinearFunctions.hpp"

enum NLType {
    NLBQ_NONE,    // plain filter
    NLBQ_NLState, // apply nonlinearities to states
    NLBQ_NLFB,    // apply nonlinearities to feedback paths
    NLBQ_ALL,     // apply nonlinearities to both states and feedback paths
};


enum NLFunction {
    NLFC_CUBIC_SOFT_CLIP,    // cubic soft clipping
    NLFC_HARD_CLIP,    // hard clipping
    NLFC_TANH_CLIP,    // tanh  clipping
    NLFC_DOUBLE_SOFT_CLIP,    // double soft clipping
};


template <typename T> class Filter{

public:

    virtual void setNonLinearType (NLType type) {}

    virtual void setNonLinearFunction(NLFunction nlfunction) {}

    virtual void setFilterParameters(int type, T Fc, T Q, T drive, T peakGainDB) {}
    
    virtual void setFilterParameters(T Fc, T t60Samp, T drive, T phase) {}

    virtual void setFilterParameters(int type, T feedforwardAmount, T feedbackAmount, T feedforwardGain, T feedbackGain, T drive) {}

    virtual T process(T in);

    virtual T frequencyResponse(T frequency);

};