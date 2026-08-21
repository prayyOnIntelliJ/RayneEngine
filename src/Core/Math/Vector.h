#ifndef VECTOR_H
#define VECTOR_H

#include <cmath>

struct Vector
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // 2D
    Vector(float x, float y)
        : x(x), y(y), z(0.0f) {}

    // 3D
    Vector(float x, float y, float z)
        : x(x), y(y), z(z) {}

    Vector() = default;

    Vector operator+(const Vector &o) const { return {x + o.x, y + o.y, z + o.z}; }

    Vector operator-(const Vector &o) const { return {x - o.x, y - o.y, z - o.z}; }

    Vector operator*(float s) const { return {x * s, y * s, z * s}; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }

    Vector Normalized() const
    {
        float len = Length();
        if (len == 0.0f) return *this;
        return {x / len, y / len, z / len};
    }

    static float Dot(const Vector &a, const Vector &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    static Vector Cross(const Vector &a, const Vector &b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

#endif
