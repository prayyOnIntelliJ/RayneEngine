#ifndef RAYNEENGINE_MATHR_H
#define RAYNEENGINE_MATHR_H
#include <cmath>

#include "sol/state.hpp"


class MathR
{
public:
    // Clamp
    static float ClampI(int value, int min, int max);
    static float ClampF(float value, float min, float max);
    static float ClampF01(float value);
    // Abs
    static float AbsI(int value);
    static float AbsF(int value);
    // Ceil, Floor
    static float Ceil(float value);
    static float Floor(float value);
    // Lerp
    static float Lerp(float start, float end, float factor);
    static float InverseLerp(float start, float end, float value);

    // Lua
    static void RegisterLua(sol::state& lua);

private:
    template <typename T>
    static T Clamp(T value, T min, T max);
    template <typename T>
    static T Abs(T value);
};

template <typename T>
T MathR::Clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;

    return value;
}

template <typename T>
T MathR::Abs(T value)
{
    if (value <= 0) return -value;

    return value;
}


#endif
