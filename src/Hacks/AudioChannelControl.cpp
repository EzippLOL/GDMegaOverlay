#include "AudioChannelControl.h"
#include "../Common.h"

#include <Geode/Geode.hpp>
#include "Geode/fmod/fmod_dsp_effects.h"
#include <Geode/fmod/fmod.hpp>

#include <Geode/modify/PlayLayer.hpp>

#include <Geode/binding/FMODAudioEngine.hpp>

using namespace geode::prelude;

FMOD_RESULT AudioChannelControl::setVolumeHook(FMOD::Channel* channel, float volume)
{
	FMOD::Sound* sound;
	channel->getCurrentSound(&sound);
	float freq;
	sound->getDefaults(&freq, nullptr);
	channel->setFrequency(freq * speed);

	return channel->setVolume(volume);
}

void AudioChannelControl::set(float frequency)
{
	speed = frequency;

	FMOD::Channel* audioChannel;

	for(int i = 0; i < 2; i++)
	{
		
		FMODAudioEngine::sharedEngine()->m_system->getChannel(126 + i, &audioChannel);
		if (audioChannel)
		{
			FMOD::Sound* sound;
			audioChannel->getCurrentSound(&sound);
			float freq;
			sound->getDefaults(&freq, nullptr);
			audioChannel->setFrequency(freq * frequency);
		}
	}
}

void AudioChannelControl::setPitch(float pitch)
{
	auto system = FMODAudioEngine::sharedEngine()->m_system;
	FMOD::ChannelGroup* group;
	system->getMasterChannelGroup(&group);

	static FMOD::DSP* pitchShifter = nullptr;

	if (pitchShifter)
	{
		group->removeDSP(pitchShifter);
		pitchShifter->release();
		pitchShifter = nullptr;
	}
	
	if (pitch == 1.f)
		return;
	
	system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &pitchShifter);
	pitchShifter->setParameterFloat(FMOD_DSP_PITCHSHIFT_FFTSIZE, 4096);
	pitchShifter->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, pitch);
	group->addDSP(0, pitchShifter);
}

class $modify(PlayLayer)
{
	void resetLevel()
	{
		Common::onAudioSpeedChange();
		PlayLayer::resetLevel();
	}
};

$execute
{
	AudioChannelControl::speed = 1.f;

	// This runs while the DLL is being loaded, so it must never throw. GD 2.2091 is 64-bit, and its
	// fmod.dll exports the 64-bit decoration of this function (the old 32-bit name "...QAG..." does not exist).
	HMODULE fmod = GetModuleHandleA("fmod.dll");

	void* setVolumeAddress = fmod ? reinterpret_cast<void*>(GetProcAddress(
		fmod, "?setVolume@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z")) : nullptr;

	if (!setVolumeAddress)
	{
		log::error("Audio speed hook disabled: FMOD::ChannelControl::setVolume was not found");
		return;
	}

	auto res = Mod::get()->hook(setVolumeAddress, &AudioChannelControl::setVolumeHook, "setVolumeHook", tulip::hook::TulipConvention::Thiscall);

	if (res.isErr())
		log::error("Audio speed hook disabled: {}", res.unwrapErr());
}
