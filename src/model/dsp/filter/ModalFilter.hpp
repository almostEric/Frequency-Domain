#pragma once

/**
 * ModalFilter.hpp
 * Author: Jatin Chowdhury
 * Email: jatin@ccrma.stanford.edu
 * 
 * Implements a modal filter using the Max Mathews Phasor Filter
 * For more information, see: https://ccrma.stanford.edu/~jos/smac03maxjos/
 */

#include "Filter.hpp"

template <typename T> class ModalFilter : public Filter<T> {
public:
    ModalFilter() = default;

    ModalFilter(T Fc, T t60Samp, T drive, T phase) {
        setFilterParameters (Fc, t60Samp, drive, phase);
    }

    void setFilterParameters(T Fc, T t60Samp, T drive, T phase) override {
        A = std::polar(drive, phase);
        auto decayFactor = std::pow((T) 0.001, (T) 1.0 / t60Samp);
        oscCoef = std::exp(jImag * (T) 2.0 * M_PI * Fc);
        filtCoef = decayFactor * oscCoef;
    }

    T process(T in) override {
        auto y = A * in + filtCoef * y1;

        y1 = y;
        return std::imag (y);
    }

    T frequencyResponse(T frequency) override {
        T w = 2.0*M_PI*frequency;
        auto denom = std::abs(std::complex<T> (1) - filtCoef * std::exp(-jImag * w));
        return std::abs(A) / denom / (T) 2.0;
    }

private:
    // filter coefficients
    std::complex<T> filtCoef { 0 };
    std::complex<T> oscCoef { 0 };
    T decayCoef = static_cast<T> (0);
    std::complex<T> A { 1 };

    // filter state
    std::complex<T> y1 { 0 };

    // constant for the square root of -1
    static constexpr std::complex<T> jImag = std::complex<T> { (T) 0, (T) 1 };
};

// because std::complex<T> is pass-by-reference, we need to declare it here
template<typename T> 
constexpr std::complex<T> ModalFilter<T>::jImag;
