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

enum NLType {
    NLBQ_NONE,    // plain biquad
    NLBQ_NLState, // apply nonlinearities to states
    NLBQ_NLFB,    // apply nonlinearities to feedback paths
    NLBQ_ALL,     // apply nonlinearities to both states and feedback paths
};

template <typename T> class NonlinearBiquad : public Biquad<T> {
public:
    NonlinearBiquad() = default;

    NonlinearBiquad(int type, T Fc, T Q, T peakGainDB) :
        Biquad<T>(type, Fc, Q, peakGainDB) {}

    void setType (NLType type) {
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
            processFunc = &NonlinearBiquad::process;
        }
    }

    T processSample(T in) {
        return (this->*processFunc)(in);
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

    /**
     * Simple cubic soft-clipping nonlinearity.
     * For more information: https://ccrma.stanford.edu/~jos/pasp/Cubic_Soft_Clipper.html
     */
    inline T nonlinearity(T x, T drive = (T) 5) const noexcept {
        x = clamp(drive * x, -1.0f, 1.0f);
        return (x - x*x*x / (T) 3) / drive;
    }

    using ProcessFunc = T (NonlinearBiquad::*) (T);
    ProcessFunc processFunc = &NonlinearBiquad::process;

private:

};
