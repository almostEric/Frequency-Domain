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
  std::string gridName = "";

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
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
      createContextMenu();
      e.consume(this);
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

    void createContextMenu() {
		ui::Menu* menu = createMenu();
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, gridName.c_str()));

    DrawShapeMenuItem *drawMaxShape = new DrawShapeMenuItem();
		drawMaxShape->text = "All Maximum";// 
		drawMaxShape->cells = cells;
		drawMaxShape->shape= 0;
		menu->addChild(drawMaxShape);

    DrawShapeMenuItem *drawMinShape = new DrawShapeMenuItem();
		drawMinShape->text = "All Minimum";// 
		drawMinShape->cells = cells;
		drawMinShape->shape= 1;
		menu->addChild(drawMinShape);

    DrawShapeMenuItem *drawMediumShape = new DrawShapeMenuItem();
		drawMediumShape->text = "All Half";// 
		drawMediumShape->cells = cells;
		drawMediumShape->shape= 2;
		menu->addChild(drawMediumShape);

    DrawShapeMenuItem *drawTriangleShape = new DrawShapeMenuItem();
		drawTriangleShape->text = "Triangle";// 
		drawTriangleShape->cells = cells;
		drawTriangleShape->shape= 3;
		menu->addChild(drawTriangleShape);

    DrawShapeMenuItem *drawSinShape = new DrawShapeMenuItem();
		drawSinShape->text = "Sine";// 
		drawSinShape->cells = cells;
		drawSinShape->shape= 4;
		menu->addChild(drawSinShape);

    DrawShapeMenuItem *drawRampShape = new DrawShapeMenuItem();
		drawRampShape->text = "Ramp";// 
		drawRampShape->cells = cells;
		drawRampShape->shape= 5;
		menu->addChild(drawRampShape);

    DrawShapeMenuItem *drawRandomShape = new DrawShapeMenuItem();
		drawRandomShape->text = "Random";// 
		drawRandomShape->cells = cells;
		drawRandomShape->shape= 6;
		menu->addChild(drawRandomShape);

    ChangeShapeMenuItem *flipHorizontal = new ChangeShapeMenuItem();
		flipHorizontal->text = "Flip Horizontal";// 
		flipHorizontal->cells = cells;
		flipHorizontal->flipDirection=-1;
		menu->addChild(flipHorizontal);

    ChangeShapeMenuItem *flipVertical = new ChangeShapeMenuItem();
		flipVertical->text = "Flip Vertical";// 
		flipVertical->cells = cells;
		flipVertical->flipDirection=1;
		menu->addChild(flipVertical);

    ChangeShapeMenuItem *reduceByHalf = new ChangeShapeMenuItem();
		reduceByHalf->text = "Reduce By Half";// 
		reduceByHalf->cells = cells;
		reduceByHalf->reductionAmount=0.5;
		menu->addChild(reduceByHalf);

    ChangeShapeMenuItem *shiftLeft = new ChangeShapeMenuItem();
		shiftLeft->text = "Shift Left";// 
		shiftLeft->cells = cells;
		shiftLeft->shiftDirection=1;
		menu->addChild(shiftLeft);

    ChangeShapeMenuItem *shiftRight = new ChangeShapeMenuItem();
		shiftRight->text = "Shift Right";// 
		shiftRight->cells = cells;
		shiftRight->shiftDirection=-1;
		menu->addChild(shiftRight);

	}

  struct DrawShapeMenuItem : MenuItem {
    OneDimensionalCells *cells;
    uint8_t shape;

    void onAction(const event::Action& e) override {
      cells->drawShape(shape);
    }
    };

  struct ChangeShapeMenuItem : MenuItem {
    OneDimensionalCells *cells;
    int flipDirection = 0;
    int shiftDirection = 0;
    float reductionAmount =0.0;

    void onAction(const event::Action& e) override {
      cells->changeShape(flipDirection,shiftDirection,reductionAmount);
    }
  };

};

struct CellBarGrid : FramebufferWidget {
  OneDimensionalCells *cells = nullptr;
  float initX = 0;
  float initY = 0;
  float dragX = 0;
  float dragY = 0;
  uint16_t yAxis = 0;
  std::string gridName = "";
  bool currentlyTurningOn = false;

