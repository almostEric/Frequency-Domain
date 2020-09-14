#include "../FrequencyDomain.hpp"
#include "../model/dsp/Binning.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Oscillator.hpp"
#include "../model/Buffer.hpp"
#include "../model/OscillatorBank.hpp"
#include "../model/Cells.hpp"
#include "../model/dsp/FFT.hpp"

#include <cstdint>
#include <vector>
#include <future>

#include "osdialog.h"
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom

//using namespace std;

using rack::simd::float_4;

#define MAX_VOICE_COUNT 32
#define MAX_FRAMES 128
#define MAX_POLYPHONY 16
#define MAX_BUFFER_SIZE 32768 //2^15
#define NUM_UI_BANDS 64
#define NUM_UI_FRAMES 328


// struct ResultArray {
//     Result results[MAX_VOICE_COUNT];
// };

struct FreudianSlipModule : Module {
    enum ParamIds {
    PLAY_PARAM,
    WINDOW_FUNCTION,
    VOICE_WAVEFORM,
    VOICE_COUNT, 
    V_OCT_PARAM, 
    FREQ_WARP_AMOUNT,
    FREQ_WARP_CENTER,
    FREQ_WARP_USE_FUNDAMENTAL,
    RING_MODULATION,
    PLAY_SPEED_PARAM,
    START_POS_PARAM,
    STOP_POS_PARAM,
    RANDOMIZE_PARAM,
    FM_AMOUNT,
    RM_MIX,
    LOOP_PARAM,
    POSITION_MODE_PARAM,
    EOC_MODE_PARAM,
    NUM_PARAMS
    };

    enum InputIds {
    PLAY_INPUT,
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
    V_OCT_CV,
    FREQ_WARP_AMOUNT_CV,
    FREQ_WARP_CENTER_CV,
    RM_SHIFT_X_CV,
    RM_SHIFT_Y_CV,
    RM_MIX_SHIFT_X_CV,
    RM_MIX_SHIFT_Y_CV,
    RANDOMIZE_CV,
    PAN_SHIFT_X_CV,
    PAN_SHIFT_Y_CV,
    PAN_ROTATE_X_CV,
    PLAY_SPEED_SHIFT_X_CV,
    PLAY_SPEED_SHIFT_Y_CV,
    PLAY_SPEED_ROTATE_X_CV,
    START_POS_SHIFT_X_CV,
    START_POS_SHIFT_Y_CV,
    START_POS_ROTATE_X_CV,
    STOP_POS_SHIFT_X_CV,
    STOP_POS_SHIFT_Y_CV,
    STOP_POS_ROTATE_X_CV,
    LOOP_INPUT,
    NUM_INPUTS
    };
    enum OutputIds { OUTPUT_L, OUTPUT_R, EOC_OUTPUT, DEBUG_OUTPUT, NUM_OUTPUTS };
    enum LightIds {
    RING_MODULATION_ENABLED_LIGHT,
    FREQ_WARP_USE_FUNDAMENTAL_LIGHT = RING_MODULATION_ENABLED_LIGHT + 3,
    MORPH_MODE_LIGHT = FREQ_WARP_USE_FUNDAMENTAL_LIGHT + 3,
    LOOP_MODE_LIGHT = MORPH_MODE_LIGHT + 3,
    POSITION_MODE_LIGHT = LOOP_MODE_LIGHT + 3,
    EOC_MODE_LIGHT = POSITION_MODE_LIGHT + 3,
    NUM_LIGHTS = EOC_MODE_LIGHT + 3
    };

    FreudianSlipModule ();
    ~FreudianSlipModule ();

