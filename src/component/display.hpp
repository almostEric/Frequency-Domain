#pragma once

#include "../model/Cells.hpp"

struct ArcDisplay : FramebufferWidget {
  float *percentage = nullptr;
  float oldPercentage = 0;

  ArcDisplay ( ) {
    dirty = true;
  }

  void step () override {
    if (percentage && *percentage != oldPercentage) {
      dirty = true;
    } else {
      dirty = false;
    }

    FramebufferWidget::step();
  }

  void draw (const DrawArgs &args) override {
    if (percentage == nullptr) {
      return;
    }

    float minAngle = -3.65;
    float maxAngle =  0.55;
    float endAngle = ((maxAngle - minAngle) * *percentage) + minAngle;
    nvgBeginPath(args.vg);
    nvgStrokeColor(args.vg, nvgRGBA(0xc8, 0xa1, 0x29, 0xff));
    nvgStrokeWidth(args.vg, 2.0);
    nvgArc(args.vg, 11, 11, 22, minAngle, endAngle, NVG_CW);
    nvgStroke(args.vg);
  }
};

struct BidirectionalArcDisplay : ArcDisplay {

  void draw (const DrawArgs &args) override {
    if (percentage == nullptr) {
      return;
    }

    float minAngle = -3.65;
    float midAngle = -1.55;
    float maxAngle =  0.55;
    float endAngle = ((maxAngle - minAngle) * *percentage / 2.0) + midAngle;
    nvgBeginPath(args.vg);
    nvgStrokeColor(args.vg, nvgRGBA(0xc8, 0xa1, 0x29, 0xff));
    nvgStrokeWidth(args.vg, 2.0);
    nvgArc(args.vg, 11, 11, 22, midAngle, endAngle, endAngle < midAngle ? NVG_CCW : NVG_CW);
    nvgStroke(args.vg);
  }
};

struct SmallArcDisplay : ArcDisplay {

  void draw (const DrawArgs &args) override {
    if (percentage == nullptr) {
      return;
    }

    float minAngle = -3.65;
    float maxAngle =  0.55;
    float endAngle = ((maxAngle - minAngle) * *percentage) + minAngle;
    nvgBeginPath(args.vg);
    nvgStrokeColor(args.vg, nvgRGBA(0xc8, 0xa1, 0x29, 0xff));
    nvgStrokeWidth(args.vg, 1.2);
    nvgArc(args.vg, 5.5, 5.5, 11, minAngle, endAngle, NVG_CW);
    nvgStroke(args.vg);
  }
};


struct SmallBidirectionalArcDisplay : ArcDisplay {

  void draw (const DrawArgs &args) override {
    if (percentage == nullptr) {
      return;
    }

    float minAngle = -3.65;
    float midAngle = -1.55;
    float maxAngle =  0.55;
    float endAngle = ((maxAngle - minAngle) * *percentage / 2.0) + midAngle;
    nvgBeginPath(args.vg);
    nvgStrokeColor(args.vg, nvgRGBA(0xc8, 0xa1, 0x29, 0xff));
    nvgStrokeWidth(args.vg, 1.2);
    nvgArc(args.vg, 5.5, 5.5, 11, midAngle, endAngle, endAngle < midAngle ? NVG_CCW : NVG_CW);
    nvgStroke(args.vg);
  }
};

struct DelayRangeDisplay : FramebufferWidget {
	std::shared_ptr<Font> font;
  float  *delayRangeTime = nullptr;
	
  DelayRangeDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/routed-gothic.ttf"));
	}


	void draw(const DrawArgs &args) override {

    if (delayRangeTime == nullptr) {
      return;
    }

		nvgFontSize(args.vg, 18);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 0);

		nvgFillColor(args.vg, nvgRGBA(0x4a, 0xc3, 0x27, 0xff));
		char text[128];
		snprintf(text, sizeof(text), "%2.1f", *delayRangeTime);
    strcat(text, "s");
		nvgTextAlign(args.vg,NVG_ALIGN_RIGHT);
		nvgText(args.vg, 0, 0, text, NULL);
	}
};




