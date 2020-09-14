#pragma once

#define NBR_INTERPOLATE_MODES 2

enum InterpolateMode {
  INTERPOLATE_LINEAR,
  INTERPOLATE_LOGRITHMIC
};


template <typename T>
T interpolate(T input1, T input2, T balance, T low = 0, T high = 100, InterpolateMode mode = INTERPOLATE_LINEAR) {
  T total = (high - low);
  T output = (input1 * ((total - balance) / total)) + (input2 * (balance / total));

  return output;
}


template <typename T>
T bilinearInterpolate(T q11,T q12,T q21,T q22,T x, T y,T low = 0, T high = 1) {

  T total = high - low;

  T fxy1 = ((high-x) / total * q11) + ((x-low) / total * q21);
  T fxy2 = ((high-x) / total * q12) + ((x-low) / total * q22);

  T fx = ((high-y) / total * fxy1) + ((y-low) / total * fxy2); 

  return fx;
}


template <typename T>
T trilinearInterpolate(T c000,T c100,T c010,T c110,T c001,T c101,T c011,T c111, T x, T y,T z, T low = 0, T high = 1) {

  //T total = high - low;

  // T xd = (high - x) / total;
  // T yd = (high - y) / total;
  // T zd = (high - z) / total;

  T c00 = c000*(1-x) + c100*x;
  T c01 = c001*(1-x) + c101*x;
  T c10 = c010*(1-x) + c110*x;
  T c11 = c011*(1-x) + c111*x;
  
  T c0 = c00*(1-y) + c10*y;
  T c1 = c01*(1-y) + c11*y;

  T c = c0*(1-z) + c1*z;

  return c;
}
