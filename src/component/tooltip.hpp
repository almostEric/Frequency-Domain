#pragma once

struct VCOVoice : ParamQuantity {
  std::string getDisplayValueString() override {
    int value = (int)getValue();

    switch (value) {
      case SIN_WAVESHAPE :
        return "Sin";
        break;
      case TRIANGLE_WAVESHAPE :
        return "Triangle";
        break;
      case SAWTOOTH_WAVESHAPE :
        return "Sawtooth";
        break;
      case SQUARE_WAVESHAPE :
        return "Square";
        break;
      case RECTANGLE_WAVESHAPE :
        return "25% Rectangle";
        break;
      default :
        return "?";
        break;
    }
  }
};


// struct NoiseColorName : ParamQuantity {
//   std::string getDisplayValueString() override {
//     int value = (int)getValue();

//     switch (value) {
//       case NOISE_WHITE :
//         return "White";
//         break;
//       case NOISE_PINK :
//         return "Pink";
//         break;
//       case NOISE_RED :
//         return "Red";
//         break;
//       case NOISE_VIOLET :
//         return "Violet";
//         break;
//       case NOISE_GREY :
//         return "Grey";
//         break;
//       case NOISE_BLUE :
//         return "Blackman";
//         break;
//       case NOISE_BLACK :
//         return "Nutall";
//         break;
//       default :
//         return "?";
//         break;
//     }
//   }
// };


struct WindowFunctionName : ParamQuantity {
  std::string getDisplayValueString() override {
    int value = (int)getValue();

    switch (value) {
      case NO_WINDOW_FUNCTION :
        return "None";
        break;
      case TRIANGLE_WINDOW_FUNCTION :
        return "Triangle";
        break;
      case WELCH_WINDOW_FUNCTION :
        return "Welch";
        break;
      case SIN_WINDOW_FUNCTION :
        return "Sin";
        break;
      case HANNING_WINDOW_FUNCTION :
        return "Hanning";
        break;
      case BLACKMAN_WINDOW_FUNCTION :
        return "Blackman";
        break;
      case NUTALL_WINDOW_FUNCTION :
        return "Nutall";
        break;
      case KAISER_WINDOW_FUNCTION :
        return "Kaiser";
        break;
      default :
        return "?";
        break;
    }
  }
};

struct SpectralModeName : ParamQuantity {
  std::string getDisplayValueString() override {
    int value = (int)getValue();

    switch (value) {
      case TOP_SORT:
        return "Normal";
        break;
      case TOP_MAGNITUDE_SORT:
        return "Inverted Magnitude";
        break;
      case TOP_REVERSE_SORT:
        return "Reverse";
        break;
      case TOP_MAGNITUDE_REVERSE_SORT:
        return "Reverse Inverted Magnitude";
        break;
      default:
        return "?";
    }
  }
};
