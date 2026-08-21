#include "AudioManager.h"
#include "../Resources/ResourceManager.h"
#include <algorithm>
#include <iostream>

AudioManager::AudioManager()
{
}

void AudioManager::Update()
{
    std::erase_if(m_ActiveSounds, [](const std::unique_ptr<sf::Sound>& sound) {
        return sound->getStatus() == sf::SoundSource::Status::Stopped;
    });
}

void AudioManager::PlaySound(const std::string& path, float volume, float pitch)
{
    auto buffer = ResourceManager::Get().GetSoundBuffer(path);
    if (!buffer)
    {
        std::cerr << "[AudioManager] Cannot play sound: " << path << "\n";
        return;
    }

    Update();

    if (m_ActiveSounds.size() >= MaxSoundChannels)
    {
        m_ActiveSounds.erase(m_ActiveSounds.begin());
    }

    auto sound = std::make_unique<sf::Sound>();
    sound->setBuffer(*buffer);
    sound->setVolume(volume * (m_MasterVolume / 100.f));
    sound->setPitch(pitch);
    sound->play();

    m_ActiveSounds.push_back(std::move(sound));
}

void AudioManager::StopAllSounds()
{
    for (auto& sound : m_ActiveSounds)
    {
        sound->stop();
    }
    m_ActiveSounds.clear();
}

void AudioManager::PlayMusic(const std::string& path, bool loop, float volume)
{
    if (!m_Music.openFromFile(path))
    {
        std::cerr << "[AudioManager] Cannot load music file: " << path << "\n";
        return;
    }

    m_MusicVolume = volume;
    m_Music.setLoop(loop);
    m_Music.setVolume(m_MusicVolume * (m_MasterVolume / 100.f));
    m_Music.play();
    std::cout << "[AudioManager] Playing music: " << path << "\n";
}

void AudioManager::StopMusic()
{
    m_Music.stop();
}

void AudioManager::PauseMusic()
{
    m_Music.pause();
}

void AudioManager::ResumeMusic()
{
    m_Music.play();
}

void AudioManager::SetMusicVolume(float volume)
{
    m_MusicVolume = volume;
    m_Music.setVolume(m_MusicVolume * (m_MasterVolume / 100.f));
}

void AudioManager::SetMasterVolume(float volume)
{
    m_MasterVolume = volume;
    m_Music.setVolume(m_MusicVolume * (m_MasterVolume / 100.f));
    for (auto& sound : m_ActiveSounds)
    {
        sound->setVolume(sound->getVolume() * (m_MasterVolume / 100.f));
    }
}

void AudioManager::RegisterLua(sol::state& lua)
{
    auto audio = lua.create_named_table("Audio");

    audio.set_function("PlaySound", sol::overload(
        [](const std::string& path) {
            Get().PlaySound(path, 100.f, 1.0f);
        },
        [](const std::string& path, float volume) {
            Get().PlaySound(path, volume, 1.0f);
        },
        [](const std::string& path, float volume, float pitch) {
            Get().PlaySound(path, volume, pitch);
        }
    ));

    audio.set_function("StopAllSounds", []() {
        Get().StopAllSounds();
    });

    audio.set_function("PlayMusic", sol::overload(
        [](const std::string& path) {
            Get().PlayMusic(path, true, 100.f);
        },
        [](const std::string& path, bool loop) {
            Get().PlayMusic(path, loop, 100.f);
        },
        [](const std::string& path, bool loop, float volume) {
            Get().PlayMusic(path, loop, volume);
        }
    ));

    audio.set_function("StopMusic", []() {
        Get().StopMusic();
    });

    audio.set_function("PauseMusic", []() {
        Get().PauseMusic();
    });

    audio.set_function("ResumeMusic", []() {
        Get().ResumeMusic();
    });

    audio.set_function("SetMusicVolume", [](float volume) {
        Get().SetMusicVolume(volume);
    });

    audio.set_function("SetMasterVolume", [](float volume) {
        Get().SetMasterVolume(volume);
    });
}
