#define NBR_WINDOW_FUNCTIONS 8

enum WindowFunctionTypes {
    NO_WINDOW_FUNCTION,
    TRIANGLE_WINDOW_FUNCTION,
    WELCH_WINDOW_FUNCTION,
    SIN_WINDOW_FUNCTION,
    HANNING_WINDOW_FUNCTION,
    BLACKMAN_WINDOW_FUNCTION,
    NUTALL_WINDOW_FUNCTION,
    KAISER_WINDOW_FUNCTION
};

template <typename T>
struct WindowFunction {

    WindowFunction(uint16_t size) {
        this->size = size;

        for (uint16_t i = 0; i < NBR_WINDOW_FUNCTIONS; i++) {
          data[i] = new T[size];
        }

        //needed for kaiser
        alpha = 7.865f;
        ii0a = 1.0f / i0(alpha);
        
        for(uint16_t i = 0; i < size; i++) {
            float phase = (float) i / (float) size;
            for(int j=0;j<NBR_WINDOW_FUNCTIONS;j++) {
                T windowValue = process(j, i, phase);
                data[j][i] = windowValue;
                sum[j] += windowValue;
            }
        }
    }

    ~WindowFunction() {
        for (uint16_t i = 0; i < NBR_WINDOW_FUNCTIONS; i++) {
          delete[] data[i];
        }
    }


    T windowValue(int windowType,int index) {
        if (index >= size) {
          return 0;
        }
        return data[windowType][index];
    }

    T process(int windowType, uint16_t index, float phase) {
        float returnValue = 0.0;

        switch(windowType) {
            case NO_WINDOW_FUNCTION :
                returnValue = 1;
                break;
            case TRIANGLE_WINDOW_FUNCTION :
                returnValue = TriangleWindow(phase);
                break;
            case WELCH_WINDOW_FUNCTION :
                returnValue = WelchWindow(phase);
                break;
            case SIN_WINDOW_FUNCTION :
                returnValue = SinWindow(phase);
                break;
            case HANNING_WINDOW_FUNCTION :
                returnValue = HanningWindow(phase);
                break;
            case BLACKMAN_WINDOW_FUNCTION :
                returnValue = BlackmanWindow(phase);
                break;
            case NUTALL_WINDOW_FUNCTION :
                returnValue = NutallWindow(phase);
                break;
            case KAISER_WINDOW_FUNCTION :
                returnValue = KaiserWindow(index);
                break;
            default:
                break;
        }

        return returnValue;
    }

    T TriangleWindow(float phase) {
        return 1- abs((phase - 0.5)/0.5);
    }

    // float ParzenWindow(float phase) {
    //     float w0 = n <
    //     return 1- abs((phase - 0.5)/0.5);
    // }

    T WelchWindow(float phase) {
        return 1- powf((phase - 0.5)  / 0.55, 2.0);
    }

    T SinWindow(float phase) {
        return sinf(2 * M_PI * phase);
    }

	T HanningWindow(float phase) {
		return 0.5f * (1 - cosf(2 * M_PI * phase));
	}

	T BlackmanWindow(float phase) {
		float a0 = 0.42;
		float a1 = 0.5;
		float a2 = 0.08;
		return a0 - (a1 * cosf(2 * M_PI * phase)) + (a2 * cosf(4 * M_PI * phase));
	}

    T NutallWindow(float phase) {
		float a0 = 0.355768;
		float a1 = 0.487396;
		float a2 = 0.144232;
		float a3 = 0.012604;
		return a0 - (a1 * cosf(2 * M_PI * phase)) + (a2 * cosf(4 * M_PI * phase)) - (a3 * cosf(6 * M_PI * phase));
	}

    T KaiserWindow(uint16_t index) {
        assert(index < size);
		float ism1 = 1.0f / (float(size) - 1);        
        float x = float(index) * 2.0f;
        x *= ism1;
        x -= 1.0f;
        x *= x;
        x = 1.0f - x;
        x = sqrtf(x);
        x *= alpha;
        return i0(x) * ii0a;
    }

    // Rabiner, Gold: "The Theory and Application of Digital Signal Processing", 1975, page 103.
    float i0(float x) {
        //fprintf(stderr, "%f => %hu \n", x, size);
        assert(x >= 0.0f);
        assert(x < 20.0f);
        float y = 0.5f * x;
        float t = .1e-8f;
        float e = 1.0f;
        float de = 1.0f;
        for (int i = 1; i <= 25; ++i) {
            de = de * y / (float)i;
            float sde = de * de;
            e += sde;
            if (e * t - sde > 0.0f) {
                break;
            }
        }
        return e;
    }

    T *data[NBR_WINDOW_FUNCTIONS] = { 0 };
    T sum[NBR_WINDOW_FUNCTIONS] = {0};
    uint16_t size;

    //These are used by the Kaiser Window Function, don't need to recalc every time
    float alpha;
    float ii0a;
    
    std::string windowFunctionName[NBR_WINDOW_FUNCTIONS] = {"None","Triangle","Welch","Sin","Hanning","Blackman","Nutall","Kaiser"};

};
