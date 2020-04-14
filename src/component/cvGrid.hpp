#define HW 2 //cell height and width

enum DrawModes {
    ANY_CELL_DRAW_MODE,
    SINGLE_X_DRAW_MODE,
    DRAW_TO_SINGLE_X_DRAW_MODE
};


struct CVGrid : Widget {
	Module *module;
	bool currentlyTurningOn = false;
	float initX = 0;
	float initY = 0;
	float dragX = 0;
	float dragY = 0;
    uint16_t shiftX = 0;
    uint16_t shiftY = 0;

    uint16_t nbrCols, nbrRows, nbrCells;
    uint8_t drawMode = 0;
    uint8_t rolloverModeX = 0;
    uint8_t rolloverModeY = 0;
    float *cells;

	CVGrid(uint16_t sizeX, uint16_t sizeY, uint8_t drawMode, uint8_t rolloverModeX, uint8_t rolloverModeY ){
        this->nbrCols = sizeX;
        this->nbrRows = sizeY;
        this->drawMode = drawMode;
        this->rolloverModeX = rolloverModeX;
        this->rolloverModeY = rolloverModeY;
        nbrCells = nbrRows * nbrCols;
        cells = (float *) malloc(sizeof(float) * sizeY * sizeX);
        memset(cells, 0, sizeof(float) * sizeY * sizeX);
    }

    ~CVGrid() {
        free(cells);
    }


    void setCellOnByDisplayPos(float displayX, float displayY, bool on){
		setCellOn(int(displayX / HW), int(displayY / HW), on);
	}

	void setCellOn(int cellX, int cellY, bool on){
		if(cellX >= 0 && cellX < nbrCols && 
		   cellY >=0 && cellY < nbrRows){
			cells[iFromXY(cellX, cellY)] = on;
            if(drawMode == SINGLE_X_DRAW_MODE) { 
                for(int i=0;i<nbrCols;i++) {
                    if(i != cellX) {
                        cells[iFromXY(i, cellY)] = 0;
                    }
                }
            } else if (drawMode == DRAW_TO_SINGLE_X_DRAW_MODE) { 
                for(int i=0;i<nbrCols;i++) {
                    if(i <= cellX) {
                        cells[iFromXY(i, cellY)] = 1;
                    } else if(i > cellX) {
                        cells[iFromXY(i, cellY)] = 0;
                    }
                }
            } 

			// colNotesCache[cellX].valid = false;
			// colNotesCache2[cellX].valid = false;
		}
	}

	bool isCellOnByDisplayPos(float displayX, float displayY){
		return isCellOn(int(displayX / HW), int(displayY / HW));
	}

	bool isCellOn(int cellX, int cellY){
		return cells[iFromXY(cellX, cellY)];
	}

	int iFromXY(int cellX, int cellY){
		return cellX + cellY * nbrCols;
	}

	int xFromI(int cellI){
		return cellI % nbrCols;
	}

	int yFromI(int cellI){
		return cellI / nbrCols;
	}

	void onButton(const event::Button &e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
			// e.target = this;
			initX = e.pos.x;
			initY = e.pos.y;
			currentlyTurningOn = !isCellOnByDisplayPos(e.pos.x, e.pos.y);
			setCellOnByDisplayPos(e.pos.x, e.pos.y, currentlyTurningOn);
		}
	}
	
	void onDragStart(const event::DragStart &e) override {
		dragX = APP->scene->rack->mousePos.x;
		dragY = APP->scene->rack->mousePos.y;
	}

	void onDragMove(const event::DragMove &e) override {
		float newDragX = APP->scene->rack->mousePos.x;
		float newDragY = APP->scene->rack->mousePos.y;
		setCellOnByDisplayPos(initX+(newDragX-dragX), initY+(newDragY-dragY), currentlyTurningOn);
	}

	void draw(const DrawArgs &args) override {
		//background
		nvgFillColor(args.vg, nvgRGB(20, 30, 33));
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFill(args.vg);

		// //grid
		// nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
		// for(int i=1;i<nbrCols;i++){
		// 	nvgStrokeWidth(args.vg, (i % 4 == 0) ? 2 : 1);
		// 	nvgBeginPath(args.vg);
		// 	nvgMoveTo(args.vg, i * HW, 0);
		// 	nvgLineTo(args.vg, i * HW, box.size.y);
		// 	nvgStroke(args.vg);
		// }
		// for(int i=1;i<nbrRows;i++){
		// 	nvgStrokeWidth(args.vg, (i % 4 == 0) ? 2 : 1);
		// 	nvgBeginPath(args.vg);
		// 	nvgMoveTo(args.vg, 0, i * HW);
		// 	nvgLineTo(args.vg, box.size.x, i * HW);
		// 	nvgStroke(args.vg);
		// }

		if(module == NULL) return;

		//cells
		nvgFillColor(args.vg, nvgRGB(185, 150, 25)); //gold
		for(int i=0;i<nbrCells;i++){
			if(cells[i]){
				int x = xFromI(i) + shiftX;
				int y = yFromI(i) + shiftY;
                if(x < 0) {
                    rolloverModeX == 1 ? x = x + nbrCols : x = 0;
                }
                if(x >= nbrCols) {
                    rolloverModeX == 1 ? x = x - nbrCols : x = nbrCols - 1;
                }
                if(y < 0) {
                    rolloverModeY == 1 ? y = y + nbrRows : y = 0;
                }
                if(y >= nbrRows) {
                    rolloverModeY == 1 ? y = y - nbrRows : x = nbrRows - 1;
                }
				nvgBeginPath(args.vg);
				nvgRect(args.vg, x * HW, y * HW, HW, HW);
				nvgFill(args.vg);
			}
		}

		nvgStrokeWidth(args.vg, 2);
	}
};