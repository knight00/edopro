#define WIN32_LEAN_AND_MEAN
#include "sound_manager.h"
#include "utils.h"
#include "config.h"
#if defined(YGOPRO_USE_IRRKLANG)
#include "sound_irrklang.h"
#define BACKEND SoundIrrklang
#elif defined(YGOPRO_USE_SDL_MIXER)
#include "sound_sdlmixer.h"
#define BACKEND SoundMixer
#elif defined(YGOPRO_USE_SFML)
#include "sound_sfml.h"
#define BACKEND SoundSFML
#endif
/////kdiy/////////
#include "game_config.h"
/////kdiy/////////

namespace ygo {
SoundManager::SoundManager(double sounds_volume, double music_volume, bool sounds_enabled, bool music_enabled, epro::path_stringview working_directory) {
#ifdef BACKEND
	fmt::print("Using: " STR(BACKEND)" for audio playback.\n");
	working_dir = Utils::ToUTF8IfNeeded(working_directory);
	soundsEnabled = sounds_enabled;
	musicEnabled = music_enabled;
	try {
		mixer = std::unique_ptr<SoundBackend>(new BACKEND());
		mixer->SetMusicVolume(music_volume);
		mixer->SetSoundVolume(sounds_volume);
	}
	catch(const std::runtime_error& e) {
		fmt::print("Failed to initialize audio backend:\n");
		fmt::print(e.what());
		succesfully_initied = soundsEnabled = musicEnabled = false;
		return;
	}
	catch(...) {
		fmt::print("Failed to initialize audio backend.\n");
		succesfully_initied = soundsEnabled = musicEnabled = false;
		return;
	}
	rnd.seed(time(0)&0xffffffff);
	////////kdiy////
	std::string bgm_now = "";
	////////kdiy////
	bgm_scene = -1;
	RefreshBGMList();
	RefreshSoundsList();
	RefreshChantsList();
	succesfully_initied = true;
#else
	fmt::print("No audio backend available.\nAudio will be disabled.\n");
	succesfully_initied = soundsEnabled = musicEnabled = false;
	return;
#endif // BACKEND
}
bool SoundManager::IsUsable() {
	return succesfully_initied;
}
void SoundManager::RefreshBGMList() {
#ifdef BACKEND
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/duel"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/menu"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/deck"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/advantage"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/disadvantage"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/win"));
	Utils::MakeDirectory(EPRO_TEXT("./sound/BGM/lose"));
	for (auto& list : BGMList)
		list.clear();
	RefreshBGMDir(EPRO_TEXT(""), BGM::DUEL);
	RefreshBGMDir(EPRO_TEXT("duel"), BGM::DUEL);
	RefreshBGMDir(EPRO_TEXT("menu"), BGM::MENU);
	RefreshBGMDir(EPRO_TEXT("deck"), BGM::DECK);
	RefreshBGMDir(EPRO_TEXT("advantage"), BGM::ADVANTAGE);
	RefreshBGMDir(EPRO_TEXT("disadvantage"), BGM::DISADVANTAGE);
	RefreshBGMDir(EPRO_TEXT("win"), BGM::WIN);
	RefreshBGMDir(EPRO_TEXT("lose"), BGM::LOSE);
