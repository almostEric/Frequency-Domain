#include "../controller/HeatOfTheMoment.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"



struct HMDisplayImpulse : FramebufferWidget {
  HeatOfTheMomentModule *module;

  HMDisplayImpulse() {
  }


  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (!module) 
      return;

    switch(module->noiseColor) {
        case NOISE_WHITE:
            nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
            break;
        case NOISE_PINK:
            nvgFillColor(args.vg, nvgRGB(0xff, 0x9c, 0xf1));
            break;
        case NOISE_RED:
            nvgFillColor(args.vg, nvgRGB(0xff, 0x00, 0x00));
            break;
        case NOISE_VIOLET:
            nvgFillColor(args.vg, nvgRGB(0x57, 0x00, 0x7f));
            break;
        case NOISE_GREY:
            nvgFillColor(args.vg, nvgRGB(0xa0, 0xa0, 0xa0));
            break;
        case NOISE_BLUE:
            nvgFillColor(args.vg, nvgRGB(0x00, 0x00, 0xff));
            break;
        case NOISE_BLACK:
            nvgFillColor(args.vg, nvgRGB(0x00, 0x00, 0x00));
            break;
        case NOISE_GAUSSIAN:
            nvgFillColor(args.vg, nvgRGB(0x20, 0xd0, 0x30));
            break;
    }

    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg,5.0,95.0);
    for(int x=0;x<91;x++) {
        double intptr;
        double innerPhase = modf(x * module->impulsePeakCount * 45.5 / 4095.0, &intptr) * 4095;
        float y = module->windowFunction->windowValue(module->windowFunctionId,x*45.5) * module->windowFunction->windowValue(module->windowFunctionId,innerPhase);
        // float y = module->windowFunction->windowValue(module->windowFunctionId,innerPhase);
    //fprintf(stderr, "Point x:%i y:%f   \n",x,y);
        nvgLineTo(args.vg,x+5,95.0 - y*90.0);
    }
    nvgLineTo(args.vg,95.0,95.0);
    nvgLineTo(args.vg,5.0,95.0);
    nvgFill(args.vg);

}

};    


struct HMDisplayWindowFunctionName : FramebufferWidget {
  HeatOfTheMomentModule *module;
  std::shared_ptr<Font> font;

  HMDisplayWindowFunctionName() {
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/routed-gothic.ttf"));
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (!module) 
      return;

    nvgFontSize(args.vg, 24);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, -0.5);
    nvgFillColor(args.vg,nvgRGB(0x1f,0xf0,0x1f)); 	
    char text[128];
    snprintf(text, sizeof(text), "%s", module->windowFunction->windowFunctionName[module->windowFunctionId].c_str());
		nvgTextAlign(args.vg,NVG_ALIGN_CENTER);
    nvgText(args.vg, 52, 16, text, NULL);      
  }
};




struct HeatOfTheMomentWidget : ModuleWidget {
  
  HeatOfTheMomentWidget(HeatOfTheMomentModule *module) {
    setModule(module);
    box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HeatOfTheMoment.svg")));


    {
      HMDisplayImpulse *dfr = new HMDisplayImpulse();    
      //if (module) {
        dfr->module = module;
      //}   
      dfr->box.pos = Vec(17.5, 23.5);
      dfr->box.size = Vec(100, 100);
      addChild(dfr);
    }

    {
      HMDisplayWindowFunctionName *dmn = new HMDisplayWindowFunctionName();    
      //if (module) {
        dmn->module = module;
      //}       
      dmn->box.pos = Vec(17.5, 132.5);
      dmn->box.size = Vec(100, 24);
      addChild(dmn);
    }

    

    addParam(createParam<LightSmallKnobSnap>(Vec(8, 170), module, HeatOfTheMomentModule::NOISE_COLOR_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->noiseColorPercentage;
      }
      c->box.pos = Vec(11.5, 173.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(24, 182), module, HeatOfTheMomentModule::NOISE_COLOR_INPUT));


    addParam(createParam<LightSmallKnobSnap>(Vec(50, 170), module, HeatOfTheMomentModule::WINDOW_FUNCTION_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->windowFunctionPercentage;
      }
      c->box.pos = Vec(53.5, 173.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(66, 182), module, HeatOfTheMomentModule::WINDOW_FUNCTION_INPUT));


    addParam(createParam<LightSmallKnobSnap>(Vec(92, 170), module, HeatOfTheMomentModule::IMPULSE_PEAK_COUNT_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->inpulsePeakCountPercentage;
      }
      c->box.pos = Vec(95.5, 173.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(109, 182), module, HeatOfTheMomentModule::IMPULSE_PEAK_COUNT_INPUT));

    addParam(createParam<LightSmallKnob>(Vec(18, 228), module, HeatOfTheMomentModule::IMPULSE_DURATION_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->impulseDurationPercentage;
      }
      c->box.pos = Vec(21.5, 231.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }  
    addInput(createInput<LightPort>(Vec(34, 240), module, HeatOfTheMomentModule::IMPULSE_DURATION_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(80, 228), module, HeatOfTheMomentModule::IMPULSE_REPEAT_FREQUENCY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->impulseRepeatPercentage;
      }
      c->box.pos = Vec(83.5, 231.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(96, 240), module, HeatOfTheMomentModule::IMPULSE_REPEAT_FREQUENCY_INPUT));


    addInput(createInput<LightPort>(Vec(10, 305), module, HeatOfTheMomentModule::GATE_INPUT));

    addOutput(createOutput<LightPort>(Vec(100, 305), module, HeatOfTheMomentModule::OUTPUT_L));

      
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  }

};

Model *modelHeatOfTheMoment = createModel<HeatOfTheMomentModule, HeatOfTheMomentWidget>("HeatOfTheMoment");
