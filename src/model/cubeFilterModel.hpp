#pragma once

#include <cstdint>
#include "cubeVertexFilterParameters.hpp"
  
// one vertex of filter paramaters
struct cubeFilterModel {
    cubeFilterModel(std::string modelName) {
        this->modelName = modelName;
    }


  std::string modelName;
  cubeVertexFilterParameters vertex[2][2][2];
  int filterType[7] = {bq_type_lowpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass,bq_type_bandpass}; 
  int filterLevel[7] = {-1,-1,-1,-1,-1,-1,-1}; 
  
};
