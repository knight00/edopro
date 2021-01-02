#ifdef YGOPRO_USE_SDL_MIXER
#include "sound_sdlmixer.h"
#define SDL_MAIN_HANDLED
#include <stdexcept>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <fmt/printf.h>
#include <thread>
#include <atomic>

SoundMixer::SoundMixer() : music(nullptr), sound_volume(0), music_volume(0) {
	SDL_SetMainReady();
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error(fmt::format("Failed to init sdl audio device!\nSDL_InitSubSystem: {}\n", SDL_GetError()));
	int flags = MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_FLAC;
	int initted = Mix_Init(flags);
	fmt::print("MIX_INIT_OGG: {}\n", !!(initted & MIX_INIT_OGG));
	fmt::print("MIX_INIT_MP3: {}\n", !!(initted & MIX_INIT_MP3));
	fmt::print("MIX_INIT_FLAC: {}\n", !!(initted & MIX_INIT_FLAC));
	if((initted&flags) != flags) {
		Mix_Quit();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		throw std::runtime_error("Not all flags set");
	}
	if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) == -1) {
		Mix_Quit();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		throw std::runtime_error(fmt::format("Cannot open channels\nMix_OpenAudio: {}\n", SDL_GetError()));
	}
}
void SoundMixer::SetSoundVolume(double volume) {
	sound_volume = (int)(volume * 128);
	Mix_Volume(-1, sound_volume);
}
void SoundMixer::SetMusicVolume(double volume) {
	music_volume = (int)(volume * 128);
	Mix_VolumeMusic(music_volume);
}
bool SoundMixer::PlayMusic(const std::string& name, bool loop) {
	if(music && cur_music == name)
		return false;
	if(music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
	}
	music = Mix_LoadMUS(name.data());
	if(music) {
		if(Mix_PlayMusic(music, loop ? -1 : 0) == -1) {
			Mix_FreeMusic(music);
			return false;
		} else {
			cur_music = name;
		}
	} else {
		return false;
	}
	return true;
}
bool SoundMixer::PlaySound(const std::string& name) {
	auto chunk = Mix_LoadWAV(name.data());
	if(chunk) {
		auto channel = Mix_PlayChannel(-1, chunk, 0);
		if(channel == -1) {
			Mix_FreeChunk(chunk);
			return false;
		}
		sounds[channel] = chunk;
		Mix_Volume(-1, sound_volume);
	} else {
		return false;
	}
	return true;
}
void SoundMixer::StopSounds() {
	Mix_HaltChannel(-1);
}
void SoundMixer::StopMusic() {
	if(music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = nullptr;
	}
}
void SoundMixer::PauseMusic(bool pause) {
	if(pause)
		Mix_PauseMusic();
	else
		Mix_ResumeMusic();
}
bool SoundMixer::MusicPlaying() {
	return Mix_PlayingMusic();
}
void SoundMixer::Tick() {
	for(auto chunk = sounds.begin(); chunk != sounds.end();) {
		if(Mix_Playing(chunk->first) == 0) {
			Mix_FreeChunk(chunk->second);
			sounds.erase(chunk++);
		} else
			chunk++;
	}
}
void KillSwitch(std::atomic_bool& die) {
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	if(die)
		exit(0);
}
SoundMixer::~SoundMixer() {
	std::atomic_bool die{true};
	std::thread(KillSwitch, std::ref(die)).detach();
	Mix_HaltChannel(-1);
	Mix_HaltMusic();
	for(auto& chunk : sounds) {
		if(chunk.second)
			Mix_FreeChunk(chunk.second);
	}
	if(music)
		Mix_FreeMusic(music);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	die = false;
}

#endif //YGOPRO_USE_SDL_MIXER