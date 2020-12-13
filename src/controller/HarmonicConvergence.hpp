#include <cstdint>
#include <vector>

#include "../FrequencyDomain.hpp"
#include "../model/dsp/Binning.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Oscillator.hpp"
#include "../model/Buffer.hpp"
#include "../model/OscillatorBank.hpp"
#include "../model/Cells.hpp"
#include "../model/dsp/FFT.hpp"

using rack::simd::float_4;

#define MAX_VOICE_COUNT 36
#define MAX_FRAMES 4
#define MAX_POLYPHONY 16
#define MAX_BUFFER_SIZE 8192 //2^13

struct HarmonicConvergenceModule : Module {
  enum ParamIds {
    WINDOW_FUNCTION,
    VOICE_WAVEFORM,
    VOICE_COUNT, 
    MIX,
    OCTAVE,
    FREQ_WARP_AMOUNT,
    FREQ_WARP_CENTER,
    FREQ_WARP_USE_FUNDAMENTAL,
    FRAME_SIZE,
    RING_MODULATION,
    VOICE_SHIFT,
    SPECTRAL_MODE,
    MORPH_AMOUNT,
    MORPH_MODE,
    FM_AMOUNT,
    RM_MIX,
    ANALYZE_CENTER,
    ANALYZE_BW,
    FEEDBACK,
    NUM_PARAMS
  };

  enum InputIds {
    INPUT_1,
    INPUT_2,
    FM_SHIFT_X_CV,
    FM_SHIFT_Y_CV,
    FM_AMOUNT_SHIFT_X_CV,
    FM_AMOUNT_SHIFT_Y_CV,
    FM_INPUT_1,
    FM_INPUT_2,
    AM_RM_INPUT_1,
    AM_RM_INPUT_2,
    VOICE_COUNT_CV,
    VOICE_WAVEFORM_CV,
    WINDOW_FUNCTION_CV,
    FRAME_SIZE_CV,
    OCTAVE_CV,
    FREQ_WARP_AMOUNT_CV,
    FREQ_WARP_CENTER_CV,
    MIX_CV,
    RM_SHIFT_X_CV,
    RM_SHIFT_Y_CV,
    RM_MIX_SHIFT_X_CV,
    RM_MIX_SHIFT_Y_CV,
    VOICE_SHIFT_CV,
    MORPH_SHIFT_X_CV,
    MORPH_SHIFT_Y_CV,
    PAN_SHIFT_X_CV,
    PAN_SHIFT_Y_CV,
    SPECTRAL_MODE_CV,
    ANALYZE_CENTER_CV,
    ANALYZE_BW_CV,
    FEEDBACK_CV,
    NUM_INPUTS
  };
  enum OutputIds { OUTPUT_L, OUTPUT_R, DEBUG_OUTPUT, NUM_OUTPUTS };
  enum LightIds {
    RING_MODULATION_ENABLED_LIGHT,
    FREQ_WARP_USE_FUNDAMENTAL_LIGHT = RING_MODULATION_ENABLED_LIGHT + 3,
    MORPH_MODE_LIGHT = FREQ_WARP_USE_FUNDAMENTAL_LIGHT + 3,
    NUM_LIGHTS = MORPH_MODE_LIGHT + 3
  };

  HarmonicConvergenceModule ();
  ~HarmonicConvergenceModule ();

  void process (const ProcessArgs &args) override;
  float paramValue (uint16_t, uint16_t, float, float);
  float lowFreq();
  float highFreq();

  void onReset() override;
  void dataFromJson(json_t *) override;
  json_t *dataToJson() override;


  FFT *fft1;
  FFT *fft2;


  // binning, 36 bins to work with
  Binning *binnings1;
  Result bins1[MAX_VOICE_COUNT] = { { 0, 0, 0 } };
  Binning *binnings2;
  Result bins2[MAX_VOICE_COUNT] = { { 0, 0, 0 } };
  WindowFunction<float> *windowFunction1;
  WindowFunction<float> *windowFunction2;
  dsp::SchmittTrigger rmTrigger,warpUseFundamentalTrigger, morphModeTrigger;


