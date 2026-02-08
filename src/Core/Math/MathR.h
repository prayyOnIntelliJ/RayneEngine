#ifndef RAYNEENGINE_MATHR_H
#define RAYNEENGINE_MATHR_H
#include <cmath>


class MathR
{
public:
    // Clamp
    template <typename T>
    static T Clamp(T value, T min, T max);
    template <typename T>
    static T Clamp01(T value);
    // Abs
    template <typename T>
    static T Abs(T value);
    // Ceil, Floor
    template <typename T>
    static T Ceil(T value);
    template <typename T>
    static T Floor(T value);
    // Lerp
    template <typename T>
    static T Lerp(T start, T end, T factor);
    template <typename T>
    static T InverseLerp(T start, T end, T value);
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

template <typename T>
T MathR::Lerp(T start, T end, T factor)
{
    factor = Clamp(factor, 0.0f, 1.0f);
    return start + factor * (end - start);
}

template <typename T>
T MathR::InverseLerp(T start, T end, T value)
{
    value = Clamp(value, start, end);
    return (value - start) / (end - start);
}


#endif