    void loadSample(std::string path);
    void analyze();
    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);

    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;


    uint64_t totalSampleC;

    unsigned int channels;
    unsigned int playerSampleRate;
    std::vector<std::vector<float>> playBuffer;
    bool loading = false;
    bool play = false;
    std::string lastPath = "";
    float samplePos = 0;
    float startPos = 0;
    //std::vector<double> displayBuff;
    std::string fileDesc = "<No Sample Loaded>";
    std::string sampleStatusDesc = "<No Sample Loaded>";
    bool fileLoaded = false;

    int sampnumber = 0;
    int retard = 0;
    bool reload = false ;
    bool oscState = false ;
    std::vector <std::string> fichier;

    int analysisStatus = 0; // 0 not done

    FFT *fft1;
    FFT *fft2;

    float histogram[NUM_UI_FRAMES][NUM_UI_BANDS] = {{0.0}};


    // binning, 32 bins to work with
    Binning *binnings1;
    Result bins1[MAX_VOICE_COUNT] = { { 0, 0, 0 } };
    Binning *binnings2;
    Result bins2[MAX_VOICE_COUNT] = { { 0, 0, 0 } };

    std::vector<Result> voiceAnalysis1;
    std::vector<Result> voiceAnalysis2;
    int32_t frameCount = 0;
    int32_t framePlayedCount[MAX_VOICE_COUNT] = {0};
    int32_t frameIndex[MAX_VOICE_COUNT] = {0};
    float frameSubIndex[MAX_VOICE_COUNT] = {0.0}; //need better name
    bool framesCompleted[MAX_VOICE_COUNT] = {false};


    uint32_t samplesPlayed;

    WindowFunction<float> *windowFunction;
    dsp::SchmittTrigger playTrigger,rmTrigger,warpUseFundamentalTrigger,loopTrigger,positionModeTrigger,eocModeTrigger;
    dsp::PulseGenerator endOfSamplePulse;

    OscillatorBank bankL;
    OscillatorBank bankR;

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

    uint8_t fmMatrix[MAX_VOICE_COUNT] = {0};
    float fmAmount[MAX_VOICE_COUNT] = {0};
    float amAmount[MAX_VOICE_COUNT] = {0};
    uint8_t rmMatrix[MAX_VOICE_COUNT] = {0};
    float rmMix[MAX_VOICE_COUNT] = {0};
    float panning[MAX_VOICE_COUNT] = {0};
    float playSpeed[MAX_VOICE_COUNT] = {1};
    float startPosition[MAX_VOICE_COUNT] = {0};
    float stopPosition[MAX_VOICE_COUNT] = {1};

    bool input1Connected;
    bool input2Connected;

    uint8_t frameSize;
    uint8_t lastFrameSize;

    uint8_t hopSize;
    float sampleRate;
    uint16_t frameSizeInBytes;
    uint16_t hopSizeInBytes;
    uint8_t currentBin;
    bool rmActive = false;
    uint8_t rmOsc1, rmOsc2;
    uint8_t windowFunctionId;
    uint8_t lastWindowFunctionId;

    float freqWarpAmount = 0;
    float freqWarpCenterFrequency = 0;
    bool warpBasedOnFundamental = false;


    float randomizeStepAmount = 0;

    bool loopMode = false;
    bool positionMode = false;
    bool eocMode = false;

    float frequencies1[MAX_VOICE_COUNT] {0};
    float magnitudes1[MAX_VOICE_COUNT] {0};
    float frequencies2[MAX_VOICE_COUNT] {0};
    float magnitudes2[MAX_VOICE_COUNT] {0};



    // percentages
    float voiceCountPercentage = 0;
    float waveformPercentage = 0;
    float mixPercentage = 0;
    float fmAmountPercentage = 0;
    float rmMixPercentage = 0;
    float vOctPercentage = 0;
    float freqWarpCenterPercentage = 0;
    float spreadPercentage = 0;
    float shiftPercentage = 0;
    float framePercentage = 0;
    float randomizeStepPercentage = 0;
    float playSpeedPercentage = 0;
    float startPositionPercentage = 0;
    float stopPositionPercentage = 0;
    const float phaseThreshold = 1E-01;


    // cells for panning
    OneDimensionalCells *frequencyModulationCells;
    OneDimensionalCells *frequencyModulationAmountCells;
    OneDimensionalCells *ringModulationCells;
    OneDimensionalCells *ringModulationMixCells;
    OneDimensionalCells *panningCells;
    OneDimensionalCells *playSpeedCells;
    OneDimensionalCells *startPositionCells;
    OneDimensionalCells *stopPositionCells;


    // float debugOutput[16];
    float maxPhase;
};
