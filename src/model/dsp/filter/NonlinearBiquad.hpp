/**
 * NonlinearBiquad.hpp
 * Author: Jatin Chowdhury
 * Email: jatin@ccrma.stanford.edu
 * 
 * Modifies Nigel Redmon's "Biquad" class
 * to include three nonlinear configurations:
 *   1. Nonlinear statefulness
 *   2. Nonlinear Feedback
 *   3. A combination of states 1 and 2
 * 
 * For more information on the DSP structures,
 * see: https://dafx2020.mdw.ac.at/proceedings/papers/DAFx2020_paper_3.pdf
 */

#include "Biquad.hpp"
//#include "NonlinearFunctions.hpp"

// enum NLType {
//     NLBQ_NONE,    // plain biquad
//     NLBQ_NLState, // apply nonlinearities to states
//     NLBQ_NLFB,    // apply nonlinearities to feedback paths
//     NLBQ_ALL,     // apply nonlinearities to both states and feedback paths
// };

template <typename T> class NonlinearBiquad : public Biquad<T> {
public:
    NonlinearBiquad() = default;

    NonlinearBiquad(int type, T Fc, T Q, T peakGainDB) :
        Biquad<T>(type, Fc, Q, peakGainDB) {}

    //void setNLBiquad(int type, T Fc, T Q, T drive, T peakGainDB) {
    void setFilterParameters(int type, T Fc, T Q, T drive, T peakGainDB) override {
        this->drive = drive;
        this->setBiquad(type,Fc,Q,peakGainDB);
    }

    void setNonLinearType (NLType type) override {
        nlType = type;

        switch(type) {
            case NLBQ_ALL:
                processFunc = &NonlinearBiquad::process_ALL;
                return;

            case NLBQ_NLFB:
                processFunc = &NonlinearBiquad::process_NLFB;
                return;

            case NLBQ_NLState:
                processFunc = &NonlinearBiquad::process_NLState;
                return;
            case NLBQ_NONE:
            default:
                processFunc = &NonlinearBiquad::process_NONE;
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
        T out = in * this->a0 + this->z1;
        this->z1 = in * this->a1 + this->z2 - this->b1 * out;
        this->z2 = in * this->a2 - this->b2 * out;
        return out;
    }

    T process_NLState(T in) {
      T out = in * this->a0 + this->z1;
      this->z1 = nonlinearity(in * this->a1 + this->z2 - this->b1 * out);
      this->z2 = nonlinearity(in * this->a2 - this->b2 * out);
      return out;
    }

    T process_NLFB(T in) {
      T out = in * this->a0 + this->z1;
      T outNL = nonlinearity(out);
      this->z1 = in * this->a1 + this->z2 - this->b1 * outNL;
      this->z2 = in * this->a2 - this->b2 * outNL;
      return out;
    }

    T process_ALL(T in) {
      T out = in * this->a0 + this->z1;
      T outNL = nonlinearity(out);
      this->z1 = nonlinearity(in * this->a1 + this->z2 - this->b1 * outNL);
      this->z2 = nonlinearity(in * this->a2 - this->b2 * outNL);
      return out;
    }

    inline T nonlinearity(T x) {
        return (this->nlFunc)(x,drive);
    }


    using ProcessFunc = T (NonlinearBiquad::*) (T);
    using NonlinearFunc = T (*) (T, T);
    
    ProcessFunc processFunc = &NonlinearBiquad::process;

    T frequencyResponse(T frequency) override {
        T w = 2.0*M_PI*frequency;

        // copy coefficients
        T a0_prime = this->a0;
        T a1_prime = this->a1;
        T a2_prime = this->a2;
        T b1_prime = this->b1;
        T b2_prime = this->b2;

        // adjust coefficients for nonlinear function
        if(nlType == NLType::NLBQ_NLState) {
            T g = clipDeriv(drive / (T) 10);
            a1_prime *= g;
            b1_prime *= g;
            a2_prime *= g * g;
            b2_prime *= g * g;
        }
        else if(nlType == NLType::NLBQ_NLFB) {
            T g = clipDeriv(drive / (T) 10);
            b1_prime *= g;
            b2_prime *= g;
        }
        else if(nlType == NLType::NLBQ_ALL) {
            T g = clipDeriv(drive / (T) 10);
            a1_prime *= g;
            b1_prime *= g * g;
            a2_prime *= g * g;
            b2_prime *= g * g * g;
        }

        T numerator = a0_prime*a0_prime + a1_prime*a1_prime + a2_prime*a2_prime + 2.0*(a0_prime*a1_prime + a1_prime*a2_prime)*cos(w) + 2.0*a0_prime*a2_prime*cos(2.0*w);
        T denominator = 1.0 + b1_prime*b1_prime + b2_prime*b2_prime + 2.0*(b1_prime + b1_prime*b2_prime)*cos(w) + 2.0*b2_prime*cos(2.0*w);
        T magnitude = sqrt(numerator / denominator);

        return magnitude;
    }

private:
    NonlinearFunc nlFunc = &cubicSoftClip;

    NLType nlType = NLType::NLBQ_NONE;
    T drive = 1.0;
};
