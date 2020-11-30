#include "HeatOfTheMoment.hpp"
#include <cmath>


HeatOfTheMomentModule::HeatOfTheMomentModule() {
    //fprintf(stderr, "initializing...  \n");

    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);


    configParam(NOISE_COLOR_PARAM, 0.0f, 6.0f, 0.0f, "Noise Color");
    configParam(WINDOW_FUNCTION_PARAM, 0.0f, 7.0f, 0.0f, "Window Function");
    configParam(IMPULSE_PEAK_COUNT_PARAM, 1.0f, 4.0f, 1.0f, "# of Peaks");

    configParam(IMPULSE_DURATION_PARAM, 0.1f, 100.0f, 1.0f, "Impulse Duration"," ms");
    configParam(IMPULSE_REPEAT_FREQUENCY_PARAM, 1.f, 1000.0f, 1.0f, "Impulse Repeat Frequency"," ms");

    // configParam(Y_PARAM, 0.0f, 1.0f, 0.0f, "Y","%",0,100);
    // configParam(Z_PARAM, 0.0f, 1.0f, 0.0f, "Z","%",0,100);


    // configParam(FILTER_MODEL_PARAM, 0, 1, 0, "Model #");

    windowFunction = new WindowFunction<float>(4096);

    onReset();
}


HeatOfTheMomentModule::~HeatOfTheMomentModule() {

}

void HeatOfTheMomentModule::onReset() {
    //fprintf(stderr, "resetting...  \n");
}

void HeatOfTheMomentModule::dataFromJson(json_t *root) {

    // lastPath
    // json_t *lastPathJ = json_object_get(root, "lastPath");
    // if (lastPathJ) {
    //     lastPath = json_string_value(lastPathJ);
    //     loadCubeFile(lastPath);
    // }
}

json_t *HeatOfTheMomentModule::dataToJson() {

  json_t *root = json_object();
  
//   json_object_set_new(root, "lastPath", json_string(lastPath.c_str()));	

  return root;
}

float HeatOfTheMomentModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}


void HeatOfTheMomentModule::process(const ProcessArgs &args) {

    noiseGenerator.sampleTime = args.sampleTime;
    noiseColor = (NoiseColor)paramValue(NOISE_COLOR_PARAM,NOISE_COLOR_INPUT,0,6);
    noiseColorPercentage = noiseColor/6.0;
    windowFunctionId = paramValue(WINDOW_FUNCTION_PARAM,WINDOW_FUNCTION_INPUT,0,7);
    windowFunctionPercentage = windowFunctionId/7.0;

    float output = 0.0;

    impulsePeakCount = paramValue(IMPULSE_PEAK_COUNT_PARAM,IMPULSE_PEAK_COUNT_INPUT,1.0,4.0);

    impulseDuration = paramValue(IMPULSE_DURATION_PARAM,IMPULSE_DURATION_INPUT,0.1,100.0);
    impulseDurationPercentage = impulseDuration / 100.0;
    repeatFrequency = paramValue(IMPULSE_REPEAT_FREQUENCY_PARAM,IMPULSE_REPEAT_FREQUENCY_INPUT,1.0,1000.0);
    impulseRepeatPercentage = repeatFrequency / 1000.0;

    impulseLengthInSamples = impulseDuration / 1000.0 * args.sampleRate;
    impulseRepeatInSamples = repeatFrequency / 1000.0 * args.sampleRate;


    if (gateTrigger.process(inputs[GATE_INPUT].getVoltage())) {
        sampleCount = 0;
        repeatCount = 0;
    }

    if(inputs[GATE_INPUT].getVoltage() > 0) {
      if(repeatCount < impulseRepeatInSamples) {
        repeatCount ++;
      } else {
        repeatCount = 0;
        sampleCount = 0;
      }
    }

    if(sampleCount < impulseLengthInSamples) {
        float phase = 4096.0 / impulseLengthInSamples * sampleCount;

        double intptr;
        int innerPhase = modf(sampleCount * impulsePeakCount / double(impulseLengthInSamples), &intptr) * 4095;

        sampleCount ++;

        output = noiseGenerator.getNoise(noiseColor) * windowFunction->windowValue(windowFunctionId,phase) * windowFunction->windowValue(windowFunctionId,innerPhase);        
        // output = noiseGenerator.getNoise(noiseColor) * windowFunction->windowValue(windowFunctionId,phase) ;        
    }

    outputs[OUTPUT_L].setVoltage(output);
    
}