#endif
}
void SoundManager::RefreshSoundsList() {
	static constexpr std::pair<SFX, epro::path_stringview> fx[] = {
		{SUMMON, EPRO_TEXT("./sound/summon.{}")},
		{SPECIAL_SUMMON, EPRO_TEXT("./sound/specialsummon.{}")},
		{ACTIVATE, EPRO_TEXT("./sound/activate.{}")},
		{SET, EPRO_TEXT("./sound/set.{}")},
		{FLIP, EPRO_TEXT("./sound/flip.{}")},
		{REVEAL, EPRO_TEXT("./sound/reveal.{}")},
		{EQUIP, EPRO_TEXT("./sound/equip.{}")},
		{DESTROYED, EPRO_TEXT("./sound/destroyed.{}")},
		{BANISHED, EPRO_TEXT("./sound/banished.{}")},
		{TOKEN, EPRO_TEXT("./sound/token.{}")},
		{ATTACK, EPRO_TEXT("./sound/attack.{}")},
		{DIRECT_ATTACK, EPRO_TEXT("./sound/directattack.{}")},
		{DRAW, EPRO_TEXT("./sound/draw.{}")},
		{SHUFFLE, EPRO_TEXT("./sound/shuffle.{}")},
		{DAMAGE, EPRO_TEXT("./sound/damage.{}")},
		{RECOVER, EPRO_TEXT("./sound/gainlp.{}")},
		{COUNTER_ADD, EPRO_TEXT("./sound/addcounter.{}")},
		{COUNTER_REMOVE, EPRO_TEXT("./sound/removecounter.{}")},
		{COIN, EPRO_TEXT("./sound/coinflip.{}")},
		{DICE, EPRO_TEXT("./sound/diceroll.{}")},
		{NEXT_TURN, EPRO_TEXT("./sound/nextturn.{}")},
		{PHASE, EPRO_TEXT("./sound/phase.{}")},
		{PLAYER_ENTER, EPRO_TEXT("./sound/playerenter.{}")},
		{CHAT, EPRO_TEXT("./sound/chatmessage.{}")}
	};
	const auto extensions = mixer->GetSupportedSoundExtensions();
	for(const auto& sound : fx) {
		for(const auto& ext : extensions) {
			const auto filename = fmt::format(sound.second, ext);
			if(Utils::FileExists(filename)) {
				SFXList[sound.first] = Utils::ToUTF8IfNeeded(filename);
				break;
			}
		}
	}
}
void SoundManager::RefreshBGMDir(epro::path_stringview path, BGM scene) {
#ifdef BACKEND
	for(auto& file : Utils::FindFiles(fmt::format(EPRO_TEXT("./sound/BGM/{}"), path), mixer->GetSupportedMusicExtensions())) {
		auto conv = Utils::ToUTF8IfNeeded(fmt::format(EPRO_TEXT("{}/{}"), path, file));
		BGMList[BGM::ALL].push_back(conv);
		BGMList[scene].push_back(std::move(conv));
	}
#endif
}
void SoundManager::RefreshChantsList() {
#ifdef BACKEND
	static constexpr std::pair<CHANT, epro::path_stringview> types[] = {
		/////kdiy///////
		{CHANT::SET,       EPRO_TEXT("set")},
		{CHANT::EQUIP,     EPRO_TEXT("equip")},
		{CHANT::DESTROY,   EPRO_TEXT("destroyed")},
		{CHANT::BANISH,    EPRO_TEXT("banished")},						
		{CHANT::DRAW,      EPRO_TEXT("draw")},	
		{CHANT::DAMAGE,    EPRO_TEXT("damage")},	
		{CHANT::RECOVER,   EPRO_TEXT("gainlp")},	
		{CHANT::NEXTTURN,  EPRO_TEXT("nextturn")},
		/////kdiy///////				
		{CHANT::SUMMON,    EPRO_TEXT("summon")},
		{CHANT::ATTACK,    EPRO_TEXT("attack")},
		{CHANT::ACTIVATE,  EPRO_TEXT("activate")}
	};
	ChantsList.clear();
	/////kdiy//////
	int i=-1;
	for (auto list : ChantSPList) {
		list.clear();
	}
	/////kdiy///////
	for (const auto& chantType : types) {
		const epro::path_string searchPath = fmt::format(EPRO_TEXT("./sound/{}"), chantType.second);
		Utils::MakeDirectory(searchPath);
		/////kdiy///////
		if(chantType.first != CHANT::SUMMON && chantType.first != CHANT::ATTACK && chantType.first != CHANT::ACTIVATE) {
			if(chantType.first == CHANT::SET) i=0;		
			if(chantType.first == CHANT::EQUIP) i=1;
			if(chantType.first == CHANT::DESTROY) i=2;
			if(chantType.first == CHANT::BANISH) i=3;
			if(chantType.first == CHANT::DRAW) i=4;
			if(chantType.first == CHANT::DAMAGE) i=5;
			if(chantType.first == CHANT::RECOVER) i=6;
			if(chantType.first == CHANT::NEXTTURN) i=7;
			if(i == -1) continue;;			
			for (auto& file : Utils::FindFiles(searchPath, mixer->GetSupportedSoundExtensions())) {
				auto conv = Utils::ToUTF8IfNeeded(chantType.second) + Utils::ToUTF8IfNeeded(EPRO_TEXT("/") + file);
				ChantSPList[i].push_back(conv);
			}			
		} else {
		/////kdiy///////	
		for (auto& file : Utils::FindFiles(searchPath, mixer->GetSupportedSoundExtensions())) {
			const auto filepath = fmt::format(EPRO_TEXT("{}/{}"), searchPath, file);
			auto scode = Utils::GetFileName(file);
			try {
				uint32_t code = static_cast<uint32_t>(std::stoul(scode));
				auto key = std::make_pair(chantType.first, code);
				if (code && !ChantsList.count(key))
					ChantsList[key] = fmt::format("{}/{}", working_dir, Utils::ToUTF8IfNeeded(fmt::format(EPRO_TEXT("{}/{}"), searchPath, file)));
			}
			catch (...) {
				continue;
			}
		}
		/////kdiy///////
		}
		/////kdiy///////
	}
#endif
}
void SoundManager::PlaySoundEffect(SFX sound) {
#ifdef BACKEND
	if(!soundsEnabled) return;
	if(sound >= SFX::SFX_TOTAL_SIZE) return;
	if(SFXList[sound].empty()) return;
	mixer->PlaySound(SFXList[sound]);
#endif
}
void SoundManager::PlayBGM(BGM scene, bool loop) {
#ifdef BACKEND
	auto& list = BGMList[scene];
	int count = list.size();
	if(musicEnabled && (scene != bgm_scene || !mixer->MusicPlaying()) && count > 0) {
		bgm_scene = scene;
		int bgm = (std::uniform_int_distribution<>(0, count - 1))(rnd);
		const std::string BGMName = fmt::format("{}/./sound/BGM/{}", working_dir, list[bgm]);
		/////kdiy/////
		std::string bgm_custom = "BGM/custom/";
		std::string bgm_menu = "BGM/menu/";
		std::string bgm_deck = "BGM/deck/";
		std::string bgm_win = "BGM/win/";
		std::string bgm_lose = "BGM/lose/";
		if(BGMName.find(bgm_menu) != std::string::npos && BGMName.find(bgm_deck) != std::string::npos && BGMName.find(bgm_win) != std::string::npos && BGMName.find(bgm_lose) != std::string::npos && bgm_now.find(bgm_custom) != std::string::npos) return;
		bgm_now = BGMName;
		/////kdiy/////
		mixer->PlayMusic(BGMName, loop);
	}
#endif
}
///////kdiy//////
void SoundManager::PlayCustomMusic(std::wstring num) {
#ifdef BACKEND
	if(soundsEnabled) {
		const std::string BGMName = fmt::format("{}/./sound/custom/{}.mp3", working_dir, Utils::ToUTF8IfNeeded(num));
		if(Utils::FileExists(Utils::ToPathString(BGMName)))
		    mixer->PlaySound(BGMName);
	}
#endif
}
void SoundManager::PlayCustomBGM(std::wstring num) {
#ifdef BACKEND
	if (musicEnabled) {
		const std::string BGMName = fmt::format("{}/./sound/BGM/custom/{}.mp3", working_dir, Utils::ToUTF8IfNeeded(num));
		if (Utils::FileExists(Utils::ToPathString(BGMName))) {
			if(mixer->MusicPlaying())
			    mixer->StopMusic();
			bgm_now = BGMName;
			mixer->PlayMusic(BGMName, gGameConfig->loopMusic);
		}
	}
#endif
}
//bool SoundManager::PlayChant(CHANT chant, uint32_t code) {
bool SoundManager::PlayChant(CHANT chant, uint32_t code, uint32_t code2) {
///////kdiy//////
#ifdef BACKEND
	if(!soundsEnabled) return false;
	///////kdiy//////
	if(code == 0) {
		int i=-1;
		if(chant == CHANT::SET) i=0;		
		if(chant == CHANT::EQUIP) i=1;
		if(chant == CHANT::DESTROY) i=2;
		if(chant == CHANT::BANISH) i=3;
		if(chant == CHANT::DRAW) i=4;
		if(chant == CHANT::DAMAGE) i=5;
		if(chant == CHANT::RECOVER) i=6;
		if(chant == CHANT::NEXTTURN) i=7;
		if(i == -1) return false;
		auto& list = ChantSPList[i];
		int count = list.size();
		if(count > 0) {
			int bgm = (std::uniform_int_distribution<>(0, count - 1))(rnd);
			std::string BGMName = working_dir + "/./sound/" + list[bgm];
			mixer->PlaySound(BGMName);
			return true;
		}
	} else {
	auto key = std::make_pair(chant, code);
	auto key2 = std::make_pair(chant, code2);
	///////kdiy//////
	if (ChantsList.count(key)) {
		mixer->PlaySound(ChantsList[key]);
		return true;
	} else if (ChantsList.count(key2)) {
		mixer->PlaySound(ChantsList[key2]);
		return true;
	}
	///////kdiy//////
	}
	///////kdiy//////
#endif
	return false;
}
void SoundManager::SetSoundVolume(double volume) {
#ifdef BACKEND
	if(!mixer)
		return;
	mixer->SetSoundVolume(volume);
#endif
}
void SoundManager::SetMusicVolume(double volume) {
#ifdef BACKEND
	if(!mixer)
		return;
	mixer->SetMusicVolume(volume);
#endif
}
void SoundManager::EnableSounds(bool enable) {
#ifdef BACKEND
	if(!mixer)
		return;
	soundsEnabled = enable;
	if(!musicEnabled) {
		mixer->StopSounds();
	}
#endif
}
void SoundManager::EnableMusic(bool enable) {
#ifdef BACKEND
	if(!mixer)
		return;
	musicEnabled = enable;
	if(!musicEnabled) {
		mixer->StopMusic();
	}
#endif
}
void SoundManager::StopSounds() {
#ifdef BACKEND
	if(mixer)
		mixer->StopSounds();
#endif
}
void SoundManager::StopMusic() {
#ifdef BACKEND
	if(mixer)
		mixer->StopMusic();
#endif
}
void SoundManager::PauseMusic(bool pause) {
#ifdef BACKEND
	if(mixer)
		mixer->PauseMusic(pause);
#endif
}

void SoundManager::Tick() {
#ifdef BACKEND
	if(mixer)
		mixer->Tick();
#endif
}

} // namespace ygo
