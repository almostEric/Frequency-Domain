#include "../FrequencyDomain.hpp"
#include "../model/dsp/filter/Filter.hpp"
#include "../model/dsp/filter/NonlinearBiquad.hpp"
#include "../model/Interpolate.hpp"
#include "../model/DelayLine.hpp"
#include "../model/MeshJunction.hpp"
#include "../model/point3d.hpp"

#include "../model/Cells.hpp"

#include <cstdint>
#include <vector>
#include <future>

#include "osdialog.h"
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom

//using namespace std;

//using namespace frequencydomain::dsp;


#define WAV_TABLE_SIZE 4096

#define MAX_BANDS 3 //+ Mesh
#define NBR_MESH_TOPOLOGIES 3


struct DanceThisMeshAroundModule : Module {
    enum ParamIds {
        MESH_TOPOLOGY_PARAM,
        MESH_SIZE_PARAM,
        INPUT_NONLIARITY_PARAM,
        BP_1_CUTOFF_PARAM,
        BP_2_CUTOFF_PARAM,
        BP_3_CUTOFF_PARAM,
        DELAY_TIME_1_PARAM,
        DELAY_TIME_2_PARAM,
        DELAY_TIME_MESH_PARAM,
        DELAY_1_FB_AMOUNT_PARAM,
        DELAY_2_FB_AMOUNT_PARAM,
        DELAY_MESH_FB_AMOUNT_PARAM,
        DELAY_MESH_FB_NONLINEARITY_PARAM,
        MESH_IMPEDANCE_PARAM,
        GROUP_FEEDBACK_PARAM,
        ALLPASS_FC_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        IMPULSE_INPUT,
        MESH_SIZE_INPUT,
        INPUT_NONLIARITY_INPUT,
        BP_1_CUTOFF_INPUT,
        BP_2_CUTOFF_INPUT,
        BP_3_CUTOFF_INPUT,
        DELAY_TIME_1_INPUT,
        DELAY_TIME_2_INPUT,
        DELAY_TIME_MESH_INPUT,
        DELAY_1_FB_AMOUNT_INPUT,
        DELAY_2_FB_AMOUNT_INPUT,
        DELAY_MESH_FB_AMOUNT_INPUT,
        DELAY_MESH_FB_NONLINEARITY_INPUT,
        MESH_IMPEDANCE_INPUT,
        GROUP_FEEDBACK_INPUT,
        ALLPASS_FC_INPUT,
        NUM_INPUTS
    };
    enum OutputIds { OUTPUT_1, DEBUG_OUTPUT, NUM_OUTPUTS };
    enum LightIds {
        SYNC_MODE_LIGHT,
        NUM_LIGHTS = SYNC_MODE_LIGHT + 3
    };

    enum MeshTopologies { MESH_RECTILINEAR, MESH_TRIANGLE, MESH_CUBIC, MESH_TETRAHEDRAL };


    DanceThisMeshAroundModule ();
    ~DanceThisMeshAroundModule ();

    // void fibonacci_sphere(uint16_t samples);
    // void rotateSphere(float yaw,float pitch,float roll);
    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);


    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;

    NonlinearBiquad<double>* bandpassFilters[MAX_BANDS * 2];
    Biquad<double>* allpassFilters[MAX_BANDS];
    float nonlinearity[MAX_BANDS] = {1.0};

    DelayLine<float> waveguideDelays[2];

    std::vector<std::vector<std::vector<std::vector<DelayLine<float>>>>> delayLines;
    std::vector<std::vector<std::vector<MeshJunction<float>>>> junctions;

    std::vector<point3d> mesh;

    uint8_t meshTopology = MESH_RECTILINEAR;
    uint8_t meshSize = 3;

    uint8_t lastMeshTopology = MESH_TRIANGLE;
    uint8_t lastMeshSize = -1;

    int resolvedIXPostion;
    int resolvedIYPostion;
    int resolvedIZPostion;


    int resolvedO1XPostion;
    int resolvedO1YPostion;
    int resolvedO1ZPostion;


    int resolvedO2XPostion;
    int resolvedO2YPostion;


    float bpCutoff[MAX_BANDS] = {0.2};
    float lastBpCutoff[MAX_BANDS] = {0.2};


    float apCutoff = 0.2;
    float lastApCutoff = 0.2;

    float lastOutput = 0;

    dsp::SchmittTrigger morphModeTrigger,syncModeTrigger;


    float sampleRate;
    

    OneDimensionalCells *harmonicShiftCells;

    // percentages
    float topologyPercentage = 0;
    float meshSizePercentage = 0;
    float bpCutoff1Percentage = 0;
    float bpCutoff2Percentage = 0;
    float bpCutoffMeshPercentage = 0;
    float groupFeedbackPercentage = 0;
    float inputNonlinerityPercentage = 0;
    float delayTime1Percentage = 0;
    float delayTime2Percentage = 0;
    float delayTimeMeshPercentage = 0;
    float delayTimeMeshFeedbackPercentage = 0;
    float delayTimeMeshNonLinearityPercentage = 0;


};
