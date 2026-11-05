#include "../Common.h"
#include "../Settings.hpp"

#include <Geode/modify/CCScheduler.hpp>
#include <Geode/binding/GameManager.hpp>

using namespace geode::prelude;

class $modify(CCScheduler)
{
	void update(float dt)
	{
		float speedhack =
		Settings::get<bool>("general/speedhack/enabled") ? Settings::get<float>("general/speedhack/value") : 1.f;

		if(speedhack <= 0)
			speedhack = 1.f;

		dt *= speedhack;

		CCScheduler::update(dt);
	}
};
