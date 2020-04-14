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
