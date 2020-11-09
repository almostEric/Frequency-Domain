struct individualFilterParameters {

    individualFilterParameters() {
    }

    float Fc;
    float Q;
    float drive;
    float gain;
    float feedforwardAmount;
    float feedbackAmount;
    float feedforwardGain;
    float feedbackGain;
};

struct cubeVertexFilterParameters {

  float makeupGain;
  individualFilterParameters filterParameters[7];


};

