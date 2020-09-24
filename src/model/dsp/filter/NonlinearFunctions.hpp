
/**
 * Simple cubic soft-clipping nonlinearity.
 * For more information: https://ccrma.stanford.edu/~jos/pasp/Cubic_Soft_Clipper.html
 */
template <typename T>
T cubicSoftClip(T x, T drive) {
    // x = std::max(std::min(drive * x, (T) 1), (T) -1);
    // return (x - x*x*x / (T) 3) / drive;
    x = drive * x;
    return std::max(std::min( (x - x*x*x / (T) 3) / drive, (T) 1), (T) -1);
}

template <typename T>
T hardClip(T x, T drive) {
    x = std::max(std::min(drive * x, (T) 1), (T) -1);
    //return x;
    return x / drive;
}

template <typename T>
T tanhClip(T x, T drive) {
    return std::tanh(drive * x) / drive;
}

template <typename T>
T doubleSoftClip(T x, T drive) {
    x = std::max(std::min(drive * x, (T) 1), (T) -1);
    //T in = x;
    T u = x == 0.0 ? 0.0 : (x > 0 ? x - 0.5 : x + 0.5);
    x = 0.75 * (u - u*u*u / (T) 3);
    x = u == 0.0 ? 0.0 : (u > 0 ? x + 0.5 : x - 0.5);
    //fprintf(stderr, "double soft clip in:%f drive:%f out%f  \n",in,drive,x);
    //return x / drive;
    return x;
}


// derivative of saturating NL function, needed for visualizer
template <typename T>
//inline T clipDeriv(T x) const noexcept {
T clipDeriv(T x) noexcept {
    T th = std::tanh(x);
    return (T) 1 - th*th;
}
