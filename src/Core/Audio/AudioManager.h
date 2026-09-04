#ifndef RAYNEENGINE_AUDIOMANAGER_H
#define RAYNEENGINE_AUDIOMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <sol/sol.hpp>

class AudioManager
{
public:
    static AudioManager &Get()
    {
        static AudioManager instance;
        return instance;
    }

    void PlaySound(const std::string &path, float volume = 100.f, float pitch = 1.0f);

    void StopAllSounds();

    void PlayMusic(const std::string &path, bool loop = true, float volume = 100.f);

    void StopMusic();

    void PauseMusic();

    void ResumeMusic();

    void SetMusicVolume(float volume);

    void SetMasterVolume(float volume);

    void Update();

    static void RegisterLua(sol::state &lua);

private:
    AudioManager();

    ~AudioManager() = default;

    AudioManager(const AudioManager &) = delete;

    AudioManager &operator=(const AudioManager &) = delete;

    std::vector<std::unique_ptr<sf::Sound> > m_ActiveSounds;
    sf::Music m_Music;
    float m_MasterVolume = 100.f;
    float m_MusicVolume = 100.f;
    static constexpr size_t MaxSoundChannels = 32;
};

#endif
