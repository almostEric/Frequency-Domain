#pragma once

#include <cstdint>

// one vertex of fiter paramaters
struct cubeFilterPoint {
    cubeFilterPoint(float x = 0.0, float y = 0.0, float z = 0.0) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

  float x;
  float y;
  float z;

};
