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
  float cellWidth = 2.0;
  float cellHeight = 2.0;
  bool currentlyTurningOn = false;
  bool setRange = false;
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
        currentlyTurningOn = !cells->active(e.pos.x / cellWidth, e.pos.y / cellHeight);
        cells->setCell(e.pos.x / cellWidth, e.pos.y / cellHeight,setRange);
      }
    }
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
      createContextMenu();
      e.consume(this);
    }
  }

  void onDragStart(const event::DragStart &e) override {
    dragX = APP->scene->rack->getMousePos().x;
    dragY = APP->scene->rack->getMousePos().y;
  }

  void onDragMove(const event::DragMove &e) override {
    float newDragX = APP->scene->rack->getMousePos().x;
    float newDragY = APP->scene->rack->getMousePos().y;
    int mods = APP->window->getMods();

		if ((mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
      newDragX = dragX;
    }

    cells->setCell((initX+(newDragX-dragX)) / cellWidth, (initY+(newDragY-dragY)) / cellHeight,setRange);
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (cells) {          
      for (uint16_t y = 0; y < cells->height; y++) {
        nvgFillColor(args.vg, cells->cellColor[y]); 
        uint16_t x = cells->displayValueForPosition(y);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, x * cellWidth, y * cellHeight, cellWidth, cellHeight);
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

struct CellRangeGrid : FramebufferWidget {
  OneDimensionalCells *cells = nullptr;
  float initX = 0;
  float initY = 0;
  float dragX = 0;
  float dragY = 0;
  float cellWidth = 2.0;
  float cellHeight = 2.0;
  bool currentlyTurningOn = false;
  bool setRange = false;
  std::string gridName = "";

  CellRangeGrid() {
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
        currentlyTurningOn = !cells->active(e.pos.x / cellWidth, e.pos.y / cellHeight);

        setRange = ((e.mods & RACK_MOD_MASK) == (GLFW_MOD_SHIFT));
          
        cells->setCell(e.pos.x / cellWidth, e.pos.y / cellHeight, setRange);
      }
    }
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
      createContextMenu();
      e.consume(this);
    }
  }

  void onDragStart(const event::DragStart &e) override {
    dragX = APP->scene->rack->getMousePos().x;
    dragY = APP->scene->rack->getMousePos().y;
  }

  void onDragMove(const event::DragMove &e) override {
    float newDragX = APP->scene->rack->getMousePos().x;
    float newDragY = APP->scene->rack->getMousePos().y;
    int mods = APP->window->getMods();

		if ((mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
      newDragX = dragX;
    }

    cells->setCell((initX+(newDragX-dragX)) / cellWidth, (initY+(newDragY-dragY)) / cellHeight,setRange);
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (cells) {          
      uint16_t cw = cells->width - 1;
      for (uint16_t y = 0; y < cells->height; y++) {
        uint16_t x = cells->displayValueForPosition(y);
        float rx = cells->extendedValueForPosition(y) * cw;

       //fprintf(stderr, "range cell %u %u %f \n",y,x,rx);

        float leftRange = rx;
        if(x-rx < 0)
          leftRange = x;
        float rightRange = rx;
        if(x+rx > cw)
          rightRange = cw-x;
        
        float rectWidth = leftRange + rightRange + 1;
        //rectWidth = std::min(rectWidth,float(cells->width) / 2.0f - x);
        nvgBeginPath(args.vg);
        nvgFillColor(args.vg, nvgRGBA(0x3a, 0xa3, 0x27,0x80)); 
        nvgRect(args.vg, std::max(x-rx,0.0f) * cellWidth, y * cellHeight, rectWidth * cellWidth, cellHeight);
        nvgFill(args.vg);
        nvgClosePath(args.vg);
        nvgBeginPath(args.vg);
        nvgFillColor(args.vg, cells->cellColor[y]); 
        nvgRect(args.vg, x * cellWidth, y * cellHeight, cellWidth, cellHeight);
        nvgFill(args.vg);
        nvgClosePath(args.vg);
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


    menu->addChild(new MenuLabel());// empty line

    ResetRangeMenuItem *resetRange = new ResetRangeMenuItem();
		resetRange->text = "Reset Range";// 
		resetRange->cells = cells;
		menu->addChild(resetRange);
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

  struct ResetRangeMenuItem : MenuItem {
    OneDimensionalCells *cells;

    void onAction(const event::Action& e) override {
      cells->resetRange();
    }
  };

};


struct CellBarGrid : FramebufferWidget {
  OneDimensionalCells *cells = nullptr;
  float initX = 0;
  float initY = 0;
  float dragX = 0;
  float dragY = 0;
  float startDragX = 0;
  float cellWidth = 2.0;
  float cellHeight = 2.0;
  uint16_t yAxis = 0;
  std::string gridName = "";
  bool currentlyTurningOn = false;
  bool setRange = false;

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

        currentlyTurningOn = !cells->active(e.pos.x / cellWidth, e.pos.y / cellHeight);        
        cells->setCell(e.pos.x / cellWidth, e.pos.y / cellHeight,setRange);
      }
    }
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
      createContextMenu();
      e.consume(this);
    }
  }

  void onDragStart(const event::DragStart &e) override {
    dragX = APP->scene->rack->getMousePos().x;
    dragY = APP->scene->rack->getMousePos().y;
  }

  void onDragMove(const event::DragMove &e) override {
    float newDragX = APP->scene->rack->getMousePos().x;
    float newDragY = APP->scene->rack->getMousePos().y;
    int mods = APP->window->getMods();

		if ((mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
      newDragX = dragX;
    }

    cells->setCell((initX+(newDragX-dragX)) / cellWidth, (initY+(newDragY-dragY)) / cellHeight,setRange);
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (cells) {
      for (uint16_t y = 0; y < cells->height; y++) {
        nvgFillColor(args.vg, cells->cellColor[y]); 
        uint16_t x = cells->displayValueForPosition(y);
        nvgBeginPath(args.vg);
        int16_t sizeOffset = x*cellWidth >= yAxis ? cellWidth : -cellWidth;
        int16_t placeOffset = x*cellWidth >= yAxis ? 0 : cellWidth;
        nvgRect(args.vg, yAxis + placeOffset, y*cellHeight, x*cellWidth + sizeOffset-yAxis, cellHeight);
        nvgFill(args.vg);
      }


      // draw x axis pin line
      if (cells->pinXAxisValues > 0) {
        nvgStrokeColor(args.vg, nvgRGBA(0x1a, 0x13, 0xc7, 0xF0)); //translucent blue
        nvgStrokeWidth(args.vg, 1.0);
        nvgBeginPath(args.vg);
        float x = cells->pinXAxisPosition * (cells->width-1) * cellWidth + 1;
        nvgMoveTo(args.vg,x,0);
        nvgLineTo(args.vg,x,cells->height * cellHeight);
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

struct CellVerticalBarGrid : FramebufferWidget {
  OneDimensionalCells *cells = nullptr;
  float initX = 0;
  float initY = 0;
  float dragX = 0;
  float dragY = 0;
  float cellWidth = 2.0;
  float cellHeight = 2.0;
  uint16_t xAxis = 0;
  std::string gridName = "";
  bool currentlyTurningOn = false;
  bool setRange = false;

  CellVerticalBarGrid(uint16_t xAxis = 0) {
    this->xAxis = xAxis;
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

      //  fprintf(stderr, "mouse values: %f %f   %f  %f   %u %u  \n",e.pos.x,e.pos.y,cellHeight,cellWidth,cells->height,cells->width);


        currentlyTurningOn = !cells->active(cells->width - 0 - (e.pos.y / cellHeight), std::floor(cells->height - 1 - (e.pos.x / cellWidth)));
        cells->setCell(cells->width - 0 - (e.pos.y / cellHeight),std::floor(cells->height - 1 - (e.pos.x / cellWidth)),setRange);


      }
    }
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
      createContextMenu();
      e.consume(this);
    }
  }

  void onDragStart(const event::DragStart &e) override {
    dragX = APP->scene->rack->getMousePos().x;
    dragY = APP->scene->rack->getMousePos().y;
  }

  void onDragMove(const event::DragMove &e) override {
    float newDragX = APP->scene->rack->getMousePos().x;
    float newDragY = APP->scene->rack->getMousePos().y;
    int mods = APP->window->getMods();

		if ((mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
      newDragY = dragY;
    }

    cells->setCell( (cells->width - 0 - (initY+(newDragY-dragY)) / cellHeight),std::floor(cells->height - 1 - ((initX+(newDragX-dragX)) / cellWidth)),setRange);    
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (cells) {

      for (uint16_t x = 0; x < cells->height; x++) {
        nvgFillColor(args.vg, cells->cellColor[x]); 
        uint16_t y = cells->width - 1 - cells->displayValueForPosition(cells->height - 1 - x);
        nvgBeginPath(args.vg);
        int16_t sizeOffset = y*cellHeight >= xAxis ? -cellHeight : cellHeight;
        int16_t placeOffset = y*cellHeight >= xAxis ? 0 : cellHeight;
        nvgRect(args.vg,x*cellWidth, xAxis + placeOffset , cellWidth, y*cellHeight + sizeOffset-xAxis);
        nvgFill(args.vg);
      }


      // draw x axis pin line
      // if (cells->pinXAxisValues > 0) {
      //   nvgStrokeColor(args.vg, nvgRGBA(0x1a, 0x13, 0xc7, 0xF0)); //translucent blue
      //   nvgStrokeWidth(args.vg, 1.0);
      //   nvgBeginPath(args.vg);
      //   float x = cells->pinXAxisPosition * (cells->width-1) * cellWidth + 1;
      //   nvgMoveTo(args.vg,x,0);
      //   nvgLineTo(args.vg,x,cells->height * cellHeight);
      //   nvgStroke(args.vg);		
      // }
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
		flipHorizontal->flipDirection=1;
		menu->addChild(flipHorizontal);

    ChangeShapeMenuItem *flipVertical = new ChangeShapeMenuItem();
		flipVertical->text = "Flip Vertical";// 
		flipVertical->cells = cells;
		flipVertical->flipDirection=-1;
		menu->addChild(flipVertical);

    ChangeShapeMenuItem *reduceByHalf = new ChangeShapeMenuItem();
		reduceByHalf->text = "Reduce By Half";// 
		reduceByHalf->cells = cells;
		reduceByHalf->reductionAmount=0.5;
		menu->addChild(reduceByHalf);

    ChangeShapeMenuItem *shiftUp = new ChangeShapeMenuItem();
		shiftUp->text = "Shift Up";// 
		shiftUp->cells = cells;
		shiftUp->shiftDirection=1;
		menu->addChild(shiftUp);

    ChangeShapeMenuItem *shiftDown = new ChangeShapeMenuItem();
		shiftDown->text = "Shift Down";// 
		shiftDown->cells = cells;
		shiftDown->shiftDirection=-1;
		menu->addChild(shiftDown);

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


