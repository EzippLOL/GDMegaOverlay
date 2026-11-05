#include "Common.h"
#include "Settings.hpp"

#include <Geode/modify/HardStreak.hpp>

using namespace geode::prelude;

class $modify(HardStreak)
{
	void updateStroke(float dt)
	{
		if (Settings::get<bool>("player/trail/enabled", false))
		{
			float trailSize = Settings::get<float>("player/trail/size", 1.f);
			m_pulseSize = trailSize;

			if (Settings::get<bool>("player/trail/color/enabled", false))
			{
				float trailColor[3]{
					Settings::get<float>("player/trail/color/r", 1.f),
					Settings::get<float>("player/trail/color/g", 1.f),
					Settings::get<float>("player/trail/color/b", 1.f)
				};

				this->setColor({
					(byte)(trailColor[0] * 255),
					(byte)(trailColor[1] * 255),
					(byte)(trailColor[2] * 255)}
				);
			}
		}

		HardStreak::updateStroke(dt);
	}
};
