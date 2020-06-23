#include "../FrequencyDomain.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Cells.hpp"
#include "../model/Buffer.hpp"
#include "../model/noise/noise.hpp"

#include <cstdint>
#include <vector>
#include <future>

#include "osdialog.h"
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom

//using namespace std;

using namespace frequencydomain::dsp;
using rack::simd::float_4;

#define MAX_SAMPLES 16
#define MAX_LIVE_INPUTS 16
#define MAX_VOICE_COUNT 32
#define MAX_POLYPHONY 16
#define MAX_BUFFER_SIZE 32768 //2^15



struct GrainsOfWrathModule : Module {
    enum ParamIds {
        GRAIN_DENSITY_PARAM,
        GRAIN_DENSITY_CV_ATTENUVERTER,
        GRAIN_DENSITY_VARIATION_PARAM,
        GRAIN_DENSITY_VARIATION_CV_ATTENUVERTER,
        START_POS_PARAM,
        STOP_POS_PARAM,
        PLAY_SPEED_PARAM,
        GRAIN_LENGTH_PARAM,
        GRAIN_PITCH_PARAM,
        GRAIN_PITCH_RANDOM_PARAM,
        WINDOW_FUNCTION_PARAM,
        VOICE_WEIGHT_SCALING_PARAM,
        FREEZE_TRIGGER_MODE_PARAM,
        PITCH_RANDOM_GAUSSIAN_MODE_PARAM,
        PAN_RANDOM_GAUSSIAN_MODE_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        LIVE_INPUT,
        START_POS_SHIFT_X_CV = LIVE_INPUT + MAX_VOICE_COUNT,
        START_POS_SHIFT_Y_CV,
        START_POS_ROTATE_X_CV,
        STOP_POS_SHIFT_X_CV,
        STOP_POS_SHIFT_Y_CV,
        STOP_POS_ROTATE_X_CV,
        PLAY_SPEED_SHIFT_X_CV,
        PLAY_SPEED_SHIFT_Y_CV,
        PLAY_SPEED_ROTATE_X_CV,
        VOICE_WEIGHT_SHIFT_X_CV,
        VOICE_WEIGHT_SHIFT_Y_CV,
        VOICE_WEIGHT_ROTATE_X_CV,
        GRAIN_LENGTH_SHIFT_X_CV,
        GRAIN_LENGTH_SHIFT_Y_CV,
        GRAIN_LENGTH_ROTATE_X_CV,
        GRAIN_PITCH_SHIFT_X_CV,
        GRAIN_PITCH_SHIFT_Y_CV,
        GRAIN_PITCH_ROTATE_X_CV,
        GRAIN_PITCH_RANDOM_SHIFT_X_CV,
        GRAIN_PITCH_RANDOM_SHIFT_Y_CV,
        GRAIN_PITCH_RANDOM_ROTATE_X_CV,
        WINDOW_FUNCTION_SHIFT_X_CV,
        WINDOW_FUNCTION_SHIFT_Y_CV,
        WINDOW_FUNCTION_ROTATE_X_CV,
        PAN_SHIFT_X_CV,
        PAN_SHIFT_Y_CV,
        PAN_ROTATE_X_CV,
        PAN_RANDOM_SHIFT_X_CV,
        PAN_RANDOM_SHIFT_Y_CV,
        PAN_RANDOM_ROTATE_X_CV,
        GRAIN_DENSITY_INPUT,
        GRAIN_DENSITY_VARIATION_INPUT,
        EXTERNAL_CLOCK_INPUT,
        LIVE_FREEZE_INPUT,        
        REVERSE_GRAIN_LIVE_INPUT,        
        REVERSE_GRAIN_SAMPLES_INPUT,        
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds { OUTPUT_L, OUTPUT_R, DEBUG_OUTPUT, NUM_OUTPUTS };
    enum LightIds {
        FREEZE_TRIGGER_MODE_TRIGGER_LIGHT,
        FREEZE_TRIGGER_MODE_GATE_LIGHT = FREEZE_TRIGGER_MODE_TRIGGER_LIGHT + 3,
        PITCH_GAUSSIAN_LIGHT = FREEZE_TRIGGER_MODE_GATE_LIGHT + 3,
        PAN_GAUSSIAN_LIGHT = PITCH_GAUSSIAN_LIGHT + 3,
        NUM_LIGHTS = PAN_GAUSSIAN_LIGHT + 3
    };

