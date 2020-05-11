#include <cstdint>
#include <vector>

#include "../FrequencyDomain.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/DelayLine.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Buffer.hpp"
#include "../model/dsp/Binning.hpp"
#include "../model/Cells.hpp"


using rack::simd::float_4;

#define MAX_FRAMES 4
#define MAX_NUM_BANDS 4096
#define NUM_UI_BANDS 128


struct MorphologyModule : Module {
  enum ParamIds { FRAME_SIZE,
                  INVERT_SPECTA_1,
                  INVERT_SPECTA_2,
                  SPREAD,
                  PIN_BAND_SPREAD_XS,
                  PIN_PANNING_XS,
                  X_AXIS_PIN_POS_BAND_SPREAD,
                  X_AXIS_PIN_POS_PANNING,
                  X_AXIS_ROTATION_BAND_SPREAD,
                  X_AXIS_ROTATION_PANNING,
                  INVERT_THRESHOLD_1,
                  INVERT_THRESHOLD_2,
                  NUM_PARAMS };
  enum InputIds {
    INPUT_1,
    INPUT_2,
    BAND_SHIFT_X_CV,
    BAND_SHIFT_Y_CV,
    PANNING_X_CV,
    PANNING_Y_CV,
    SPREAD_CV,
    X_AXIS_PIN_POS_BAND_SPREAD_CV,
    X_AXIS_PIN_POS_PANNING_CV,
    X_AXIS_ROTATION_BAND_SPREAD_CV,
    X_AXIS_ROTATION_PANNING_CV,
    INVERT_THRESHOLD_1_CV,
    INVERT_THRESHOLD_2_CV,
    NUM_INPUTS
  };
  enum OutputIds { OUTPUT_L, OUTPUT_R, TEST_OUTPUT,NUM_OUTPUTS };
  enum LightIds { INVERT_SPECTA_1_LIGHT,
                  INVERT_SPECTA_2_LIGHT = INVERT_SPECTA_1_LIGHT+3,
                  PIN_BAND_SPREAD_XS_LIGHT = INVERT_SPECTA_2_LIGHT + 3,
                  PIN_PANNING_XS_LIGHT = PIN_BAND_SPREAD_XS_LIGHT + 3,                  
                  NUM_LIGHTS = PIN_PANNING_XS_LIGHT+3};

  MorphologyModule ();
  ~MorphologyModule ();
 
  void process (const ProcessArgs &args) override;
  float paramValue (uint16_t, uint16_t, float, float);

  void onReset() override;
  void dataFromJson(json_t *) override;
  json_t *dataToJson() override;


  WindowFunction<float> *windowFunction;


  // buffer for managing input/output
  Buffer<float> *dryBuffer1[MAX_FRAMES] = { 0 };
  Buffer<float> *dryBuffer2[MAX_FRAMES] = { 0 };
  float processedL[MAX_FRAMES][MAX_NUM_BANDS * 2] = {{0}};
  float processedR[MAX_FRAMES][MAX_NUM_BANDS * 2] = {{0}};

  float magnitude1Array[MAX_NUM_BANDS] {0};
  float phase1Array[MAX_NUM_BANDS] {0};
  float magnitude2Array[MAX_NUM_BANDS] {0};
  float phase2Array[MAX_NUM_BANDS] {0};
  //float *processed[MAX_FRAMES] = {0}; 

  dsp::SchmittTrigger invertSpectra1Trigger,invertSpectra2Trigger,pinBandSpreadXsTrigger,pinPanningXsTrigger;

//Not sure if this shoud be based on ui # of bands (128 right now)
  float bandShift[MAX_NUM_BANDS] = {0};
  float panning[MAX_NUM_BANDS] = {0};

  Buffer<float> *dry1,*dry2;

  FFT *fft1,*fft2;
  uint8_t frameSize;
  uint8_t lastFrameSize;
  uint16_t nbrBands;
  uint8_t hopSize;
  float sampleRate;
  uint8_t currentBin = 0;
  uint16_t curPos[MAX_FRAMES] = {0};
  //uint16_t curPos= 0;
  bool invertSpectra1 = false;
  bool invertSpectra2 = false;
  uint8_t windowFunctionId;

  uint8_t pinBandSpreadXs = 0;
  uint8_t pinPanningXs = 0;


  uint16_t bandsPerUIBand[NUM_UI_BANDS] = {0};
  
  // percentages
  float bandShiftSpreadPercentage = 0;
  float bandSpreadXAxisPercentage = 0;
  float panningXAxisPercentage = 0;
  float bandShiftXAxisRotatePercentage = 0;
  float panningXAxisRotatePercentage = 0;

  float invertThreshold1Percentage = 0;
  float invertThreshold2Percentage = 0;


  // cells for stuff
  OneDimensionalCells *bandShiftCells;
  OneDimensionalCells *panningCells;

};
