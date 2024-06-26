#include "../FrequencyDomain.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Buffer.hpp"
#include "../model/point3d.hpp"
#include "../model/noise/noise.hpp"
#include "../model/dsp/filter/Filter.hpp"
#include "../model/dsp/filter/NonlinearBiquad.hpp"
#include "../model/dsp/filter/ChebyshevI.hpp"
#include "../model/dsp/filter/CombFilter.hpp"
#include "../model/dsp/filter/ModalFilter.hpp"
#include "../model/cubeFilterModel.hpp"
#include "../model/cubeFilterPoint.hpp"
#include "../model/Interpolate.hpp"


#include <cstdint>
#include <vector>
#include <future>

#include "osdialog.h"
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom

//using simd::float_4;

#define NBR_FILTER_STAGES 7
#define NBR_CHANNELS 2
#define NBR_VERTEX 8
#define NBR_DIMENSIONS 3
#define NBR_EDGES 12



struct BoxOfRevelationModule : Module {
    enum ParamIds {
        FILTER_MODEL_PARAM,
        FREQUENCY_PARAM,
        Y_PARAM,
        Z_PARAM,
        FREQUENCY_2_PARAM,
        Y_2_PARAM,
        Z_2_PARAM,
        LINK_PARAM,
        MS_MODE_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        INPUT_L,
        INPUT_R,
        FILTER_MODEL_INPUT,
        FREQUENCY_INPUT,
        Y_INPUT,
        Z_INPUT,
        FREQUENCY_2_INPUT,
        Y_2_INPUT,
        Z_2_INPUT,
        NUM_INPUTS
    };

    enum OutputIds { OUTPUT_L, OUTPUT_R,  NUM_OUTPUTS };
    enum LightIds {
        LINK_MODE_LIGHT,
        MS_MODE_LIGHT = LINK_MODE_LIGHT + 3,
        NUM_LIGHTS = MS_MODE_LIGHT + 3
    };

    BoxOfRevelationModule ();
    ~BoxOfRevelationModule ();

    void loadCubeFile(std::string path);
    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);
    void reConfigParam(int paramId, float minValue, float maxValue, float defaultValue);


    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;


    dsp::SchmittTrigger linkModeTrigger,midSideModeTrigger;

    std::unique_ptr<Filter<double>> pFilter[NBR_FILTER_STAGES][NBR_CHANNELS];
    //std::unique_ptr<NonlinearBiquad<double>> pFilter[NBR_FILTER_STAGES][NBR_CHANNELS];
    //std::unique_ptr<ChebyshevI<double>> cFilter[NBR_FILTER_STAGES][NBR_CHANNELS]; //temporary until we get abstract class set up

//Biquad filter params
    double Fc[NBR_FILTER_STAGES] = {0};
    double Q[NBR_FILTER_STAGES] = {0};
    double drive[NBR_FILTER_STAGES][2] = {{0}};
    double gain[NBR_FILTER_STAGES] = {0};

    //Comb filter Parameters
    double feedforwardDelay[NBR_FILTER_STAGES] = {0};
    double feedbackDelay[NBR_FILTER_STAGES] = {0};
    double feedforwardGain[NBR_FILTER_STAGES] = {0};
    double feedbackGain[NBR_FILTER_STAGES] = {0};


    //Common filter params
    double attenuation[NBR_FILTER_STAGES][2] = {{0}};

    std::string lastPath;

    std::vector<cubeFilterModel> cubeModels;
    int nbrCubeModels = 0;
    double makeupGain = 0; //In dB
    double makeupAttenuation[2] = {1.0}; //actual multiplier

    int currentModel = 0;
    cubeFilterPoint currentPoint[2];
    int nbrfilterLevels; // Number of serial filter levels in current model

    float frequency[2]= {0};
    float yMorph[2]={0};
    float zMorph[2]={0};
    float lastFrequency[2] = {-1};
    float lastYMoprh[2] = {-1};
    float lastZMoprh[2] = {-1};


    int lastModel = -1;

    bool linkMode = true;
    bool msMode = false;

    float cubePoints[NBR_VERTEX][NBR_DIMENSIONS] = {{0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{1,1,1}};
    int edges[NBR_EDGES][2] = {{0,1},{1,5},{5,4},{4,0}, {2,3},{3,7},{7,6},{6,2}, {0,2},{1,3},{5,7},{4,6}};


    float sampleRate;

    // percentages
    float frequencyPercentage[2] = {0};
    float yMorphPercentage[2] = {0};
    float zMorphPercentage[2] = {0};

    float modelPercentage = 0;


};
