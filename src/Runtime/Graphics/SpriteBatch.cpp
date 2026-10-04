#include "SpriteBatch.h"
#include <cmath>

namespace Rayne
{

static constexpr float DEG2RAD = 3.14159265358979323846f / 180.f;

SpriteBatch::SpriteBatch()
{
    m_Vertices.reserve(4096);
    m_Batches.reserve(64);
}

void SpriteBatch::Begin()
{
    m_Vertices.clear();
    m_Batches.clear();
}

void SpriteBatch::Draw(const sf::Texture* texture,
                       sf::Vector2f position,
                       sf::Vector2f size,
                       float rotation,
                       sf::Vector2f scale,
                       sf::Vector2f origin,
                       sf::Color color,
                       sf::FloatRect textureRect)
{
    // If texture rect is empty and we have a valid texture, use the whole texture
    if (textureRect.width == 0.f && textureRect.height == 0.f && texture)
    {
        sf::Vector2u texSize = texture->getSize();
        textureRect = sf::FloatRect(0.f, 0.f, static_cast<float>(texSize.x), static_cast<float>(texSize.y));
    }

    // Determine texture coordinates
    float u0 = textureRect.left;
    float v0 = textureRect.top;
    float u1 = textureRect.left + textureRect.width;
    float v1 = textureRect.top + textureRect.height;

    // Local vertex coordinates relative to origin
    // Local quad: (0,0) to (size.x, size.y), scaled by scale
    float w = size.x * scale.x;
    float h = size.y * scale.y;
    float ox = origin.x * scale.x;
    float oy = origin.y * scale.y;

    float p0x = -ox;
    float p0y = -oy;

    float p1x = w - ox;
    float p1y = -oy;

    float p2x = w - ox;
    float p2y = h - oy;

    float p3x = -ox;
    float p3y = h - oy;

    // Rotate if necessary
    if (rotation != 0.f)
    {
        float rad = rotation * DEG2RAD;
        float cosR = std::cos(rad);
        float sinR = std::sin(rad);

        auto rotate = [&](float& x, float& y) {
            float rx = x * cosR - y * sinR;
            float ry = x * sinR + y * cosR;
            x = rx;
            y = ry;
        };

        rotate(p0x, p0y);
        rotate(p1x, p1y);
        rotate(p2x, p2y);
        rotate(p3x, p3y);
    }

    // Translate by position
    p0x += position.x; p0y += position.y;
    p1x += position.x; p1y += position.y;
    p2x += position.x; p2y += position.y;
    p3x += position.x; p3y += position.y;

    // Ensure batch command matches current texture
    if (m_Batches.empty() || m_Batches.back().texture != texture)
    {
        BatchCommand cmd;
        cmd.texture = texture;
        cmd.vertexOffset = m_Vertices.size();
        cmd.vertexCount = 0;
        m_Batches.push_back(cmd);
    }

    // Append 4 vertices (sf::Quads)
    m_Vertices.emplace_back(sf::Vector2f(p0x, p0y), color, sf::Vector2f(u0, v0));
    m_Vertices.emplace_back(sf::Vector2f(p1x, p1y), color, sf::Vector2f(u1, v0));
    m_Vertices.emplace_back(sf::Vector2f(p2x, p2y), color, sf::Vector2f(u1, v1));
    m_Vertices.emplace_back(sf::Vector2f(p3x, p3y), color, sf::Vector2f(u0, v1));

    m_Batches.back().vertexCount += 4;
}

void SpriteBatch::End(sf::RenderWindow& window)
{
    m_LastDrawCallCount = m_Batches.size();

    for (const auto& batch : m_Batches)
    {
        if (batch.vertexCount == 0) continue;

        sf::RenderStates states;
        states.texture = batch.texture;

        window.draw(&m_Vertices[batch.vertexOffset], batch.vertexCount, sf::Quads, states);
    }

    m_Batches.clear();
    m_Vertices.clear();
}

} // namespace Rayne
