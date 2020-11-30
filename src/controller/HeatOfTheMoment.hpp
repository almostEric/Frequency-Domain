#include "../FrequencyDomain.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/NoiseGenerator.hpp"
#include "../model/Interpolate.hpp"


#include <cstdint>
#include <vector>
#include <future>

#include "cmath"



struct HeatOfTheMomentModule : Module {
    enum ParamIds {
        NOISE_COLOR_PARAM,
        WINDOW_FUNCTION_PARAM,
        IMPULSE_DURATION_PARAM,
        IMPULSE_PEAK_COUNT_PARAM,
        IMPULSE_REPEAT_FREQUENCY_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        NOISE_COLOR_INPUT,
        WINDOW_FUNCTION_INPUT,
        IMPULSE_DURATION_INPUT,
        IMPULSE_PEAK_COUNT_INPUT,
        IMPULSE_REPEAT_FREQUENCY_INPUT,
        GATE_INPUT,
        NUM_INPUTS
    };

    enum OutputIds { OUTPUT_L, NUM_OUTPUTS };
    enum LightIds {
        NUM_LIGHTS
    };

    HeatOfTheMomentModule ();
    ~HeatOfTheMomentModule ();

    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);

    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;

    int sampleCount = 0;
    int repeatCount = 0;
    dsp::SchmittTrigger gateTrigger;

    int impulsePeakCount = 0;


    float impulseDuration;
    int impulseLengthInSamples;
    float repeatFrequency;
    int impulseRepeatInSamples;

    WindowFunction<float> *windowFunction;
    int windowFunctionId = 0;
    NoiseColor noiseColor = NOISE_WHITE;
    NoiseGenerator noiseGenerator;


    float sampleRate;

    // percentages
    float noiseColorPercentage = 0;
    float windowFunctionPercentage = 0;
    float inpulsePeakCountPercentage = 0;
    float impulseDurationPercentage = 0;
    float impulseRepeatPercentage = 0;


};
