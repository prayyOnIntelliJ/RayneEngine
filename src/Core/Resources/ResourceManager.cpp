#include "ResourceManager.h"
#include <fstream>
#include <vector>
#include <SFML/Graphics/Image.hpp>

// ---------------------------------------------------------------------------
// EXIF orientation helper – parses raw JPEG bytes, no external library needed
// Returns EXIF orientation value 1-8 (1 = normal, 0 = not found / not JPEG)
// ---------------------------------------------------------------------------
static int ReadJpegExifOrientation(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;

    // Check JPEG SOI marker
    uint8_t soi[2];
    f.read(reinterpret_cast<char*>(soi), 2);
    if (soi[0] != 0xFF || soi[1] != 0xD8) return 0;

    while (f)
    {
        uint8_t marker[2];
        f.read(reinterpret_cast<char*>(marker), 2);
        if (!f || marker[0] != 0xFF) break;

        uint8_t segLen[2];
        f.read(reinterpret_cast<char*>(segLen), 2);
        if (!f) break;

        int len = (segLen[0] << 8) | segLen[1];

        // APP1 marker = 0xFFE1 → may contain Exif
        if (marker[1] == 0xE1 && len > 6)
        {
            std::vector<uint8_t> seg(len - 2);
            f.read(reinterpret_cast<char*>(seg.data()), len - 2);
            if (!f) break;

            // Check "Exif\0\0" header
            if (seg.size() >= 6 && seg[0]=='E' && seg[1]=='x' && seg[2]=='i' && seg[3]=='f' && seg[4]==0 && seg[5]==0)
            {
                const uint8_t *tiff = seg.data() + 6;
                size_t tiffSize = seg.size() - 6;
                if (tiffSize < 8) break;

                bool littleEndian = (tiff[0] == 'I' && tiff[1] == 'I');
                auto read16 = [&](const uint8_t *p) -> uint16_t {
                    return littleEndian ? (uint16_t)(p[0] | (p[1]<<8)) : (uint16_t)((p[0]<<8) | p[1]);
                };
                auto read32 = [&](const uint8_t *p) -> uint32_t {
                    return littleEndian ? (uint32_t)(p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24))
                                       : (uint32_t)((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]);
                };

                uint32_t ifdOffset = read32(tiff + 4);
                if (ifdOffset + 2 > tiffSize) break;

                uint16_t numEntries = read16(tiff + ifdOffset);
                for (uint16_t i = 0; i < numEntries; ++i)
                {
                    size_t entryOff = ifdOffset + 2 + i * 12;
                    if (entryOff + 12 > tiffSize) break;
                    uint16_t tag = read16(tiff + entryOff);
                    if (tag == 0x0112) // Orientation
                    {
                        return read16(tiff + entryOff + 8);
                    }
                }
            }
            continue;
        }

        // Skip non-APP1 segments
        f.seekg(len - 2, std::ios::cur);
    }
    return 0;
}

// Apply EXIF orientation to an sf::Image in-place
static void ApplyExifOrientation(sf::Image &img, int orientation)
{
    if (orientation <= 1 || orientation > 8) return;

    unsigned int w = img.getSize().x;
    unsigned int h = img.getSize().y;

    // Build transformed pixel buffer
    bool transpose = (orientation >= 5);
    unsigned int dstW = transpose ? h : w;
    unsigned int dstH = transpose ? w : h;

    std::vector<sf::Uint8> dst(dstW * dstH * 4);
    const sf::Uint8 *src = img.getPixelsPtr();

    for (unsigned int y = 0; y < h; ++y)
    {
        for (unsigned int x = 0; x < w; ++x)
        {
            unsigned int dstX, dstY;
            switch (orientation)
            {
                case 2: dstX = w - 1 - x; dstY = y;         break; // mirror H
                case 3: dstX = w - 1 - x; dstY = h - 1 - y; break; // 180°
                case 4: dstX = x;          dstY = h - 1 - y; break; // mirror V
                case 5: dstX = y;          dstY = x;          break; // transpose
                case 6: dstX = h - 1 - y; dstY = x;          break; // 90° CW
                case 7: dstX = h - 1 - y; dstY = w - 1 - x;  break; // transverse
                case 8: dstX = y;          dstY = w - 1 - x;  break; // 90° CCW
                default: dstX = x; dstY = y;                  break;
            }
            size_t srcIdx = (y * w + x) * 4;
            size_t dstIdx = (dstY * dstW + dstX) * 4;
            dst[dstIdx + 0] = src[srcIdx + 0];
            dst[dstIdx + 1] = src[srcIdx + 1];
            dst[dstIdx + 2] = src[srcIdx + 2];
            dst[dstIdx + 3] = src[srcIdx + 3];
        }
    }

    img.create(dstW, dstH, dst.data());
}

// ---------------------------------------------------------------------------

std::shared_ptr<sf::Texture> ResourceManager::GetTexture(const std::string &path)
{
    auto it = m_Textures.find(path);
    if (it != m_Textures.end())
        return it->second;

    std::cout << "[INFO] [ResourceManager] Loading texture from disk: " << path << "...\n";

    // Check for JPEG EXIF orientation before loading into SFML
    // (SFML ignores EXIF data, causing rotated images for phone/camera photos)
    std::string lower = path;
    for (char &c : lower) c = (char)std::tolower((unsigned char)c);
    bool isJpeg = lower.size() >= 4 &&
                  (lower.substr(lower.size() - 4) == ".jpg" ||
                   lower.substr(lower.size() - 5) == ".jpeg");

    int exifOrientation = isJpeg ? ReadJpegExifOrientation(path) : 0;

    sf::Image img;
    if (!img.loadFromFile(path))
    {
        std::cerr << "[ERROR] [ResourceManager] Failed to load texture: " << path << "\n";
        return nullptr;
    }

    if (exifOrientation > 1)
    {
        std::cout << "[INFO] [ResourceManager] Applying EXIF orientation " << exifOrientation << " to: " << path << "\n";
        ApplyExifOrientation(img, exifOrientation);
    }

    auto texture = std::make_shared<sf::Texture>();
    if (!texture->loadFromImage(img))
    {
        std::cerr << "[ERROR] [ResourceManager] Failed to create texture from image: " << path << "\n";
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