struct CellGrid : FramebufferWidget {
  OneDimensionalCells *cells = nullptr;
  float initX = 0;
  float initY = 0;
  float dragX = 0;
  float dragY = 0;
  bool currentlyTurningOn = false;

  CellGrid() {
    dirty = true;
  }

  void step () override {
    if (cells && (*cells).dirty) {
      dirty = true;
      (*cells).dirty = false;
    } else {
      dirty = false;
    }

    FramebufferWidget::step();
  }

  void onButton(const event::Button &e) override {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);

      if (cells) {
        initX = e.pos.x;
        initY = e.pos.y;
        currentlyTurningOn = !cells->active(e.pos.x / 2.0, e.pos.y / 2.0);
        cells->setCell(e.pos.x / 2.0, e.pos.y / 2.0);
      }
    }
  }

  void onDragStart(const event::DragStart &e) override {
    dragX = APP->scene->rack->mousePos.x;
    dragY = APP->scene->rack->mousePos.y;
  }

  void onDragMove(const event::DragMove &e) override {
    float newDragX = APP->scene->rack->mousePos.x;
    float newDragY = APP->scene->rack->mousePos.y;

    cells->setCell((initX+(newDragX-dragX)) / 2.0, (initY+(newDragY-dragY)) / 2.0);
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (cells) {
      nvgFillColor(args.vg, nvgRGB(0x3a, 0xa3, 0x27)); //CRT Green

      for (uint16_t y = 0; y < cells->height; y++) {
        uint16_t x = cells->displayValueForPosition(y);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, x * 2, y * 2, 2, 2);
        nvgFill(args.vg);
      }
    }
  }
};

struct CellBarGrid : FramebufferWidget {
  OneDimensionalCells *cells = nullptr;
  float initX = 0;
  float initY = 0;
  float dragX = 0;
  float dragY = 0;
  uint16_t yAxis = 0;
  bool currentlyTurningOn = false;

  CellBarGrid(uint16_t yAxis = 0) {
    this->yAxis = yAxis;
  }

  void step () override {
    if (cells && (*cells).dirty) {
      dirty = true;
      (*cells).dirty = false;
    } else {
      dirty = false;
    }

    FramebufferWidget::step();
  }

  void onButton(const event::Button &e) override {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
      e.consume(this);

      if (cells) {
        initX = e.pos.x;
        initY = e.pos.y;
        currentlyTurningOn = !cells->active(e.pos.x, e.pos.y);
        cells->setCell(e.pos.x / 2.0, e.pos.y / 2.0);
      }
    }
  }

  void onDragStart(const event::DragStart &e) override {
    dragX = APP->scene->rack->mousePos.x;
    dragY = APP->scene->rack->mousePos.y;
  }

  void onDragMove(const event::DragMove &e) override {
    float newDragX = APP->scene->rack->mousePos.x;
    float newDragY = APP->scene->rack->mousePos.y;

    cells->setCell((initX+(newDragX-dragX)) / 2.0, (initY+(newDragY-dragY)) / 2.0);
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (cells) {
      nvgFillColor(args.vg, nvgRGB(0x3a, 0x73, 0x27)); //crt green

      for (uint16_t y = 0; y < cells->height; y++) {
        uint16_t x = cells->displayValueForPosition(y);
        nvgBeginPath(args.vg);
        int16_t sizeOffset = x*2 >= yAxis ? 2 : -2;
        int16_t placeOffset = x*2 >= yAxis ? 0 : 2;
        nvgRect(args.vg, yAxis + placeOffset, y*2, x*2 + sizeOffset-yAxis, 2);
        nvgFill(args.vg);
      }
    }
  }
};


struct DisplayBarGrid : FramebufferWidget {
  float *graph= nullptr;

  uint16_t height;
  float width;

  DisplayBarGrid(float width, uint16_t height) {
    this->width = width;
    this->height = height;
  }


  void draw(const DrawArgs &args) override {
    if (graph) {
      nvgFillColor(args.vg, nvgRGBA(0xca, 0xc3, 0x27, 0x80)); //yellow for now

      for (uint16_t y = 0; y < height; y++) {
        uint16_t x = clamp(graph[y] * width,0.0f,width);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, y*2, x+2, 2);
        nvgFill(args.vg);
      }
    }
  }
};

