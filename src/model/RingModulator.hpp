struct RingModulator {

    float voltageBias = 0;
	float voltageLinear = 0.5;
	float h = 1; //Slope
	float nl = 2.0; //Non-Linearity

    float diode_sim(float inVoltage) {
	  	if( inVoltage <= voltageBias )
	  		return 0;

	    if( inVoltage <= voltageLinear) {
	    	return h * (inVoltage - voltageBias) * (inVoltage - voltageBias) / ((nl * voltageLinear) - (nl * voltageBias));
	    } else {
	    	return (h * inVoltage) - (h * voltageLinear) + (h * ((voltageLinear - voltageBias) * (voltageLinear - voltageBias) / ((nl * voltageLinear) - (nl * voltageBias))));
	    }
	  }


    float processModel(float vIn, float vC) {
        float A = 0.5 * vIn + vC;
        float B = vC - 0.5 * vIn;

        float dPA = diode_sim( A );
        float dMA = diode_sim( -A );
        float dPB = diode_sim( B );
        float dMB = diode_sim( -B );

        float result = dPA + dMA - dPB - dMB;

        return result;
    }


};
