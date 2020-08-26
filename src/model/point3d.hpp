#pragma once

#include <cstdint>

// 3d point
struct point3d {
  point3d(uint16_t fileId = 0, uint16_t id = 0, float x = 0.0, float y = 0.0, float z = 0.0) {
    this->fileId = fileId;
    this->id = id;
    this->x = x;
    this->y = y;
    this->z = z;

    this->xRotated = x;
    this->yRotated = y;
    this->zRotated = z;
  }

  uint16_t fileId;
  uint16_t id;
  float x;
  float y;
  float z;

  float xRotated;
  float yRotated;
  float zRotated;
};
