#include <cstdint>
#include <vector>

#include "../FrequencyDomain.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/DelayLine.hpp"
#include "../model/dsp/Audio.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Buffer.hpp"
#include "../model/dsp/Binning.hpp"
#include "../model/Cells.hpp"


using rack::simd::float_4;

#define MAX_FRAMES 4
#define NUM_UI_BANDS 128


struct DelayedReactionModule : Module {
  enum ParamIds { FRAME_SIZE,
                  DELAY_RANGE,
                  PIN_ATTENUATION_0S,
                  PIN_DELAY_TIME_0S,
                  REVERSE_DELAY,
                  PIN_FEEDBACK_0S,
                  PIN_PANNING_0S,
                  LINK_ATTENUATION,
                  LINK_DELAY_TIME,
                  LINK_FEEDBACK,
                  MIX,                
                  X_AXIS_PIN_POS_ATTENUATION,
                  X_AXIS_PIN_POS_DELAY_TIME,
                  X_AXIS_PIN_POS_FEEDBACK,
                  X_AXIS_PIN_POS_PANNING,
                  X_AXIS_ROTATION_ATTENUATION,
                  X_AXIS_ROTATION_DELAY_TIME,
                  X_AXIS_ROTATION_FEEDBACK,
                  X_AXIS_ROTATION_PANNING,
                  NUM_PARAMS };
  enum InputIds {
    INPUT,
    FEEDBACK_RETURN,
    ATTENUATION_X_CV,
    ATTENUATION_Y_CV,
    DELAY_TIME_X_CV,
    DELAY_TIME_Y_CV,
    DELAY_FEEDBACK_X_CV,
    DELAY_FEEDBACK_Y_CV,
    PANNING_X_CV,
    PANNING_Y_CV,
    MIX_CV,
    X_AXIS_PIN_POS_ATTENUATION_CV,
    X_AXIS_PIN_POS_DELAY_TIME_CV,
    X_AXIS_PIN_POS_FEEDBACK_CV,
    X_AXIS_PIN_POS_PANNING_CV,
    X_AXIS_ROTATION_ATTENUATION_CV,
    X_AXIS_ROTATION_DELAY_CV,
    X_AXIS_ROTATION_FEEDBACK_CV,
    X_AXIS_ROTATION_PANNING_CV,
    NUM_INPUTS
  };
  enum OutputIds { OUTPUT_L, OUTPUT_R, FEEDBACK_SEND, NUM_OUTPUTS };
  enum LightIds { DELAY_RANGE_LIGHT,
                  PIN_ATTENUATION_0S_LIGHT = DELAY_RANGE_LIGHT + 3,
                  PIN_DELAY_TIME_0S_LIGHT = PIN_ATTENUATION_0S_LIGHT + 3,
                  REVERSE_DELAY_LIGHT = PIN_DELAY_TIME_0S_LIGHT + 3,
                  PIN_FEEDBACK_0S_LIGHT = REVERSE_DELAY_LIGHT + 3,
                  PIN_PANNING_0S_LIGHT = PIN_FEEDBACK_0S_LIGHT + 3,
                  LINK_ATTENUATION_LIGHT = PIN_PANNING_0S_LIGHT + 3,
                  LINK_DELAY_TIME_LIGHT = LINK_ATTENUATION_LIGHT + 3,
                  LINK_FEEDBACK_LIGHT = LINK_DELAY_TIME_LIGHT + 3,
                  NUM_LIGHTS = LINK_FEEDBACK_LIGHT+3
                };

  // Expander
	float consumerMessage[3] = {};// this module must read from here
	float producerMessage[3] = {};// mother will write into here

  float *messageToSlave;	



  DelayedReactionModule ();
  ~DelayedReactionModule ();
 
  void process (const ProcessArgs &args) override;
  float paramValue (uint16_t, uint16_t, float, float);

  void onReset() override;
  void dataFromJson(json_t *) override;
  json_t *dataToJson() override;


  WindowFunction<float> *windowFunction;


  // buffer for managing input/output
  Buffer<float> *dryBuffer[MAX_FRAMES] = { 0 };
  float *feedbackResult[MAX_FRAMES] = { 0 };
  float *processed[MAX_FRAMES] = {0}; 

  dsp::SchmittTrigger delayRangeTrigger,pinAttenuation0sTrigger,pinDelayTime0sTrigger,reverseDelayTrigger,pinFeedback0sTrigger,pinPanning0sTrigger,linkAttenuationTrigger,linkDelayTimeTrigger,linkFeedbackTrigger;

  Buffer<float> *dry;

  FFT *fft;
  FFT *feedbackfft;
  FFTDelayLine delayLine[4096];
  ReverseFFTDelayLine reverseDelayLine[4096];
  float bandPanning[4096] = {0.0};
  uint8_t frameSize;
  uint16_t nbrBands;
  uint8_t lastFrameSize;
  uint8_t hopSize;
  uint32_t baseDelaySizeinBytes;
  float sampleRate;
  uint8_t currentBin = 0;
  uint16_t curPos[MAX_FRAMES] = {0};
  //uint16_t curPos= 0;
  float feedback = 0;
  uint8_t delayRange = 0;
  float delayRangeTime;
  float delayAdjustment = 1;
  bool reverseMode = false;
  uint8_t windowFunctionId;

  uint16_t bandsPerUIBand[NUM_UI_BANDS] = {0};

  float spectrograph[NUM_UI_BANDS] = {0};

  uint8_t pinAttenuation0s = 0;
  uint8_t pinDelayTime0s = 0;
  uint8_t pinFeedback0s = 0;
  uint8_t pinPanning0s = 0;


  bool attenuationLinked = false;
  bool delayTimeLinked = false;
  bool feedbackLinked = false;


  // percentages
  float attenuationXAxisPercentage = 0;
  float delayTimeXAxisPercentage = 0;
  float feedbackXAxisPercentage = 0;
  float panningXAxisPercentage = 0;
  float attenuationXAxisRotatePercentage = 0;
  float delayTimeXAxisRotatePercentage = 0;
  float feedbackXAxisRotatePercentage = 0;
  float panningXAxisRotatePercentage = 0;
  float mixPercentage = 0;

  // cells for stuff
  OneDimensionalCells *attenuationCells;
  OneDimensionalCells *delayTimeCells;
  OneDimensionalCells *feedbackCells;
  OneDimensionalCells *panningCells;

};
