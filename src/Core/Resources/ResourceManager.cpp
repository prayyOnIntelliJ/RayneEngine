#include "ResourceManager.h"

std::shared_ptr<sf::Texture> ResourceManager::GetTexture(const std::string &path)
{
    auto it = m_Textures.find(path);
    if (it != m_Textures.end())
        return it->second;

    std::cout << "[INFO] [ResourceManager] Loading texture from disk: " << path << "...\n";
    auto texture = std::make_shared<sf::Texture>();
    if (!texture->loadFromFile(path))
    {
        std::cerr << "[ERROR] [ResourceManager] Failed to load texture: " << path << "\n";
        return nullptr;
    }

    texture->setSmooth(false);
    m_Textures[path] = texture;
    std::cout << "[INFO] [ResourceManager] Successfully loaded texture: " << path << " (" << texture->getSize().x << "x"
            << texture->getSize().y << ")\n";
    return texture;
}

std::shared_ptr<sf::Font> ResourceManager::GetFont(const std::string &path)
{
    auto it = m_Fonts.find(path);
    if (it != m_Fonts.end())
        return it->second;

    std::cout << "[INFO] [ResourceManager] Loading font from disk: " << path << "...\n";
    auto font = std::make_shared<sf::Font>();
    if (!font->loadFromFile(path))
    {
        std::cerr << "[ERROR] [ResourceManager] Failed to load font: " << path << "\n";
        return nullptr;
    }

    m_Fonts[path] = font;
    std::cout << "[INFO] [ResourceManager] Successfully loaded font: " << path << "\n";
    return font;
}

std::shared_ptr<sf::SoundBuffer> ResourceManager::GetSoundBuffer(const std::string &path)
{
    auto it = m_Sounds.find(path);
    if (it != m_Sounds.end())
        return it->second;

    std::cout << "[INFO] [ResourceManager] Loading sound buffer from disk: " << path << "...\n";
    auto buffer = std::make_shared<sf::SoundBuffer>();
    if (!buffer->loadFromFile(path))
    {
        std::cerr << "[ERROR] [ResourceManager] Failed to load sound buffer: " << path << "\n";
        return nullptr;
    }

    m_Sounds[path] = buffer;
    std::cout << "[INFO] [ResourceManager] Successfully loaded sound buffer: " << path << " (" << buffer->getDuration().
            asSeconds() << "s)\n";
    return buffer;
}

void ResourceManager::ClearTextures()
{
    std::cout << "[ResourceManager] Cleared " << m_Textures.size() << " texture(s).\n";
    m_Textures.clear();
}

void ResourceManager::ClearFonts()
{
    std::cout << "[ResourceManager] Cleared " << m_Fonts.size() << " font(s).\n";
    m_Fonts.clear();
}

void ResourceManager::ClearSounds()
{
    std::cout << "[ResourceManager] Cleared " << m_Sounds.size() << " sound buffer(s).\n";
    m_Sounds.clear();
}

void ResourceManager::ClearAll()
{
    ClearTextures();
    ClearFonts();
    ClearSounds();
}

void ResourceManager::PrintStats() const
{
    std::cout << "[ResourceManager] Cache stats: "
            << m_Textures.size() << " texture(s), "
            << m_Fonts.size() << " font(s), "
            << m_Sounds.size() << " sound buffer(s).\n";
}

void ResourceManager::RegisterLua(sol::state &lua)
{
    auto res = lua.create_named_table("Resource");

    res.set_function("PreloadTexture",
                     [](const std::string &path) -> bool { return Get().GetTexture(path) != nullptr; });

    res.set_function("PreloadFont", [](const std::string &path) -> bool { return Get().GetFont(path) != nullptr; });

    res.set_function("PreloadSound", [](const std::string &path) -> bool {
        return Get().GetSoundBuffer(path) != nullptr;
    });

    res.set_function("ClearTextures", []() { Get().ClearTextures(); });

    res.set_function("ClearFonts", []() { Get().ClearFonts(); });

    res.set_function("ClearSounds", []() { Get().ClearSounds(); });

    res.set_function("ClearAll", []() { Get().ClearAll(); });

    res.set_function("TextureCount", []() -> size_t { return Get().TextureCount(); });

    res.set_function("FontCount", []() -> size_t { return Get().FontCount(); });

    res.set_function("SoundCount", []() -> size_t { return Get().SoundBufferCount(); });

    res.set_function("PrintStats", []() { Get().PrintStats(); });
}
