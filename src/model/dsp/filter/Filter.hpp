template <typename T> class Filter{

public:

    virtual T process(T in);


    virtual T frequencyResponse(T frequency);

};