  // 9 x 4 = 36 oscillators
  Oscillator<float_4> oscillators[9];
  OscillatorBank bank;

  // buffer for managing input/output
  Buffer<float> *dryBuffer1[MAX_FRAMES] = { 0 };
  float outBuffer1[MAX_FRAMES][MAX_BUFFER_SIZE] = { { 0 } };
  Buffer<float> *dry1;
  float freqDomain1[MAX_BUFFER_SIZE] = { 0 };
  float phaseDomain1[MAX_BUFFER_SIZE] = { 0 };
  float lastPhaseDomain1[MAX_BUFFER_SIZE] = { 0 };
  float phaseDifference1[MAX_BUFFER_SIZE] = { 0 };
  Buffer<float> *dryBuffer2[MAX_FRAMES] = { 0 };
  float outBuffer2[MAX_FRAMES][MAX_BUFFER_SIZE] = { { 0 } };
  Buffer<float> *dry2;
  float freqDomain2[MAX_BUFFER_SIZE] = { 0 };
  float phaseDomain2[MAX_BUFFER_SIZE] = { 0 };
  float lastPhaseDomain2[MAX_BUFFER_SIZE] = { 0 };
  float phaseDifference2[MAX_BUFFER_SIZE] = { 0 };

  float morphing[MAX_VOICE_COUNT] = {0};
  uint8_t fmMatrix[MAX_VOICE_COUNT] = {0};
  float fmAmount[MAX_VOICE_COUNT] = {0};
  float amAmount[MAX_VOICE_COUNT] = {0};
  uint8_t rmMatrix[MAX_VOICE_COUNT] = {0};
  float rmMix[MAX_VOICE_COUNT] = {0};
  float panning[MAX_VOICE_COUNT] = {0};

  bool input1Connected;
  bool input2Connected;

  uint8_t frameSize1;
  uint8_t frameSize2;
  uint8_t lastFrameSize1;
  uint8_t lastFrameSize2;
  
  uint8_t hopSize1;
  uint8_t hopSize2;
  float sampleRate;
  uint16_t frameSizeInBytes1;
  uint16_t hopSizeInBytes1;
  uint16_t frameSizeInBytes2;
  uint16_t hopSizeInBytes2;
  uint8_t currentBin;
  bool rmActive = false;
  uint8_t rmOsc1, rmOsc2;
  uint8_t windowFunctionId;

  float freqWarpAmount = 0;
  float freqWarpCenterFrequency = 0;
  bool warpBasedOnFundamental = false;
  
  float analyzeCenter = 0;
  float analyzeBW = 0;

  float feedback = 0;
  float feedbackValue1, feedbackValue2 = 0;
  
  bool morphMode = false;

  // percentages
  float voiceCountPercentage = 0;
  float waveformPercentage = 0;
  float mixPercentage = 0;
  float morphPercentage = 0;
  float fmAmountPercentage = 0;
  float rmMixPercentage = 0;
  float octavePercentage = 0;
  float freqWarpCenterPercentage = 0;
  float bandwidthPercentage = 0;
  float blurPercentage = 0;
  float osc1Percentage = 0;
  float osc2Percentage = 0;
  float spreadPercentage = 0;
  float shiftPercentage = 0;
  float framePercentage = 0;
  float spectralPercentage = 0;
  float analyzeCenterPercentage = 0;
  float analyzeBWPercentage = 0;
  float feedbackPercentage = 0;
  
  const float phaseThreshold = 1E-01;


  // cells for panning
  OneDimensionalCells *morphingCells;
  OneDimensionalCells *frequencyModulationCells;
  OneDimensionalCells *frequencyModulationAmountCells;
  OneDimensionalCells *ringModulationCells;
  OneDimensionalCells *ringModulationMixCells;
  OneDimensionalCells *panningCells;

  // these are set by hand (ear?) - there has to be a better way
  uint16_t magnitudeAdjustment[8] = { 50, 110, 220, 380, 640, 1400, 2300, 600 };
  float magnitudeNormaliztion = 0;

  float debugOutput[16];
  float maxPhase;
};
