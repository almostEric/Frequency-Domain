#include "../FrequencyDomain.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/dsp/Binning.hpp"
#include "../model/Buffer.hpp"
#include "../model/point3d.hpp"
#include "../model/noise/noise.hpp"
#include "../model/Oscillator.hpp"
#include "../model/Cells.hpp"

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

#define WAV_TABLE_SIZE 2048
#define MAX_MORPHED_WAVETABLES 4
#define MAX_POLYPHONY 16
#define NBR_MORPH_MODES 4
#define NBR_SYNC_MODES 3
#define MAX_HARMONICS 16


struct BallOfConfusionModule : Module {
    enum ParamIds {
        YAW_PARAM,
        PITCH_PARAM,
        ROLL_PARAM,
        QUANTIZE_YAW_PARAM,
        QUANTIZE_PITCH_PARAM,
        QUANTIZE_ROLL_PARAM,
        FM_AMOUNT,
        FREQUENCY_PARAM,
        SYNC_MODE_PARAM,
        MORPH_MODE_PARAM,
        SYNC_POSITION_PARAM,
        SPECTRUM_SHIFT_PARAM,
        WAVEFOLD_AMOUNT_PARAM,
        WWAVEFOLD_SYMMETRY_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        YAW_INPUT,
        PITCH_INPUT,
        ROLL_INPUT,
        V_OCTAVE_INPUT,
        FM_INPUT,
        FM_AMOUNT_INPUT,
        PHASE_INPUT,
        SYNC_INPUT,
        SYNC_POSITION_INPUT,
        SPECTRUM_SHIFT_INPUT,
        WAVEFOLD_AMOUNT_INPUT,
        HARMONIC_SHIFT_X_INPUT,
        HARMONIC_SHIFT_Y_INPUT,
        NUM_INPUTS
    };
    enum OutputIds { OUTPUT_L, OUTPUT_R, DEBUG_OUTPUT, NUM_OUTPUTS };
    enum LightIds {
        SYNC_MODE_LIGHT,
        MORPH_MODE_LIGHT = SYNC_MODE_LIGHT + 3,
        YAW_QUANTIZED_LIGHT = MORPH_MODE_LIGHT + 3,
        PITCH_QUANTIZED_LIGHT = YAW_QUANTIZED_LIGHT + 3,
        ROLL_QUANTIZED_LIGHT = PITCH_QUANTIZED_LIGHT + 3,
        NUM_LIGHTS = ROLL_QUANTIZED_LIGHT + 3
    };

    enum MorphModes { MORPH_INTERPOLATE, MORPH_SPECTRAL, MORPH_SPECTRAL_0_PHASE, MORPH_TRANSFER };

    enum SyncModes { SYNC_HARD, SYNC_SOFT, SYNC_SOFT_REVERSE };


    BallOfConfusionModule ();
    ~BallOfConfusionModule ();

    void clearSamples();
    void loadDirectory(std::string path);
    void loadIndividualWavefile(std::string path);
    void loadAdditionalWavefile(std::string path);
    void loadSample(uint8_t slot, std::string path);
    void fibonacci_sphere(uint16_t samples);
    void rotateSphere(float yaw,float pitch,float roll);
    void buildActualWaveTable();
    void calculateWaveFolding();
    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);


    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;



    std::vector<point3d> sphere;
    std::vector<uint16_t> waveTableFileSampleCount;
    std::vector<float> waveTableList;
    std::vector<float> waveTableSpectralList;
    std::vector<uint16_t> waveTableFundamentalHarmonicList;
    std::vector<std::string> waveTableNames;

    uint16_t waveTablePathCount = 0;
    uint16_t waveTableFileCount = 0;
    uint16_t waveTableCount = 0;
    uint16_t waveTablesInUse[4] = {0};
    float waveTableDistance[4] = {0};
    float waveTableWeighting[4] = {0};

    float actualWaveTable[WAV_TABLE_SIZE] = {0};
    float ifftWaveTable[WAV_TABLE_SIZE] = {0};
    float prefoldedWaveTable[WAV_TABLE_SIZE] = {0};

    float fftBuffer[WAV_TABLE_SIZE] = {0}; 

    float magnitudeBuffer[WAV_TABLE_SIZE] = {0}; 
    float phaseBuffer[WAV_TABLE_SIZE] = {0}; 

    Binning *binnings;
    Result bins[1] = { { 0, 0, 0 } };
    float harmonicShiftAmount[16] = {0};
    float lastHarmonicShiftAmount[16] = {0};


    std::string morphModes[NBR_MORPH_MODES] ={"Interpolate","Spectral","Spectral 0","Transfer"};
    std::string syncModes[NBR_SYNC_MODES] ={"Hard","Soft","Soft Reverse"};


    bool loading = false;
    std::vector<std::string> lastPath;
    std::string fileDesc = "<Empty>";
    bool fileLoaded = false;

    int sampnumber = 0; 
    int retard = 0;
    bool reload = false ;
    std::vector<bool> loadFromDirectory;
    std::vector <std::string> fichier;    

    uint32_t samplesPlayed;

    WindowFunction<float> *windowFunction;
    FFT *fft;

    GaussianNoiseGenerator _gauss;
    dsp::SchmittTrigger morphModeTrigger,syncModeTrigger;
    //dsp::PulseGenerator endOfSamplePulse;

	WavelessOscillator<16, 16, WAV_TABLE_SIZE, float_4> oscillators[4];

    bool rebuild = false;
    bool recalculateWave = false;

    float yaw=0;
    float pitch=0;
    float roll=0;
    float lastYaw = -1;
    float lastPitch = -1;
    float lastRoll = -1;

    float scatterPercent = 0;
    float lastScatterPercent = 0;

    int morphMode = 0;
    int lastMorphMode = -1;

    float phase=0;
    float currentWavePosition = 0;

    int syncMode = SYNC_HARD; 
    float syncPosition = 1;

    int16_t spectrumShift = 0;
    int16_t lastSpectrumShift = 0;

    float wavefoldAmount = 1.0;
    float wavefoldSymmetry = 0.0;
    float lastWavefoldAmount = -1.0;
    float lastWavefoldSymmetry = -1.0;
    bool recalcFold = false;

    float sampleRate;
    

    OneDimensionalCells *harmonicShiftCells;

    // percentages
    float yawPercentage = 0;
    float pitchPercentage = 0;
    float rollPercentage = 0;

    float tuningPercentage = 0;
    float fmAmountPercentage = 0;
    float syncPositionPercentage = 0;
    float spectrumShiftPercentage = 0;
    float wavefoldAmountPercentage = 0;
    float wavefoldSymmetryPercentage = 0;

};
