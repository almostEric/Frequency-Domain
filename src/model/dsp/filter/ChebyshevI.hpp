//
//  ChebyshevI.h
#pragma once

//#include "Filter.hpp"

#include <cmath>



enum {
  c1_type_lowpass = 0,
  c1_type_highpass,
  c1_type_bandpass,
  c1_type_notch,
  c1_type_peak,
  c1_type_lowshelf,
  c1_type_highshelf,
  c1_type_allpass
};

template <typename T> class ChebyshevI : public Filter<T> {
public:
  ChebyshevI() {
    type = c1_type_lowpass;
    a0 = 1.0;
    a1 = a2 = b1 = b2 = 0.0;
    Fc = 0.50;
    rippleDB = 0.1; //10% for now
    peakGain = 0.0;
    z1 = z2 = 0.0;

    count = 0;
  }

  ChebyshevI(int type, T Fc, T rippleDB, T peakGainDB) {
    setFilterParameters(type, Fc, rippleDB, 0.1, peakGainDB);
    count = 0;
    z1 = z2 = 0.0;
  }

  virtual ~ChebyshevI() { }

  void setType(int type) {
    this->type = type;
  }

  void setRippledB(T rippleDB) {
    this->rippleDB = rippleDB;
  }

  void setFc(T Fc) {
    this->Fc = Fc;
  }

  void setPeakGain(T peakGainDB) {
    this->peakGain = peakGainDB;
  }


  //void setChebyshevI(int type, T Fc, T rippleDB, T peakGainDB) {
  void setFilterParameters(int type, T Fc, T Q, T drive, T peakGainDB) override {
    this->type = type;
    this->rippleDB = Q;
    this->Fc = Fc;
    setPeakGain(peakGainDB);
    calcChebyshevI();    
  }

  void setNonLinearType (NLType type) override {
  }

  void setNonLinearFunction(NLFunction nlfunction) override {
  }

  T process(T in) override {

    T out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
if(count < 100) {
    fprintf(stderr, "i:%i in:%f a0:%f z1:%f z2:%f  out:%f     a1:%f a2:%f  b1:%f b2:%f  \n",count, in,a0,z1,z2,out,a1,a2,b1,b2);
    count++;
}
    return out;
  }

  void calcChebyshevI() {
    T norm;
    // T V = pow(10, fabs(peakGain) / 20.0);
    // T K = tan(M_PI * Fc);

T rp = -cos(M_PI/4);
T ip = sin(M_PI/4);
//
// Warp from a circle to an ellipse
if (rippleDB > 0) {
    T es = sqrt( pow((1.0 / (1.0-rippleDB)),2)-1.0 );
    T vx = (1.0/2.0) * log( (1/es) + sqrt( (1/(es*es)) + 1) );
    T kx = (1.0/2.0) * log( (1/es) + sqrt( (1/(es*es)) - 1) );
    kx = cosh(kx);
    rp = rp * sinh(vx) / kx;
    ip = ip * cosh(vx) / kx;
}

// fprintf(stderr, "rp:%f ip:%f  \n",rp,ip);

//
//s-domain to z-domain conversion
T t = 2.0 * tan(0.5);
T w = 2.0*M_PI*Fc;
T m = rp*rp + ip*ip;
T d = 4.0 - 4.0*rp*t + m*t*t;
T x0 = t*t/d;
T x1 = 2.0*t*t/d;
T x2 = t*t/d;
T y1 = (8.0 - 2.0*m*t*t)/d;
T y2 = (-4.0 - 4.0*rp*t - m*t*t)/d;
T k = 0.0;
//
// LP TO LP, or LP TO HP transform
if (type == c1_type_highpass) {
    k = -cos(w/2.0 + 0.5) / cos(w/2.0 - 0.5);   
} else if (type == c1_type_lowpass) {
    k = sin(0.5 - w/2.0) / sin(0.5 + w/2.0);  
} 
d = 1.0 + y1*k - y2*k*k;
a0 = (x0 - x1*k + x2*k*k)/d;
a1 = (-2.0*x0*k + x1 + x1*k*k - 2.0*x2*k)/d;
a2 = (x0*k*k - x1*k + x2)/d;
b1 = (2.0*k + y1 + y1*k*k - 2.0*y2*k)/d;
b2 = (-(k*k) - y1*k + y2)/d;
if (type == c1_type_highpass) {
    a1 = -a1;
    b1 = -b1;
} 


a0= 6.362307E-01;
a1= -1.272461E+00;
a2= 6.362307E-01;
b1= 1.125379E+00 ;
b2= -4.195440E-01;
// T sa = a0 + a1 + a2;
// T sb = b1 + b2;
// T gain = sa / (1-sb);
// fprintf(stderr, "gain:%f   \n",gain);

// a0 /= gain;
// a1 /= gain;
// a2 /= gain;



//fprintf(stderr, "a0:%f a1:%f a2:%f   b1:%f b2:%f  \n",a0,a1,a2,b1,b2);

    // switch (this->type) {
    // case c1_type_lowpass:
    //   norm = 1 / (1 + K / Q + K * K);
    //   a0 = K * K * norm;
    //   a1 = 2 * a0;
    //   a2 = a0;
    //   b1 = 2 * (K * K - 1) * norm;
    //   b2 = (1 - K / Q + K * K) * norm;
    //   break;

    // case c1_type_highpass:
    //   norm = 1 / (1 + K / Q + K * K);
    //   a0 = 1 * norm;
    //   a1 = -2 * a0;
    //   a2 = a0;
    //   b1 = 2 * (K * K - 1) * norm;
    //   b2 = (1 - K / Q + K * K) * norm;
    //   break;

    // case c1_type_bandpass:
    //   norm = 1 / (1 + K / Q + K * K);
    //   a0 = K / Q * norm;
    //   a1 = 0;
    //   a2 = -a0;
    //   b1 = 2 * (K * K - 1) * norm;
    //   b2 = (1 - K / Q + K * K) * norm;
    //   break;

    // case c1_type_notch:
    //   norm = 1 / (1 + K / Q + K * K);
    //   a0 = (1 + K * K) * norm;
    //   a1 = 2 * (K * K - 1) * norm;
    //   a2 = a0;
    //   b1 = a1;
    //   b2 = (1 - K / Q + K * K) * norm;
    //   break;

    // case c1_type_peak:
    //   if (peakGain >= 0.0) { // boost
    //     norm = 1 / (1 + 1 / Q * K + K * K);
    //     a0 = (1 + V / Q * K + K * K) * norm;
    //     a1 = 2 * (K * K - 1) * norm;
    //     a2 = (1 - V / Q * K + K * K) * norm;
    //     b1 = a1;
    //     b2 = (1 - 1 / Q * K + K * K) * norm;
    //   } else { // cut
    //     norm = 1 / (1 + V / Q * K + K * K);
    //     a0 = (1 + 1 / Q * K + K * K) * norm;
    //     a1 = 2 * (K * K - 1) * norm;
    //     a2 = (1 - 1 / Q * K + K * K) * norm;
    //     b1 = a1;
    //     b2 = (1 - V / Q * K + K * K) * norm;
    //   }
    //   break;
    // case c1_type_lowshelf:
    //   if (peakGain >= 0.0) { // boost
    //     norm = 1 / (1 + sqrt(2) * K + K * K);
    //     a0 = (1 + sqrt(2 * V) * K + V * K * K) * norm;
    //     a1 = 2 * (V * K * K - 1) * norm;
    //     a2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
    //     b1 = 2 * (K * K - 1) * norm;
    //     b2 = (1 - sqrt(2) * K + K * K) * norm;
    //   } else { // cut
    //     norm = 1 / (1 + sqrt(2 * V) * K + V * K * K);
    //     a0 = (1 + sqrt(2) * K + K * K) * norm;
    //     a1 = 2 * (K * K - 1) * norm;
    //     a2 = (1 - sqrt(2) * K + K * K) * norm;
    //     b1 = 2 * (V * K * K - 1) * norm;
    //     b2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
    //   }
    //   break;
    // case c1_type_highshelf:
    //   if (peakGain >= 0.0) { // boost
    //     norm = 1 / (1 + sqrt(2) * K + K * K);
    //     a0 = (V + sqrt(2 * V) * K + K * K) * norm;
    //     a1 = 2 * (K * K - V) * norm;
    //     a2 = (V - sqrt(2 * V) * K + K * K) * norm;
    //     b1 = 2 * (K * K - 1) * norm;
    //     b2 = (1 - sqrt(2) * K + K * K) * norm;
    //   } else { // cut
    //     norm = 1 / (V + sqrt(2 * V) * K + K * K);
    //     a0 = (1 + sqrt(2) * K + K * K) * norm;
    //     a1 = 2 * (K * K - 1) * norm;
    //     a2 = (1 - sqrt(2) * K + K * K) * norm;
    //     b1 = 2 * (K * K - V) * norm;
    //     b2 = (V - sqrt(2 * V) * K + K * K) * norm;
    //   }
    //   break;
    // case c1_type_allpass:
    //   T alpha = sin(Fc) / 2.0 * Q;
    //   T cs = cos(Fc);
    //   norm = 1.0 / (1.0 + alpha);
    //   b1 = -2.0 * cs * norm;
    //   b2 = (1.0 - alpha) * norm;
    //   a0 = (1.0 - alpha) * norm;
    //   a1 = -2.0 * cs * norm;
    //   a2 = (1.0 + alpha) * norm;

    //   break;
    // }

    return;
  }

  virtual T frequencyResponse(T frequency) override {
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
  T Fc, rippleDB, peakGain;
  T z1, z2;

  int count;
};