    GrainsOfWrathModule ();
    ~GrainsOfWrathModule ();

    void clearSamples();
    void loadDirectory(std::string path);
    void loadSample(uint8_t slot, std::string path);
    float nextGaussian();
    int8_t weightedProbability(float *weights,int weightCount, float scaling,float randomIn);
    void spawnGrain();
    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);


    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;


    uint64_t totalSampleC[MAX_SAMPLES] = {0};

    unsigned int channels;
    unsigned int playerSampleRate;
    std::vector<std::vector<float>> playBuffer[MAX_SAMPLES];
    bool loading = false;
    std::string lastPath[MAX_SAMPLES] = {""};
    float samplePosition[MAX_SAMPLES] = {0};
    //std::vector<double> displayBuff;
    std::string fileDesc = "<Empty>";
    std::string sampleStatusDesc[MAX_SAMPLES] ={"<Empty>"};
    bool fileLoaded = false;

    int sampnumber = 0; 
    int retard = 0;
    bool reload = false ;
    std::vector <std::string> fichier;    

    uint32_t samplesPlayed;

    WindowFunction<float> *windowFunction;
    GaussianNoiseGenerator _gauss;
    dsp::SchmittTrigger playTrigger,freezeModeTrigger,pitchGaussianTrigger,panGaussianTrigger,freezeTrigger[MAX_LIVE_INPUTS],reverseTrigger[MAX_VOICE_COUNT];
    //dsp::PulseGenerator endOfSamplePulse;

    float startPosition[MAX_SAMPLES] = {0};
    float stopPosition[MAX_SAMPLES] = {1};
    float playSpeed[MAX_SAMPLES] = {1};

    float voiceWeighting[MAX_VOICE_COUNT] = {1};
    float actualVoiceWeighting[MAX_VOICE_COUNT] = {0}; 
    float weightScaling = 0;
    int windowFunctionId[MAX_VOICE_COUNT] = {4};

    float grainLength[MAX_VOICE_COUNT] = {0};
    float grainPitch[MAX_VOICE_COUNT] = {0};
    float grainPitchRandom[MAX_VOICE_COUNT] = {0};
    float panning[MAX_VOICE_COUNT] = {0};
    float panningRandom[MAX_VOICE_COUNT] = {0};

    bool pitchRandomGaussian = false;
    bool panRandomGaussian = false;

    float sampleRate;
    uint64_t sampleCounter = 0;
    uint64_t lastGrainSampleCount = 0;

    float windowFunctionSize = 0; 
    // uint8_t windowFunctionId;
    // uint8_t lastWindowFunctionId;


    float densityAddedRandomness = 0;

    Buffer<float> *liveBuffer[MAX_LIVE_INPUTS] = { 0 };
    bool liveVoiceFrozen[MAX_LIVE_INPUTS] = {false};

    bool freezeTriggerMode = false;

    bool reverseGrains[MAX_VOICE_COUNT] = {false};


    // percentages
    float startPositionPercentage = 0;
    float stopPositionPercentage = 0;
    float playSpeedPercentage = 0;

    float windowFunctionPercentage = 0;
    float grainDensityPercentage = 0;
    float grainDensityVariationPercentage = 0;

    float grainLengthPercentage = 0;
    float grainPitchPercentage = 0;
    float grainPitchRandomPercentage = 0;
    float weightScalingPercentage = 0;

    //Grain Vectors
    std::vector<std::vector<float>> grains;
    std::vector<uint8_t> individualGrainVoice;
    std::vector<float> individualGrainPosition;
    std::vector<float> individualGrainPitch;
    std::vector<float> individualGrainPanning;
    std::vector<uint8_t> individualGrainWindowFunction;
    std::vector<uint64_t> individualGrainSpawnTime;
    std::vector<bool> individualGrainReversed;


    OneDimensionalCells *startPositionCells;
    OneDimensionalCells *stopPositionCells;
    OneDimensionalCells *playSpeedCells;
    OneDimensionalCells *voiceWeightCells;
    OneDimensionalCells *grainLengthCells;
    OneDimensionalCells *grainPitchCells;
    OneDimensionalCells *grainPitchRandomCells;
    OneDimensionalCells *windowFunctionCells;
    OneDimensionalCells *panningCells;


};
