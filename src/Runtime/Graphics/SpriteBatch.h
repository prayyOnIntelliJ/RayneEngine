#ifndef RAYNEENGINE_SPRITEBATCH_H
#define RAYNEENGINE_SPRITEBATCH_H

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <vector>

namespace Rayne
{

struct BatchCommand
{
    const sf::Texture* texture = nullptr;
    size_t vertexOffset = 0;
    size_t vertexCount = 0;
};

class SpriteBatch
{
public:
    SpriteBatch();
    ~SpriteBatch() = default;

    // Begin a new batch pass.
    void Begin();

    // Submit a quad/sprite to the batch.
    void Draw(const sf::Texture* texture,
              sf::Vector2f position,
              sf::Vector2f size,
              float rotation = 0.f,
              sf::Vector2f scale = {1.f, 1.f},
              sf::Vector2f origin = {0.f, 0.f},
              sf::Color color = sf::Color::White,
              sf::FloatRect textureRect = sf::FloatRect());

    // Flush and draw all batched quads to the given target window.
    void End(sf::RenderWindow& window);

    // Get number of draw calls produced in the last Begin/End cycle.
    size_t GetDrawCallCount() const { return m_LastDrawCallCount; }

private:
    std::vector<sf::Vertex> m_Vertices;
    std::vector<BatchCommand> m_Batches;
    size_t m_LastDrawCallCount = 0;
};

} // namespace Rayne

#endif // RAYNEENGINE_SPRITEBATCH_H
