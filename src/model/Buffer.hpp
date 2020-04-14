#pragma once

#include <cstdint>

// circular buffers for managing state data
template <typename T>
struct Buffer {
  Buffer(uint16_t size, uint16_t initialPosition) {
    data = new T[size]{0};
    for (uint16_t i = 0; i < size; i++) {
      data[i] = 0;
    }
    looped = false;
    if(initialPosition >= size) 
      initialPosition -=size;
    setPos = initialPosition;
    getPos = initialPosition;

    this->size = size;
  }

  ~Buffer() {
    delete data;
  }

  void set(T value) {
    data[setPos] = value;

    setPos++;

    if (setPos >= size) {
      setPos = 0;
      looped = true;
    } else if (setPos == 1) {
      looped = false;
    }
  }

  T get() {
    T value = data[getPos];
    getPos++;

    if (getPos >= size) {
      getPos = 0;
    }

    return value;
  }

  T *data;
  uint16_t size;
  uint16_t setPos;
  uint16_t getPos;
  bool looped;
};
