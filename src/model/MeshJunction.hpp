template <typename T>
struct MeshJunction {
    

    T RectilinearJunction(T in1, T in2, T in3, T in4) {
        this->in1 = in1;
        this->in2 = in2;
        this->in3 = in3;
        this->in4 = in4;
        currentValue = (in1 + in2 + in3 + in4 + externalInput) / 2.0;
        if(currentValue < -1.0) {
            currentValue = -1.0;
        } else if(currentValue > 1.0) {
            currentValue = 1.0;
        }
        externalInput = 0.0; //Clear out
        return currentValue;
    }

    void AddExternalInput(T extIn) {
        this->externalInput += extIn;
    }

    T TriangularJunction(T in1, T in2, T in3,T in4, T in5, T in6) {
        this->in1 = in1;
        this->in2 = in2;
        this->in3 = in3;
        this->in4 = in4;
        this->in5 = in5;
        this->in5 = in5;
        currentValue = (in1 + in2 + in3 + in4 + in5 + in6 + externalInput) / 3.0;
        if(currentValue < -1.0) {
            currentValue = -1.0;
        } else if(currentValue > 1.0) {
            currentValue = 1.0;
        }
        externalInput = 0.0; //Clear out
        return currentValue;
    }

    T CubicJunction(T in1, T in2, T in3,T in4, T in5, T in6) {
        this->in1 = in1;
        this->in2 = in2;
        this->in3 = in3;
        this->in4 = in4;
        this->in5 = in5;
        this->in5 = in5;
        currentValue = (in1 + in2 + in3 + in4 + in5 + in6 + externalInput) / 3.0;
        if(currentValue < -1.0) {
            currentValue = -1.0;
        } else if(currentValue > 1.0) {
            currentValue = 1.0;
        }
        externalInput = 0.0; //Clear out
        return currentValue;
    }


    T externalInput = 0.0;
    T currentValue, impedance;
    T in1, in2, in3, in4, in5, in6;
};