#pragma once

#include "shadow.hpp"

struct BaseKnob : app::SvgKnob {
protected:
  Shadow shadow = Shadow();

public:
  BaseKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}

  void setSVG(std::shared_ptr<Svg> svg) {
    app::SvgKnob::setSvg(svg);

    shadow.setBox(box);
  }

  void draw(const DrawArgs &args) override {
    /** shadow */
    shadow.draw(args.vg);

    /** component */
    ParamWidget::draw(args);

    NVGcolor icol = nvgRGBAf(0.84f, 0.84f, 0.84f, 0.0);
    NVGcolor ocol = nvgRGBAf(0.1f, 0.1f, 0.1f, 0.5);
    NVGpaint paint = nvgLinearGradient(args.vg, 0, 0, box.size.x, box.size.y, icol, ocol);
    nvgBeginPath(args.vg);
    nvgFillPaint(args.vg, paint);
    nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, box.size.x / 2);
    nvgClosePath(args.vg);
    nvgFill(args.vg);
  }
};

struct LightKnob : BaseKnob {
  LightKnob() {
    minAngle = -0.68*M_PI;
    maxAngle = 0.68*M_PI;

    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/knob.svg")));
    shadow.setBox(box);
    shadow.setSize(0.8);
    shadow.setStrength(0.2);
    shadow.setShadowPosition(2, 3.5);
  }
};

struct LightKnobFull : BaseKnob {
  LightKnobFull() {
    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/knob.svg")));
  }
};

struct LightKnobSnap : LightKnob {
  LightKnobSnap() {
    snap = true;
  }
};

struct LightKnobFullSnap : LightKnobFull {
  LightKnobFullSnap() {
    snap = true;
  }
};

struct LightSmallKnob : BaseKnob {
  LightSmallKnob() {
    minAngle = -0.68*M_PI;
    maxAngle = 0.68*M_PI;

    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/knob_small.svg")));
    shadow.setBox(box);
    shadow.setSize(0.4);
    shadow.setStrength(0.2);
    shadow.setShadowPosition(1, 1.75);
  }
};

struct LightSmallKnobSnap : BaseKnob {
  LightSmallKnobSnap() {
    snap = true;
    minAngle = -0.68*M_PI;
    maxAngle = 0.68*M_PI;

    setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/knob_small.svg")));
    shadow.setBox(box);
    shadow.setSize(0.4);
    shadow.setStrength(0.2);
    shadow.setShadowPosition(1, 1.75);
  }
};