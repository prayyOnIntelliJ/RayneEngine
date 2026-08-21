#ifndef RAYNEENGINE_RESOURCEMANAGER_H
#define RAYNEENGINE_RESOURCEMANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <sol/sol.hpp>

// -----------------------------------------------------------
// ResourceManager
// Singleton that caches all engine assets by file path.
// Identical paths always return the same shared instance,
// so textures, fonts, and sound buffers are never loaded twice.
// -----------------------------------------------------------
class ResourceManager
{
public:
    static ResourceManager &Get()
    {
        static ResourceManager instance;
        return instance;
    }

    // --- Asset accessors ---
    // Returns nullptr and prints an error if loading fails.
    std::shared_ptr<sf::Texture> GetTexture(const std::string &path);

    std::shared_ptr<sf::Font> GetFont(const std::string &path);

    std::shared_ptr<sf::SoundBuffer> GetSoundBuffer(const std::string &path);

    // --- Cache management ---
    void ClearTextures();

    void ClearFonts();

    void ClearSounds();

    void ClearAll();

    // --- Debug / stats ---
    size_t TextureCount() const { return m_Textures.size(); }
    size_t FontCount() const { return m_Fonts.size(); }
    size_t SoundBufferCount() const { return m_Sounds.size(); }

    void PrintStats() const;

    // --- Lua Bindings ---
    static void RegisterLua(sol::state &lua);

private:
    ResourceManager() = default;

    ~ResourceManager() = default;

    ResourceManager(const ResourceManager &) = delete;

    ResourceManager &operator=(const ResourceManager &) = delete;

    std::unordered_map<std::string, std::shared_ptr<sf::Texture> > m_Textures;
    std::unordered_map<std::string, std::shared_ptr<sf::Font> > m_Fonts;
    std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer> > m_Sounds;
};

#endif // RAYNEENGINE_RESOURCEMANAGER_H