  CellBarGrid(uint16_t yAxis = 0) {
    this->yAxis = yAxis;
    this->gridName = gridName;
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
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
      //module->selectionSet(type, id);
      createContextMenu();
      e.consume(this);
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


      // draw x axis pin line
      if (cells->pinXAxisValues > 0) {
        nvgStrokeColor(args.vg, nvgRGBA(0x1a, 0x13, 0xc7, 0xF0)); //translucent blue
        nvgStrokeWidth(args.vg, 1.0);
        nvgBeginPath(args.vg);
        float x = cells->pinXAxisPosition * (cells->width-1) * 2 + 1;
        nvgMoveTo(args.vg,x,0);
        nvgLineTo(args.vg,x,cells->height * 2);
        nvgStroke(args.vg);		
      }
    }
  }

  void createContextMenu() {
		ui::Menu* menu = createMenu();
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, gridName.c_str()));

    DrawShapeMenuItem *drawMaxShape = new DrawShapeMenuItem();
		drawMaxShape->text = "All Maximum";// 
		drawMaxShape->cells = cells;
		drawMaxShape->shape= 0;
		menu->addChild(drawMaxShape);

    DrawShapeMenuItem *drawMinShape = new DrawShapeMenuItem();
		drawMinShape->text = "All Minimum";// 
		drawMinShape->cells = cells;
		drawMinShape->shape= 1;
		menu->addChild(drawMinShape);

    DrawShapeMenuItem *drawMediumShape = new DrawShapeMenuItem();
		drawMediumShape->text = "All Half";// 
		drawMediumShape->cells = cells;
		drawMediumShape->shape= 2;
		menu->addChild(drawMediumShape);

    DrawShapeMenuItem *drawTriangleShape = new DrawShapeMenuItem();
		drawTriangleShape->text = "Triangle";// 
		drawTriangleShape->cells = cells;
		drawTriangleShape->shape= 3;
		menu->addChild(drawTriangleShape);

    DrawShapeMenuItem *drawSinShape = new DrawShapeMenuItem();
		drawSinShape->text = "Sine";// 
		drawSinShape->cells = cells;
		drawSinShape->shape= 4;
		menu->addChild(drawSinShape);

    DrawShapeMenuItem *drawRampShape = new DrawShapeMenuItem();
		drawRampShape->text = "Ramp";// 
		drawRampShape->cells = cells;
		drawRampShape->shape= 5;
		menu->addChild(drawRampShape);

    DrawShapeMenuItem *drawRandomShape = new DrawShapeMenuItem();
		drawRandomShape->text = "Random";// 
		drawRandomShape->cells = cells;
		drawRandomShape->shape= 6;
		menu->addChild(drawRandomShape);

    ChangeShapeMenuItem *flipHorizontal = new ChangeShapeMenuItem();
		flipHorizontal->text = "Flip Horizontal";// 
		flipHorizontal->cells = cells;
		flipHorizontal->flipDirection=-1;
		menu->addChild(flipHorizontal);

    ChangeShapeMenuItem *flipVertical = new ChangeShapeMenuItem();
		flipVertical->text = "Flip Vertical";// 
		flipVertical->cells = cells;
		flipVertical->flipDirection=1;
		menu->addChild(flipVertical);

    ChangeShapeMenuItem *reduceByHalf = new ChangeShapeMenuItem();
		reduceByHalf->text = "Reduce By Half";// 
		reduceByHalf->cells = cells;
		reduceByHalf->reductionAmount=0.5;
		menu->addChild(reduceByHalf);

    ChangeShapeMenuItem *shiftLeft = new ChangeShapeMenuItem();
		shiftLeft->text = "Shift Left";// 
		shiftLeft->cells = cells;
		shiftLeft->shiftDirection=1;
		menu->addChild(shiftLeft);

    ChangeShapeMenuItem *shiftRight = new ChangeShapeMenuItem();
		shiftRight->text = "Shift Right";// 
		shiftRight->cells = cells;
		shiftRight->shiftDirection=-1;
		menu->addChild(shiftRight);

	}

  struct DrawShapeMenuItem : MenuItem {
    OneDimensionalCells *cells;
    uint8_t shape;

    void onAction(const event::Action& e) override {
      cells->drawShape(shape);
    }
    };

  struct ChangeShapeMenuItem : MenuItem {
    OneDimensionalCells *cells;
    int flipDirection = 0;
    int shiftDirection = 0;
    float reductionAmount =0.0;

    void onAction(const event::Action& e) override {
      cells->changeShape(flipDirection,shiftDirection,reductionAmount);
    }
  };

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
      nvgFillColor(args.vg, nvgRGBA(0xca, 0xc3, 0x27, 0xc0)); //yellow for now

      for (uint16_t y = 0; y < height; y++) {
        uint16_t x = clamp(graph[y] * width,0.0f,width);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, y*2, x+2, 2);
        nvgFill(args.vg);
      }
    }
  }
};

