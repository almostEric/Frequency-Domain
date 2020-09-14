#include "BoxOfRevelation.hpp"
#include <cmath>
#include "../model/Interpolate.hpp"


BoxOfRevelationModule::BoxOfRevelationModule() {
    //fprintf(stderr, "initializing...  \n");

    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);


    configParam(FREQUENCY_PARAM, 1.0f, 10.0f, 1.0f, "Frequency","%",0,100);
    configParam(Y_PARAM, 0.0f, 1.0f, 0.0f, "Y","%",0,100);
    configParam(Z_PARAM, 0.0f, 1.0f, 0.0f, "Z","%",0,100);


    configParam(FILTER_MODEL_PARAM, 0, 1, 0, "Model #");

    for(int c=0;c<NBR_CHANNELS;c++) {
        for(int s=0;s<NBR_FILTER_STAGES;s++) {
            pFilter[s][c] = new Biquad<double>(bq_type_bandpass, 0.5 , 0.207, 0);
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
    filterModels.clear();
    nbrFilterModels = 0;
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
}

json_t *BoxOfRevelationModule::dataToJson() {

  json_t *root = json_object();
  
  json_object_set_new(root, "lastPath", json_string(lastPath.c_str()));	

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
		throw UserException(string::f("Model Cube File file has invalid JSON at %d:%d %s", error.line, error.column, error.text));

    // fprintf(stderr, "Loading cube - successfully parsed  \n");

    fclose(file);

    lastPath = path;
    currentModel = -1;
    filterModels.clear();
    

    json_t *modelsJ = json_object_get(rootJ, "models");
    if (modelsJ) {
        size_t index;
        json_t *modelJ;


        json_array_foreach(modelsJ, index, modelJ) {

            json_t *modelNameJ = json_object_get(modelJ, "modelName");
            if (modelNameJ) {
                std::string modelName = json_string_value(modelNameJ);
                cubeFilterModel model = cubeFilterModel(modelName);

                json_t *filterTypesJ = json_object_get(modelJ, "filterTypes");
                size_t filterIndex;
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
                        float Fc = 15000;
                        float _q = .707;
                        float _gain = 0;
                        json_t *filterNumberJ = json_object_get(filterJ, "filterNumber");
                        if(filterNumberJ)
                            filterNumber = json_integer_value(filterNumberJ);

                        json_t *FcJ = json_object_get(filterJ, "Fc");
                        if(FcJ)
                            Fc = json_real_value(FcJ);

                        json_t *qJ = json_object_get(filterJ, "Q");
                        if(qJ)
                            _q = json_real_value(qJ);

                        json_t *gainJ = json_object_get(filterJ, "gain");
                        if(gainJ)
                            _gain = json_real_value(gainJ);

                        model.vertex[x][y][z].filterParameters[filterNumber].Fc = Fc;
                        model.vertex[x][y][z].filterParameters[filterNumber].Q = _q;
                        model.vertex[x][y][z].filterParameters[filterNumber].gain = _gain;
                        
                    }                    
                }
                filterModels.push_back(model);
            }
        }
    }

	json_decref(rootJ);
    currentModel = 0;
    lastModel = -1;
    //fprintf(stderr, "Loading cube - reconfiguring params %llu \n",filterModels.size());
    nbrFilterModels = filterModels.size();
    reConfigParam(FILTER_MODEL_PARAM,0,nbrFilterModels-1,0);
}

