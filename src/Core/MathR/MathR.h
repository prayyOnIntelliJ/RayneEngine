#ifndef RAYNEENGINE_MATHR_H
#define RAYNEENGINE_MATHR_H
#include <valarray>


class MathR
{
public:
    // Clamp
    template<typename T>
    static T Clamp(T value, T min, T max);
    template<typename T>
    static T Clamp01(T value);
    // Abs
    template<typename T>
    static T Abs(T value);
    // Ceil
    template<typename T>
    static T Ceil(T value);
    // Floor
    template<typename T>
    static T Floor(T value);
};

template <typename T>
T MathR::Clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

template <typename T>
T MathR::Clamp01(T value)
{
    return Clamp<T>(value, 0, 1);
}

template <typename T>
T MathR::Abs(T value)
{
    return std::abs(value);
}

template <typename T>
T MathR::Ceil(T value)
{
    return std::ceil(value);
}

template <typename T>
T MathR::Floor(T value)
{
    return std::floor(value);
}


#endif
