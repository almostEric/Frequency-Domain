#pragma once

enum RolloverModes {
    PIN_ROLLOVER_MODE,
    WRAP_AROUND_ROLLOVER_MODE,
};

struct Cells {
  virtual ~Cells() {}
  virtual void setCell(int16_t a, int16_t b) {}
  virtual bool active(uint16_t value, uint16_t position) { return false; }
  virtual void reset() { }
  bool dirty;
};

struct TwoDimensionalCells : Cells {
  TwoDimensionalCells(uint16_t width, uint16_t height) {
    this->height = height;
    this->width = width;

    cells = new bool*[height];

    for (uint16_t i = 0; i < height; i++) {
      cells[i] = new bool[width]{0};
    }
  }

  ~TwoDimensionalCells() {
    for (uint16_t i = 0; i < height; i++) {
      delete[] cells[i];
    }

    delete[] cells;
  }

  void setCell(int16_t value, int16_t position) override {
    if (position >= height || position < 0) {
      return;
    }

    // check our bounds, since the widget can pass us invalid data due to drag
    if (value >= width) {
      value = width - 1;
    } else if (value < 0) {
      value = 0;
    }

    cells[position][value] = !cells[position][value];
  }

  bool active(uint16_t value, uint16_t position) override {
    return cells[position][value];
  }

  bool **cells;
  uint16_t width;
  uint16_t height;
  float defaultValue;
};

struct OneDimensionalCells : Cells {
  OneDimensionalCells(uint16_t width, uint16_t height,float lowRange, float highRange, float defaultValue = 0.0) {
    this->width = width;
    this->height = height;
    this->lowRange = lowRange;
    this->highRange = highRange;
    this->totalRange = highRange - lowRange;
    this->defaultValue = defaultValue;

    // allocate the cells
    cells = new float[height]{defaultValue};
    dirty = true;
  }

  ~OneDimensionalCells() {
    delete[] cells;
  }

  void reset() override {
    for (uint16_t i = 0; i < height; i++) {
      cells[i] = defaultValue;
    }
  }

  float floatForValue(uint16_t value) {
    return ((value / float(width)) * totalRange) + lowRange;
  }

  uint16_t intForValue(float value) {
    return uint16_t(((value - lowRange) / totalRange) * (width-1));
  }


  virtual float valueForPosition(uint16_t position)  {
    int16_t adjustedPosition = position + (shiftY * height);
    // takes the floating point value of the cell and returns a position approximation
    if(adjustedPosition < 0) {
        adjustedPosition = 0;
    } else if(adjustedPosition >= height) {
        adjustedPosition = height - 1;
    }

    float value = cells[adjustedPosition];
    float adjustedValue = pinZeroValues && value == 0 ? 0 : value + (shiftX * totalRange);

    if (adjustedValue < lowRange) {
        adjustedValue = lowRange;
    } else if (adjustedValue > highRange) {
        adjustedValue = highRange;
    }

    return adjustedValue;
  }


  uint16_t displayValueForPosition(uint16_t position) {
  
    return intForValue(valueForPosition(position));
  }

  void setCell(int16_t value, int16_t position) override {
    if (position >= height || position < 0) {
      return;
    }

    int16_t adjustedPosition = position - (shiftY * height);
    if(adjustedPosition < 0) {
        adjustedPosition = 0;
    } else if(adjustedPosition >= height) {
        adjustedPosition = height - 1;
    }


    int16_t adjustedValue = value - (shiftX * width);
    if (adjustedValue < 0) {
        adjustedValue = 0;
    } else if (adjustedValue > width) {
        adjustedValue = width;
    }

    cells[adjustedPosition] = floatForValue(adjustedValue);

    lastPosition = position;
    lastValue = value;
    readyForExpander = true;

    dirty = true;
  }

  bool active(uint16_t value, uint16_t position) override {
    return (valueForPosition(position) == value);
  }

  float *cells;
  uint16_t width;
  uint16_t height;
  float lowRange;
  float highRange;
  float totalRange;
  float defaultValue;
  uint8_t rolloverModeX = PIN_ROLLOVER_MODE;
  uint8_t rolloverModeY = PIN_ROLLOVER_MODE;
  float shiftX = 0;
  float shiftY = 0;

  bool pinZeroValues = false;

  uint16_t lastPosition;
  uint16_t lastValue;
  bool readyForExpander = false;


};

struct OneDimensionalCellsWithRollover : OneDimensionalCells {
  OneDimensionalCellsWithRollover(uint16_t width, uint16_t height,float lowRange, float highRange, uint8_t rolloverModeX, uint8_t rolloverModeY, float defaultValue = 0.0) : OneDimensionalCells(width, height, lowRange, highRange, defaultValue) {
    this->rolloverModeX = rolloverModeX;
    this->rolloverModeY = rolloverModeY;
  }

  float valueForPosition(uint16_t position) override {
    int16_t adjustedPosition = position + (shiftY * height);
    if(adjustedPosition < 0) {
        if(rolloverModeY == WRAP_AROUND_ROLLOVER_MODE) {
          do {
            adjustedPosition +=height;
          }
          while (adjustedPosition < 0);
        } else {
          adjustedPosition = 0;  
        }
    } else if (adjustedPosition >= height) {
      if(rolloverModeY == WRAP_AROUND_ROLLOVER_MODE) {
        do {
          adjustedPosition -=height;
        }
        while(adjustedPosition >= height);
      } else {
        adjustedPosition = height -1;
      }
    }

    float value = cells[adjustedPosition];
    float adjustedValue = pinZeroValues && value == 0 ? 0 : value + (shiftX * totalRange);
    
    if (adjustedValue < lowRange) {
        adjustedValue = rolloverModeX == WRAP_AROUND_ROLLOVER_MODE ? adjustedValue + width : lowRange;
    } else if (adjustedValue > highRange) {
        adjustedValue = rolloverModeX == WRAP_AROUND_ROLLOVER_MODE ? adjustedValue - highRange : highRange;
    }

    return adjustedValue;
  }

  void setCell(int16_t value, int16_t position) override {
    if (position >= height || position < 0) {
      return;
    }

    int16_t adjustedPosition = position - (shiftY * height);
    if(adjustedPosition < 0) {
        if(rolloverModeY == WRAP_AROUND_ROLLOVER_MODE) {
          do {
            adjustedPosition +=height;
          }
          while (adjustedPosition < 0);
        } else {
          adjustedPosition = 0;  
        }
    } else if (adjustedPosition >= height) {
      if(rolloverModeY == WRAP_AROUND_ROLLOVER_MODE) {
        do {
          adjustedPosition -=height;
        }
        while(adjustedPosition >= height);
      } else {
        adjustedPosition = height -1;
      }
    }


    int16_t adjustedValue = value - (shiftX * width);
    if (adjustedValue < 0) {
        adjustedValue = rolloverModeX == WRAP_AROUND_ROLLOVER_MODE ? adjustedValue + width : 0;
    } else if (adjustedValue > width) {
        adjustedValue = rolloverModeX == WRAP_AROUND_ROLLOVER_MODE ? adjustedValue - width : width;
    }

    cells[adjustedPosition] = floatForValue(adjustedValue);

    lastPosition = position;
    lastValue = value;
    readyForExpander = true;

    dirty = true;
  }
 

  uint8_t rolloverModeX = PIN_ROLLOVER_MODE;
  uint8_t rolloverModeY = PIN_ROLLOVER_MODE;
};
