#include "BoxOfRevelation.hpp"
#include <cmath>


BoxOfRevelationModule::BoxOfRevelationModule() {
    //fprintf(stderr, "initializing...  \n");

    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(FILTER_MODEL_PARAM, 0, 1, 0, "Model #");

    configParam(FREQUENCY_PARAM, 0.0f, 1.0f, 0.0f, "Frequency","%",0,100);
    configParam(Y_PARAM, 0.0f, 1.0f, 0.0f, "Y","%",0,100);
    configParam(Z_PARAM, 0.0f, 1.0f, 0.0f, "Z","%",0,100);

    configParam(FREQUENCY_2_PARAM, 0.0f, 1.0f, 0.0f, "2nd Frequency","%",0,100);
    configParam(Y_2_PARAM, 0.0f, 1.0f, 0.0f, "2nd Y","%",0,100);
    configParam(Z_2_PARAM, 0.0f, 1.0f, 0.0f, "2nd Z","%",0,100);

    configButton(LINK_PARAM,"Link L/R");
    configButton(MS_MODE_PARAM,"Mid-Side Mode");

    configInput(INPUT_L, "Left");
    configInput(INPUT_R, "Right");

    configInput(FILTER_MODEL_INPUT, "Model # CV");

    configInput(FREQUENCY_INPUT, "Frequency CV");
    configInput(Y_INPUT, "Y CV");
    configInput(Z_INPUT, "Z CV");

    configInput(FREQUENCY_2_INPUT, "2nd Frequency CV");
    configInput(Y_2_INPUT, "2nd Y");
    configInput(Z_2_INPUT, "2nd Z");

    configOutput(OUTPUT_L, "Left/Mono");
    configOutput(OUTPUT_R, "Right");


    for(int c=0;c<NBR_CHANNELS;c++) {
        for(int s=0;s<NBR_FILTER_STAGES;s++) {
            pFilter[s][c].reset(new NonlinearBiquad<double>(bq_type_bandpass, 0.5 , 0.207, 0));
        }
    }
    onReset();
}

void BoxOfRevelationModule::reConfigParam(int paramId, float minValue, float maxValue, float defaultValue) {
    ParamQuantity *pq = paramQuantities[paramId];
    pq->minValue = minValue;
    pq->maxValue = maxValue;
    pq->defaultValue = defaultValue;		
}


BoxOfRevelationModule::~BoxOfRevelationModule() {

}

void BoxOfRevelationModule::onReset() {
    //fprintf(stderr, "resetting...  \n");
    cubeModels.clear();
    nbrCubeModels = 0;
    lastPath = asset::plugin(pluginInstance,"res/presets/defaultFilterCubes.json");
    loadCubeFile(lastPath);
    currentModel = 0;
}

void BoxOfRevelationModule::dataFromJson(json_t *root) {

    // lastPath
    json_t *lastPathJ = json_object_get(root, "lastPath");
    if (lastPathJ) {
        lastPath = json_string_value(lastPathJ);
        loadCubeFile(lastPath);
    }

    json_t *linkModeJ = json_object_get(root, "linkMode");
    if (linkModeJ) 
        linkMode = json_integer_value(linkModeJ);

    json_t *msModeJ = json_object_get(root, "msMode");
    if (msModeJ) 
        msMode = json_integer_value(msModeJ);

}

json_t *BoxOfRevelationModule::dataToJson() {

  json_t *root = json_object();
  
  json_object_set_new(root, "lastPath", json_string(lastPath.c_str()));	
  json_object_set_new(root, "linkMode", json_integer(linkMode));	
  json_object_set_new(root, "msMode", json_integer(msMode));	

  return root;
}

float BoxOfRevelationModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}

