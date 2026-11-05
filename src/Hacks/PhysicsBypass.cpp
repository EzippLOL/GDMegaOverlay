#include "PhysicsBypass.h"
#include "Settings.hpp"
#include "Macrobot/Macrobot.h"
#include <Geode/modify/PlayLayer.hpp>
#include "Common.h"

#include <cstring>

using namespace geode::prelude;

// In 2.2091 the physics step is a double constant (1/240) in the exe's read-only data. It is
// read by GJBaseGameLayer::getModifiedDelta (which splits the frame time into steps) and by the
// saved-position code, so changing it moves all of them together, like the old version did.
namespace
{
	constexpr uintptr_t STEP_CONSTANT_RVA = 0x835040;

	Patch* stepPatch = nullptr;
	bool stepInitialised = false;

	ByteVector toBytes(double value)
	{
		ByteVector bytes(sizeof(double));
		std::memcpy(bytes.data(), &value, sizeof(double));
		return bytes;
	}

	void setStep(double step)
	{
		void* address = reinterpret_cast<void*>(geode::base::get() + STEP_CONSTANT_RVA);

		if (!stepInitialised)
		{
			stepInitialised = true;

			// only touch the constant if it really is 1/240, so a different game build is never corrupted
			double current = 0;
			std::memcpy(&current, address, sizeof(double));

			if (current != 1.0 / 240.0)
			{
				log::error("TPS bypass disabled: unexpected value at the physics step constant");
				return;
			}

			auto res = Mod::get()->patch(address, toBytes(step));

			if (res.isErr())
			{
				log::error("TPS bypass disabled: could not patch the physics step constant");
				return;
			}

			stepPatch = res.unwrap();
			return;
		}

		if (stepPatch)
			(void)stepPatch->updateBytes(toBytes(step));
	}
}

class $modify(PlayLayer)
{
	bool init(GJGameLevel* level, bool dontCreateObjects)
	{
		PhysicsBypass::timestamp = level->m_timestamp;

		return PlayLayer::init(level, dontCreateObjects);
	}

	void resetLevel()
	{
		PlayLayer::resetLevel();
		PhysicsBypass::calculateTickrate();
	}
};

void PhysicsBypass::calculateTickrate()
{
	float tps = Common::getTPS();

	if (tps <= 0.f)
		return;

	setStep(1.0 / static_cast<double>(tps));

	if (PlayLayer::get() && PlayLayer::get()->m_level->m_timestamp > 0)
	{
		// keep the completion percentage right: the level length is counted in steps
		float timestampMultiplier = (tps / 240.f);
		float stepsMultiplier = (timestamp * timestampMultiplier) / PlayLayer::get()->m_level->m_timestamp;
		PlayLayer::get()->m_level->m_timestamp = timestamp * timestampMultiplier;
		PlayLayer::get()->m_gameState.m_currentProgress *= stepsMultiplier;
	}
}
