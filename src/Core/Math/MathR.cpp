#include "MathR.h"

float MathR::ClampI(const int value, const int min, const int max) { return Clamp(value, min, max); }

float MathR::ClampF(const float value, const float min, const float max) { return Clamp(value, min, max); }

float MathR::ClampF01(const float value) { return Clamp(value, 0.f, 1.f); }

float MathR::AbsI(const int value) { return Abs(value); }

float MathR::AbsF(const int value) { return Abs(value); }

float MathR::Floor(const float value)
{
    const int i = static_cast<int>(value);
    if (value < static_cast<float>(i)) return i - 1;

    return i;
}

float MathR::Lerp(const float start, const float end, float factor)
{
    factor = Clamp(factor, 0.0f, 1.0f);
    return start + factor * (end - start);
}

float MathR::InverseLerp(const float start, const float end, float value)
{
    value = Clamp(value, start, end);
    return (value - start) / (end - start);
}

float MathR::Sin(float x)
{
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;

    float result = 0.f;
    float term = x;
    float x_sq = x * x;

    for (int i = 1; i <= 10; i++)
    {
        result += term;
        const int n = 2 * i;
        term *= -x_sq / (static_cast<float>(n) * (n + 1));
    }
    return result;
}

float MathR::Cos(float x)
{
    return Sin(x + (PI / 2.f));
}

float MathR::Ceil(const float value) { return -Floor(-value); }

void MathR::RegisterLua(sol::state &lua)
{
    auto math = lua.create_named_table("MathR");
    math.set_function("ClampI", &ClampI);
    math.set_function("ClampF", &ClampF);
    math.set_function("ClampF01", &ClampF01);
    math.set_function("AbsI", &AbsI);
    math.set_function("AbsF", &AbsF);
    math.set_function("Ceil", &Ceil);
    math.set_function("Floor", &Floor);
    math.set_function("Lerp", &Lerp);
    math.set_function("InverseLerp", &InverseLerp);
    math.set_function("Sin", &Sin);
    math.set_function("Cos", &Cos);
}