void BoxOfRevelationModule::saveCubeFile(std::string path) {
    json_t *root = json_object();    
    json_t *array = json_array();

    for(int i=0;i<filterModels.size();i++) {
        json_t *model = json_object();
        json_object_set_new(model, "modelName", json_string(filterModels[i].modelName.c_str()));

        json_t *filterTypes = json_array();
        for(int j=0;j<NBR_FILTER_STAGES;j++) {
            json_array_append_new(filterTypes, json_integer(filterModels[i].filterType[j]));
        }	
        json_object_set_new(model, "filterTypes", filterTypes);

        json_t *filterLevels = json_array();
        for(int j=0;j<NBR_FILTER_STAGES;j++) {
            json_array_append_new(filterLevels, json_integer(filterModels[i].filterLevel[j]));
        }	
        json_object_set_new(model, "filterLevels", filterLevels);

        json_t *vertexes = json_array();
        for(int z=0;z<2;z++) {
            for(int y=0;y<2;y++) {
                for(int x=0;x<2;x++) {
                    json_t *vertex = json_object();
                    json_t *filters = json_array();
                    for(int j=0;j<NBR_FILTER_STAGES;j++) {
                        json_t *filter = json_object();
                        json_object_set_new(filter, "filter number", json_integer(j));
                        json_object_set_new(filter, "Fc", json_real(filterModels[i].vertex[x][y][z].filterParameters[j].Fc));
                        json_object_set_new(filter, "Q", json_real(filterModels[i].vertex[x][y][z].filterParameters[j].Q));
                        json_object_set_new(filter, "gain", json_real(filterModels[i].vertex[x][y][z].filterParameters[j].gain));
                        json_array_append_new(filters, filter);
                    }
                    json_object_set_new(vertex, "x", json_integer(x));                        	
                    json_object_set_new(vertex, "y", json_integer(y));                        	
                    json_object_set_new(vertex, "z", json_integer(z));                        	
                    json_object_set_new(vertex, "filters", filters);                        	
                    json_array_append_new(vertexes, vertex);
                }
            }
        }
        json_object_set_new(model, "vertexes", vertexes);

        json_array_append_new(array, model);
    }
    
    FILE* file = fopen(path.c_str(), "w");
	if (!file)
		return;

    json_object_set_new(root, "models", array);	

    json_dumpf(root, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
	json_decref(root);

    fclose(file);
}

void BoxOfRevelationModule::process(const ProcessArgs &args) {

    const float epsilon = .001; 
    sampleRate = args.sampleRate;

    currentModel = paramValue(FILTER_MODEL_PARAM,FILTER_MODEL_INPUT,0.0f,nbrFilterModels-1); 
    modelPercentage = float(currentModel) / (nbrFilterModels > 1 ? nbrFilterModels-1 : 1);
    if(nbrFilterModels > 0 && currentModel != -1 && currentModel != lastModel) {
        nbrfilterLevels = 0;
        for(int s=0;s<NBR_FILTER_STAGES;s++) {
            if(filterModels[currentModel].filterLevel[s]+1 > nbrfilterLevels) {
                nbrfilterLevels = filterModels[currentModel].filterLevel[s]+1;
            }
        }
    }   
    //fprintf(stderr, "current model %i fiter levels %i  \n",currentModel,nbrfilterLevels);

    frequency = paramValue(FREQUENCY_PARAM,FREQUENCY_INPUT,1.0f,10.0f);
    frequencyPercentage = (frequency-1.0) / 9.0;
    frequency = std::log10(frequency);

    yMorph = paramValue(Y_PARAM,Y_INPUT,0.0f,1.0f);
    yMorphPercentage = yMorph;
    zMorph = paramValue(Z_PARAM,Z_INPUT,0.0f,1.0f);
    zMorphPercentage = zMorph;


    if(currentModel != -1 && nbrFilterModels > 0 && (currentModel != lastModel || std:: abs(frequency-lastFrequency) > epsilon || std::abs(yMorph-lastYMoprh) > epsilon || std::abs(zMorph-lastZMoprh) > epsilon)) {

        currentPoint.x = frequency;
        currentPoint.y = yMorph;
        currentPoint.z = zMorph;

        makeupGain = trilinearInterpolate(filterModels[currentModel].vertex[0][0][0].makeupGain,
                                            filterModels[currentModel].vertex[1][0][0].makeupGain,
                                            filterModels[currentModel].vertex[0][1][0].makeupGain,
                                            filterModels[currentModel].vertex[1][1][0].makeupGain,
                                            filterModels[currentModel].vertex[0][0][1].makeupGain,
                                            filterModels[currentModel].vertex[1][0][1].makeupGain,
                                            filterModels[currentModel].vertex[0][1][1].makeupGain,
                                            filterModels[currentModel].vertex[1][1][1].makeupGain,
                                            frequency,yMorph,zMorph);

        makeupAttenuation = powf(10,makeupGain / 20.0f);
    //fprintf(stderr, "  att:%f  \n",makeupAttenuation);



        for(int s=0;s<NBR_FILTER_STAGES;s++) {
            float cutOffFrequeny = trilinearInterpolate(filterModels[currentModel].vertex[0][0][0].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[1][0][0].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[0][1][0].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[1][1][0].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[0][0][1].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[1][0][1].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[0][1][1].filterParameters[s].Fc,
                                                        filterModels[currentModel].vertex[1][1][1].filterParameters[s].Fc,
                                                        frequency,yMorph,zMorph);

            float _q = trilinearInterpolate(filterModels[currentModel].vertex[0][0][0].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[1][0][0].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[0][1][0].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[1][1][0].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[0][0][1].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[1][0][1].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[0][1][1].filterParameters[s].Q,
                                            filterModels[currentModel].vertex[1][1][1].filterParameters[s].Q,
                                            frequency,yMorph,zMorph);

            float _gain = trilinearInterpolate(filterModels[currentModel].vertex[0][0][0].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[1][0][0].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[0][1][0].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[1][1][0].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[0][0][1].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[1][0][1].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[0][1][1].filterParameters[s].gain,
                                                    filterModels[currentModel].vertex[1][1][1].filterParameters[s].gain,
                                                    frequency,yMorph,zMorph);


            Fc[s] = cutOffFrequeny;
            Q[s] = _q;
            gain[s] = _gain;
			attenuation[s] = powf(10,_gain / 20.0f);
            

    //fprintf(stderr, " Params stage:%i FT:%i Fc:%f Q:%f pDB:%f att:%f  \n",s,filterModels[currentModel].filterType[s],cutOffFrequeny,_q,_gain,attenuation[s]);
            pFilter[s][0]->setBiquad(filterModels[currentModel].filterType[s],clamp(cutOffFrequeny,20.0f,20000.0f)/ sampleRate,_q,0);
            pFilter[s][1]->setBiquad(filterModels[currentModel].filterType[s],clamp(cutOffFrequeny,20.0f,20000.0f)/ sampleRate,_q,0);
        }



        lastFrequency = frequency;
        lastYMoprh = yMorph;
        lastZMoprh = zMorph;
        lastModel = currentModel;
    }

        

    double processedIn[2];
    double processedOut[2];
    double out[2] = {0.0};
        
    if(nbrFilterModels > 0) {
        processedIn[0] = inputs[INPUT_L].getVoltage() / 5.0;
        processedIn[1] = inputs[INPUT_R].getVoltage() / 5.0;
        //fprintf(stderr, "initial out %f %f  \n",out[0],out[1]);
        for(int l=0;l<nbrfilterLevels;l++) {
            float filtersInLevel = 0;
            for(int s=0;s<NBR_FILTER_STAGES;s++) {
                if(filterModels[currentModel].filterLevel[s] == l) {
                    for(int c=0;c<NBR_CHANNELS;c++) {
                        processedOut[c] = pFilter[s][c]->process(processedIn[c]);
                        out[c]+=(processedOut[c] * attenuation[s]);
                    }
                    filtersInLevel+=1;
                }
            }
            if(filtersInLevel > 0) {
                for(int c=0;c<NBR_CHANNELS;c++) {
                    out[c] = out[c]/std::sqrt(filtersInLevel);
                    processedIn[c] = out[c]; // Input for next level
                }
            }
        }


        for(int c=0;c<NBR_CHANNELS;c++) {
            out[c] = out[c] * makeupAttenuation * 5.0; // Needs to be renamed, not really attenuation
            //fprintf(stderr, "OUTOUT %i %f %f  \n",c,in,processedIn * 5.0);
            outputs[OUTPUT_L+c].setVoltage(out[c]);
        }
    }


    
}

