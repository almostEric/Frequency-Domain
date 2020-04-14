#include <cstdint>
#include <vector>

#include "../SpectreModular.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/DelayLine.hpp"
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
                  PIN_FEEDBACK_0S,
                  LINK_ATTENUATION,
                  LINK_DELAY_TIME,
                  LINK_FEEDBACK,
                  MIX,                
                  NUM_PARAMS };
  enum InputIds {
    INPUT,
    ATTENUATION_X_CV,
    ATTENUATION_Y_CV,
    DELAY_TIME_X_CV,
    DELAY_TIME_Y_CV,
    DELAY_FEEDBACK_X_CV,
    DELAY_FEEDBACK_Y_CV,
    MIX_CV,
    NUM_INPUTS
  };
  enum OutputIds { OUTPUT, NUM_OUTPUTS };
  enum LightIds { DELAY_RANGE_LIGHT,
                  PIN_ATTENUATION_0S_LIGHT = DELAY_RANGE_LIGHT + 3,
                  PIN_DELAY_TIME_0S_LIGHT = PIN_ATTENUATION_0S_LIGHT + 3,
                  PIN_FEEDBACK_0S_LIGHT = PIN_DELAY_TIME_0S_LIGHT + 3,
                  LINK_ATTENUATION_LIGHT = PIN_FEEDBACK_0S_LIGHT + 3,
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
  // float feedbackResult[MAX_FRAMES][NUM_BANDS * 2] = {{ 0 }};
  // float processed[MAX_FRAMES][NUM_BANDS * 2] = { {0} }; 
  float *feedbackResult[MAX_FRAMES] = { 0 };
  float *processed[MAX_FRAMES] = {0}; 

  dsp::SchmittTrigger delayRangeTrigger,pinAttenuation0sTrigger,pinDelayTime0sTrigger,pinFeedback0sTrigger,linkAttenuationTrigger,linkDelayTimeTrigger,linkFeedbackTrigger;
//Not sure if this shoud be based on ui # of bands (128 right now)
  // float attenuation[NUM_BANDS] = {0};
  // float delayTime[NUM_BANDS] = {0};
  // float feedbackAmount[NUM_BANDS] = {0};

  Buffer<float> *dry;

  FFT *fft;
  FFT *feedbackfft;
  DelayLine delayLine[4096];
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
  uint8_t windowFunctionId;

  uint16_t bandsPerUIBand[NUM_UI_BANDS] = {0};

  bool pinAttenuation0s = false;
  bool pinDelayTime0s = false;
  bool pinFeedback0s = false;


  bool attenuationLinked = false;
  bool delayTimeLinked = false;
  bool feedbackLinked = false;


  // percentages
  float mixPercentage = 0;

  // cells for stuff
  OneDimensionalCells *attenuationCells;
  OneDimensionalCells *delayTimeCells;
  OneDimensionalCells *feedbackCells;

};