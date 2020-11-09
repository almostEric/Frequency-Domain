
/**
 * Simple cubic soft-clipping nonlinearity.
 * For more information: https://ccrma.stanford.edu/~jos/pasp/Cubic_Soft_Clipper.html
 */
template <typename T>
T cubicSoftClip(T x, T drive) {
    T cx = std::max(std::min(drive * x, (T) 1.0), (T) -1.0);
    T y = std::max(std::min(cx - cx*cx*cx / (T) 3, (T) 2.0/3.0), (T) -2.0/3.0) / drive;
    return y;
}

template <typename T>
T hardClip(T x, T drive) {
    x = std::max(std::min(drive * x, (T) 1.0), (T) -1.0);
        // fprintf(stderr, "hard clip in:%f  drive:%f   \n",x,drive);
    return x / drive;
}

template <typename T>
T tanhClip(T x, T drive) {
    return std::tanh(drive * x) / drive;
}

template <typename T>
T doubleSoftClip(T x, T drive) {
    x = std::max(std::min(drive * x, (T) 1.0), (T) -1.0);
    //T in = x;
    T u = x == 0.0 ? 0.0 : (x > 0 ? x - 0.5 : x + 0.5);
    x = 0.75 * (u - u*u*u / (T) 3);
    x = u == 0.0 ? 0.0 : (u > 0 ? x + 0.5 : x - 0.5);
    //fprintf(stderr, "double soft clip in:%f drive:%f out%f  \n",in,drive,x);
    return x / drive;
    // return x;
}


// derivative of saturating NL function, needed for visualizer
template <typename T>
//inline T clipDeriv(T x) const noexcept {
T clipDeriv(T x) noexcept {
    T th = std::tanh(x);
    return (T) 1 - th*th;
}