void BoxOfRevelationModule::loadCubeFile(std::string path)  {

    //fprintf(stderr, "Loading cube - started: %s  \n",path.c_str());

    FILE* file = fopen(path.c_str(), "r");
	if (!file)
		return;

    // fprintf(stderr, "Loading cube - file opened  \n");

    
	json_error_t error;
	json_t* rootJ = json_loadf(file, 0, &error);
	if (!rootJ)
		throw Exception(string::f("Model Cube File file has invalid JSON at %d:%d %s", error.line, error.column, error.text));

    // fprintf(stderr, "Loading cube - successfully parsed  \n");

    fclose(file);

    lastPath = path;
    currentModel = -1;
    cubeModels.clear();
    

    json_t *modelsJ = json_object_get(rootJ, "models");
    if (modelsJ) {
        size_t index;
        json_t *modelJ;


        json_array_foreach(modelsJ, index, modelJ) {

            json_t *modelNameJ = json_object_get(modelJ, "modelName");
            if (modelNameJ) {
                std::string modelName = json_string_value(modelNameJ);
                cubeFilterModel model = cubeFilterModel(modelName);

                json_t *filterModelsJ = json_object_get(modelJ, "filterModels");
                size_t filterIndex;
                json_t *filterModelJ;
                json_array_foreach(filterModelsJ, filterIndex, filterModelJ) {
                    int filterModel = json_integer_value(filterModelJ);
                    model.filterModel[filterIndex] = filterModel;
                }

                json_t *filterNLStructuresJ = json_object_get(modelJ, "filterNonLinearityStructures");
                json_t *filterNLStructureJ;
                json_array_foreach(filterNLStructuresJ, filterIndex, filterNLStructureJ) {
                    int filterNLStructure = json_integer_value(filterNLStructureJ);
                    model.filterNonlinearityStructure[filterIndex] = filterNLStructure;
                }

                json_t *filterNLFunctionsJ = json_object_get(modelJ, "filterNonLinearityFunctions");
                json_t *filterNLFunctionJ;
                json_array_foreach(filterNLFunctionsJ, filterIndex, filterNLFunctionJ) {
                    int filterNLFunction = json_integer_value(filterNLFunctionJ);
                    model.filterNonlinearityFunction[filterIndex] = filterNLFunction;
                }

                json_t *filterTypesJ = json_object_get(modelJ, "filterTypes");
                json_t *filterTypeJ;
                json_array_foreach(filterTypesJ, filterIndex, filterTypeJ) {
                    int filterType = json_integer_value(filterTypeJ);
                    model.filterType[filterIndex] = filterType;
                }

                json_t *filterLevelsJ = json_object_get(modelJ, "filterLevels");
                json_t *filterLevelJ;
                json_array_foreach(filterLevelsJ, filterIndex, filterLevelJ) {
                    int filterLevel = json_integer_value(filterLevelJ);
                    model.filterLevel[filterIndex] = filterLevel;   
                            //fprintf(stderr, "Current Model: %llu  Filter Index: %llu  Level: %i  \n",index,filterIndex,filterLevel);
                }

                json_t *vertexesJ = json_object_get(modelJ  , "vertexes");
                size_t vertexIndex;
                json_t *vertexJ;
                json_array_foreach(vertexesJ, vertexIndex, vertexJ) {
                    int x = json_integer_value(json_object_get(vertexJ, "x"));
                    int y = json_integer_value(json_object_get(vertexJ, "y"));
                    int z = json_integer_value(json_object_get(vertexJ, "z"));
                    float makeupGain = json_real_value(json_object_get(vertexJ, "makeupGain"));
                    model.vertex[x][y][z].makeupGain = makeupGain;
                    json_t *filtersJ = json_object_get(vertexJ, "filters");
                    size_t filterIndex;
                    json_t *filterJ;
                    json_array_foreach(filtersJ, filterIndex, filterJ) {
                        int filterNumber = filterIndex;
                        switch (model.filterModel[filterIndex]) {
                            case FILTER_MODEL_BIQUAD:
                            {
                                float Fc = 15000;
                                float _q = .707;
                                json_t *filterNumberJ = json_object_get(filterJ, "filterNumber");
                                if(filterNumberJ)
                                    filterNumber = json_integer_value(filterNumberJ);

                                json_t *FcJ = json_object_get(filterJ, "Fc");
                                if(FcJ)
                                    Fc = json_real_value(FcJ);

                                json_t *qJ = json_object_get(filterJ, "Q");
                                if(qJ)
                                    _q = json_real_value(qJ);

                                model.vertex[x][y][z].filterParameters[filterNumber].Fc = Fc;
                                model.vertex[x][y][z].filterParameters[filterNumber].Q = _q;
                                break;
                            }
                            case FILTER_MODEL_COMB:
                            {
                                float _feedforwardAmount = 0;
                                float _feedbackAmount = 0;
                                float _feedforwardGain = 0.0;
                                float _feedbackGain = 0.0;

                                json_t *filterNumberJ = json_object_get(filterJ, "filterNumber");
                                if(filterNumberJ)
                                    filterNumber = json_integer_value(filterNumberJ);

                                json_t *FfaJ = json_object_get(filterJ, "feedforwardAmount");
                                if(FfaJ)
                                    _feedforwardAmount = json_real_value(FfaJ);

                                json_t *FfbJ = json_object_get(filterJ, "feedbackAmount");
                                if(FfbJ)
                                    _feedbackAmount = json_real_value(FfbJ);

                                json_t *ffgJ = json_object_get(filterJ, "feedforwardGain");
                                if(ffgJ)
                                    _feedforwardGain = json_real_value(ffgJ);

                                json_t *fbgJ = json_object_get(filterJ, "feedbackGain");
                                if(fbgJ)
                                    _feedbackGain = json_real_value(fbgJ);

                                model.vertex[x][y][z].filterParameters[filterNumber].feedforwardAmount = _feedforwardAmount;
                                model.vertex[x][y][z].filterParameters[filterNumber].feedbackAmount = _feedbackAmount;
                                model.vertex[x][y][z].filterParameters[filterNumber].feedforwardGain = _feedforwardGain;
                                model.vertex[x][y][z].filterParameters[filterNumber].feedbackGain = _feedbackGain;
                                break;
                            }
                            case FILTER_MODEL_MODAL:
                            {
                                float Fc = 15000;
                                float _decay = 0.25;
                                float _phase = 1.0;
                                json_t *filterNumberJ = json_object_get(filterJ, "filterNumber");
                                if(filterNumberJ)
                                    filterNumber = json_integer_value(filterNumberJ);

                                json_t *FcJ = json_object_get(filterJ, "Fc");
                                if(FcJ)
                                    Fc = json_real_value(FcJ);

                                json_t *decayJ = json_object_get(filterJ, "decay");
                                if(decayJ)
                                    _decay = json_real_value(decayJ);

                                json_t *phaseJ = json_object_get(filterJ, "phase");
                                if(phaseJ)
                                    _phase = json_real_value(phaseJ);

                                model.vertex[x][y][z].filterParameters[filterNumber].Fc = Fc;
                                model.vertex[x][y][z].filterParameters[filterNumber].decay = _decay;
                                model.vertex[x][y][z].filterParameters[filterNumber].phase = _phase;
                                break;
                            }
                        }
                        float _drive = 1.0;
                        json_t *driveJ = json_object_get(filterJ, "drive");
                                if(driveJ)
                                    _drive = json_real_value(driveJ);
                        model.vertex[x][y][z].filterParameters[filterNumber].drive = _drive;

                        float _gain = 0.0;
                        json_t *gainJ = json_object_get(filterJ, "gain");
                        if(gainJ)
                            _gain = json_real_value(gainJ);
                        model.vertex[x][y][z].filterParameters[filterNumber].gain = _gain;

                    }                    
                }
                cubeModels.push_back(model);
            }
        }
    }

	json_decref(rootJ);
    currentModel = 0;
    lastModel = -1;
    //fprintf(stderr, "Loading cube - reconfiguring params %llu \n",filterModels.size());
    nbrCubeModels = cubeModels.size();
    reConfigParam(FILTER_MODEL_PARAM,0,nbrCubeModels-1,0);
}

