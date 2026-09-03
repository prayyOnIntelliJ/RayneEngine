#ifndef RAYNEENGINE_MATHR_H
#define RAYNEENGINE_MATHR_H

#include "sol/state.hpp"

class MathR
{
public:
    static constexpr float PI = 3.1415926535f;

    static float ClampI(int value, int min, int max);

    static float ClampF(float value, float min, float max);

    static float ClampF01(float value);

    static float AbsI(int value);

    static float AbsF(int value);

    static float Ceil(float value);

    static float Floor(float value);

    static float Lerp(float start, float end, float factor);

    static float InverseLerp(float start, float end, float value);

    static float Sin(float x);

    static float Cos(float x);

    static void RegisterLua(sol::state &lua);

private:
    template<typename T>
    static T Clamp(T value, T min, T max);

    template<typename T>
    static T Abs(T value);
};

template<typename T>
T MathR::Clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;

    return value;
}

template<typename T>
T MathR::Abs(T value)
{
    if (value <= 0) return -value;

    return value;
}

#endif
