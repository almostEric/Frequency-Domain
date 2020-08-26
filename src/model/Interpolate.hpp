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
