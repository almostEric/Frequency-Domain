#pragma once

#include <cstdint>
#include "cubeVertexFilterParameters.hpp"

enum FilterModels {
  FILTER_MODEL_BIQUAD
};

// one vertex of filter paramaters
struct cubeFilterModel {
    

    cubeFilterModel(std::string modelName) {
        this->modelName = modelName;
    }


  std::string modelName;
  cubeVertexFilterParameters vertex[2][2][2];
  int filterModel[7] = {FILTER_MODEL_BIQUAD,FILTER_MODEL_BIQUAD,FILTER_MODEL_BIQUAD,FILTER_MODEL_BIQUAD,FILTER_MODEL_BIQUAD,FILTER_MODEL_BIQUAD,FILTER_MODEL_BIQUAD}; 
  int filterNonlinearityStructure[7] = {NLBQ_NONE,NLBQ_NONE,NLBQ_NONE,NLBQ_NONE,NLBQ_NONE,NLBQ_NONE,NLBQ_NONE}; 
  int filterNonlinearityFunction[7] = {NLFC_CUBIC_SOFT_CLIP,NLFC_CUBIC_SOFT_CLIP,NLFC_CUBIC_SOFT_CLIP,NLFC_CUBIC_SOFT_CLIP,NLFC_CUBIC_SOFT_CLIP,NLFC_CUBIC_SOFT_CLIP,NLFC_CUBIC_SOFT_CLIP}; 
  int filterType[7] = {bq_type_lowpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass}; 
  int filterLevel[7] = {-1,-1,-1,-1,-1,-1,-1}; 
  
};