void BoxOfRevelationModule::process(const ProcessArgs &args) {

    const float epsilon = .001; 
    sampleRate = args.sampleRate;

    if (linkModeTrigger.process(params[LINK_PARAM].getValue())) {
        linkMode = !linkMode;
    }
    if(!linkMode) {
        lights[LINK_MODE_LIGHT+0].value = 0;
        lights[LINK_MODE_LIGHT+1].value = 0;
        lights[LINK_MODE_LIGHT+2].value = 0.0;
    } else { 
        lights[LINK_MODE_LIGHT+0].value = 1;
        lights[LINK_MODE_LIGHT+1].value = 1;
        lights[LINK_MODE_LIGHT+2].value = 0.2;
    }

    if (midSideModeTrigger.process(params[MS_MODE_PARAM].getValue())) {
        msMode = !msMode;
    }
    if(!msMode) {
        lights[MS_MODE_LIGHT+0].value = 0;
        lights[MS_MODE_LIGHT+1].value = 0;
        lights[MS_MODE_LIGHT+2].value = 0.0;
    } else { 
        lights[MS_MODE_LIGHT+0].value = 1;
        lights[MS_MODE_LIGHT+1].value = 1;
        lights[MS_MODE_LIGHT+2].value = 0.2;
    }

    currentModel = paramValue(FILTER_MODEL_PARAM,FILTER_MODEL_INPUT,0.0f,nbrCubeModels-1); 
    modelPercentage = float(currentModel) / (nbrCubeModels > 1 ? nbrCubeModels-1 : 1);
    if(nbrCubeModels > 0 && currentModel != -1 && currentModel != lastModel) {
        nbrfilterLevels = 0;
        for(int s=0;s<NBR_FILTER_STAGES;s++) {
            switch(cubeModels[currentModel].filterModel[s]) {
                case FILTER_MODEL_CHEBYSHEV:
                    pFilter[s][0].reset(new ChebyshevI<double>(c1_type_lowpass, 0.5 , 0.1, 0));
                    pFilter[s][1].reset(new ChebyshevI<double>(c1_type_lowpass, 0.5 , 0.1, 0));
                    break;
                case FILTER_MODEL_COMB:
                    pFilter[s][0].reset(new CombFilter<double>(0, 0.0 , 0.0 , 0.0, 0.0));
                    pFilter[s][1].reset(new CombFilter<double>(0, 0.0 , 0.0 , 0.0, 0.0));
                    break;
                case FILTER_MODEL_MODAL:
                    pFilter[s][0].reset(new ModalFilter<double>(0.5 , 1000.0, 0.5, 0.0));
                    pFilter[s][1].reset(new ModalFilter<double>(0.5 , 1000.0, 0.5, 0.0));
                    break;
                case FILTER_MODEL_BIQUAD:
                default:
                    pFilter[s][0].reset(new NonlinearBiquad<double>(bq_type_bandpass, 0.5 , 0.207, 0));
                    pFilter[s][1].reset(new NonlinearBiquad<double>(bq_type_bandpass, 0.5 , 0.207, 0));
            }
            if(cubeModels[currentModel].filterLevel[s]+1 > nbrfilterLevels) {
                nbrfilterLevels = cubeModels[currentModel].filterLevel[s]+1;
            }
        }
    }   
    //fprintf(stderr, "current model %i fiter levels %i  \n",currentModel,nbrfilterLevels);

    frequency[0] = paramValue(FREQUENCY_PARAM,FREQUENCY_INPUT,0.0f,1.0f);
    frequencyPercentage[0] = frequency[0];
    frequency[0] = (frequency[0]*9.0) + 1.0;
    frequency[0] = std::log10(frequency[0]);

    yMorph[0] = paramValue(Y_PARAM,Y_INPUT,0.0f,1.0f);
    yMorphPercentage[0] = yMorph[0];
    zMorph[0] = paramValue(Z_PARAM,Z_INPUT,0.0f,1.0f);
    zMorphPercentage[0] = zMorph[0];

    if (linkMode) {
        frequency[1] = frequency[0];
        frequencyPercentage[1] = frequencyPercentage[0];
        yMorph[1] = yMorph[0];
        yMorphPercentage[1] = yMorph[0];
        zMorph[1] = zMorph[0];
        zMorphPercentage[1] = zMorph[0];

    } else {
        frequency[1] = paramValue(FREQUENCY_2_PARAM,FREQUENCY_2_INPUT,0.0f,1.0f);
        frequencyPercentage[1] = frequency[1];
        frequency[1] = (frequency[1]*9.0) + 1.0;
        frequency[1] = std::log10(frequency[1]);

        yMorph[1] = paramValue(Y_2_PARAM,Y_2_INPUT,0.0f,1.0f);
        yMorphPercentage[1] = yMorph[1];
        zMorph[1] = paramValue(Z_2_PARAM,Z_2_INPUT,0.0f,1.0f);
        zMorphPercentage[1] = zMorph[1];
    }


    for(int c=0;c<2;c++) {
        if(currentModel != -1 && nbrCubeModels > 0 && (currentModel != lastModel || 
            std::abs(frequency[c]-lastFrequency[c]) > epsilon || std::abs(yMorph[c]-lastYMoprh[c]) > epsilon || std::abs(zMorph[c]-lastZMoprh[c]) > epsilon)) {
        
            currentPoint[c].x = frequency[c];
            currentPoint[c].y = yMorph[c];
            currentPoint[c].z = zMorph[c];


            makeupGain = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].makeupGain,
                                                cubeModels[currentModel].vertex[1][0][0].makeupGain,
                                                cubeModels[currentModel].vertex[0][1][0].makeupGain,
                                                cubeModels[currentModel].vertex[1][1][0].makeupGain,
                                                cubeModels[currentModel].vertex[0][0][1].makeupGain,
                                                cubeModels[currentModel].vertex[1][0][1].makeupGain,
                                                cubeModels[currentModel].vertex[0][1][1].makeupGain,
                                                cubeModels[currentModel].vertex[1][1][1].makeupGain,
                                                frequency[c],yMorph[c],zMorph[c]);

            makeupAttenuation[c] = powf(10,makeupGain / 20.0f);
        // fprintf(stderr, "  att:%f  \n",makeupGain);



            for(int s=0;s<NBR_FILTER_STAGES;s++) {
                if(cubeModels[currentModel].filterModel[s] == FILTER_MODEL_BIQUAD || cubeModels[currentModel].filterModel[s] == FILTER_MODEL_CHEBYSHEV) {
                    float cutOffFrequeny = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][0][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[0][1][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][1][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[0][0][1].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][0][1].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[0][1][1].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][1][1].filterParameters[s].Fc,
                                                                frequency[c],yMorph[c],zMorph[c]);

                    float _q = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[1][0][0].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[0][1][0].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[1][1][0].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[0][0][1].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[1][0][1].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[0][1][1].filterParameters[s].Q,
                                                    cubeModels[currentModel].vertex[1][1][1].filterParameters[s].Q,
                                                    frequency[c],yMorph[c],zMorph[c]);

                    float _drive = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][0][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][1][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][1][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][0][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][0][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][1][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][1][1].filterParameters[s].drive,
                        frequency[c],yMorph[c],zMorph[c]);

                    Fc[s] = cutOffFrequeny;
                    Q[s] = _q;
                    drive[s][c] = _drive;

                    pFilter[s][c]->setFilterParameters(cubeModels[currentModel].filterType[s],clamp(cutOffFrequeny,20.0f,20000.0f)/ sampleRate,_q,_drive,0);
                } else if (cubeModels[currentModel].filterModel[s] == FILTER_MODEL_MODAL) {
                    float cutOffFrequeny = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][0][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[0][1][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][1][0].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[0][0][1].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][0][1].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[0][1][1].filterParameters[s].Fc,
                                                                cubeModels[currentModel].vertex[1][1][1].filterParameters[s].Fc,
                                                                frequency[c],yMorph[c],zMorph[c]);

                    float _decay = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[1][0][0].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[0][1][0].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[1][1][0].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[0][0][1].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[1][0][1].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[0][1][1].filterParameters[s].decay,
                                                    cubeModels[currentModel].vertex[1][1][1].filterParameters[s].decay,
                                                    frequency[c],yMorph[c],zMorph[c]);

                    float _drive = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][0][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][1][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][1][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][0][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][0][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][1][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][1][1].filterParameters[s].drive,
                        frequency[c],yMorph[c],zMorph[c]);

                    Fc[s] = cutOffFrequeny;
                    drive[s][c] = _drive;

                    pFilter[s][c]->setFilterParameters(clamp(cutOffFrequeny,20.0f,20000.0f)/ sampleRate,_decay * sampleRate,_drive,0.0);
                } else { // COMB FILTER
                    float _feedforwardAmount = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[1][0][0].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[0][1][0].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[1][1][0].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[0][0][1].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[1][0][1].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[0][1][1].filterParameters[s].feedforwardAmount,
                                                    cubeModels[currentModel].vertex[1][1][1].filterParameters[s].feedforwardAmount,
                                                    frequency[c],yMorph[c],zMorph[c]);

                    float _feedbackAmount = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[1][0][0].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[0][1][0].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[1][1][0].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[0][0][1].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[1][0][1].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[0][1][1].filterParameters[s].feedbackAmount,
                                                    cubeModels[currentModel].vertex[1][1][1].filterParameters[s].feedbackAmount,
                                                    frequency[c],yMorph[c],zMorph[c]);

                    float _feedforwardGain = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[1][0][0].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[0][1][0].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[1][1][0].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[0][0][1].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[1][0][1].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[0][1][1].filterParameters[s].feedforwardGain,
                        cubeModels[currentModel].vertex[1][1][1].filterParameters[s].feedforwardGain,
                        frequency[c],yMorph[c],zMorph[c]);


                    float _feedbackGain = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[1][0][0].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[0][1][0].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[1][1][0].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[0][0][1].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[1][0][1].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[0][1][1].filterParameters[s].feedbackGain,
                                                            cubeModels[currentModel].vertex[1][1][1].filterParameters[s].feedbackGain,
                                                            frequency[c],yMorph[c],zMorph[c]);

                    float _drive = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][0][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][1][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][1][0].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][0][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][0][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[0][1][1].filterParameters[s].drive,
                        cubeModels[currentModel].vertex[1][1][1].filterParameters[s].drive,
                        frequency[c],yMorph[c],zMorph[c]);

                    
                    feedforwardDelay[s] = _feedforwardAmount;
                    feedbackDelay[s] = _feedbackAmount;
                    feedforwardGain[s] = _feedforwardGain;
                    feedbackGain[s] = _feedbackGain;
                    drive[s][c] = _drive;

                    pFilter[s][c]->setFilterParameters(cubeModels[currentModel].filterType[s],_feedforwardAmount,_feedbackAmount,_feedforwardGain,_feedbackGain,_drive);

                }

                pFilter[s][c]->setNonLinearType((NLType) cubeModels[currentModel].filterNonlinearityStructure[s]);

                pFilter[s][c]->setNonLinearFunction((NLFunction) cubeModels[currentModel].filterNonlinearityFunction[s]);

            
                float _gain = trilinearInterpolate(cubeModels[currentModel].vertex[0][0][0].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[1][0][0].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[0][1][0].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[1][1][0].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[0][0][1].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[1][0][1].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[0][1][1].filterParameters[s].gain,
                                                cubeModels[currentModel].vertex[1][1][1].filterParameters[s].gain,
                                                frequency[c],yMorph[c],zMorph[c]);

                
                gain[s] = _gain;
                attenuation[s][c] = powf(10,_gain / 20.0f);
                

        //fprintf(stderr, " Params stage:%i FT:%i Fc:%f Q:%f pDB:%f att:%f  \n",s,cubeModels[currentModel].filterType[s],cutOffFrequeny,_q,_gain,attenuation[s]);

            }

            lastFrequency[c] = frequency[c];
            lastYMoprh[c] = yMorph[c];
            lastZMoprh[c] = zMorph[c];
        }
    }
        lastModel = currentModel;

        

    double processedIn[2];
    double processedOut[2];
    double out[2] = {0.0};
        
    if(nbrCubeModels > 0) {
        processedIn[0] = inputs[INPUT_L].getVoltage() / 5.0;
        processedIn[1] = inputs[INPUT_R].getVoltage() / 5.0;
        if(msMode) {
            double mid = (processedIn[0] + processedIn[1]) / 2.0;
            double side = (processedIn[0] - processedIn[1]) / 2.0;
            processedIn[0] = mid;
            processedIn[1] = side;
        }
        //fprintf(stderr, "initial out %f %f  \n",out[0],out[1]);
        for(int l=0;l<nbrfilterLevels;l++) {
            out[0] = 0;
            out[1] = 0;
            float filtersInLevel = 0;
            for(int s=0;s<NBR_FILTER_STAGES;s++) {
                if(cubeModels[currentModel].filterLevel[s] == l) {
                    for(int c=0;c<NBR_CHANNELS;c++) {
                        processedOut[c] = pFilter[s][c]->process(processedIn[c]);
                        out[c]+=processedOut[c] * attenuation[s][c];
                    }
                    filtersInLevel+=1;
                }
            }
            if(filtersInLevel > 0) {
                for(int c=0;c<NBR_CHANNELS;c++) {
                    out[c] = out[c]/filtersInLevel;
                    //out[c] = out[c]/std::sqrt(filtersInLevel);
                    processedIn[c] = out[c]; // Input for next level
                }
            }
        }

// fprintf(stderr, "makeup gain  %f %f \n",makeupAttenuation[0],makeupAttenuation[1]);

        out[0] = out[0] * makeupAttenuation[0]; // Needs to be renamed, not really attenuation
        out[1] = out[1] * makeupAttenuation[1]; 

        if(msMode) {
            double mid = out[0];
            double side = out[1];
            out[0] = (mid+side) /2.0;
            out[1] = (mid-side) /2.0;
        }

        outputs[OUTPUT_L].setVoltage(clamp(out[0] * 5.0,-10.0f,10.0f));
        outputs[OUTPUT_R].setVoltage(clamp(out[1] * 5.0,-10.0f,10.0f));

    }


    
}

