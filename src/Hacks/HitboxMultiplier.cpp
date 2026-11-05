#include "HitboxMultiplier.h"

#include "Settings.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameObject.hpp>

#include "../Common.h"

class $modify(PlayLayer)
{
	bool init(GJGameLevel* level, bool dontCreateObjects)
	{
		bool res = PlayLayer::init(level, dontCreateObjects);
		HitboxMultiplier::memberRectForObject.clear();
		return res;
	}

	void resetLevel()
	{
		for(auto&pair : HitboxMultiplier::memberRectForObject)
			pair.first->m_objectRect = pair.second;

		HitboxMultiplier::memberRectForObject.clear();
		
		PlayLayer::resetLevel();
	}
};

void scaleRect(cocos2d::CCRect* r, float a, float b)
{
	r->origin.x += r->size.width / 2.f;
	r->origin.y += r->size.height / 2.f;
	r->size.width *= a;
	r->size.height *= b;
	r->origin.x -= r->size.width / 2.f;
	r->origin.y -= r->size.height / 2.f;
}

void HitboxMultiplier::scaleHazard(GameObject* hazard)
{
	if (hazard->m_objectType != GameObjectType::Hazard && hazard->m_objectType != GameObjectType::AnimatedHazard) 
		return;
	
	float scaleHazards[2] = {Settings::get<float>("level/hitbox_multiplier/scale_hazards/x", 1.f), Settings::get<float>("level/hitbox_multiplier/scale_hazards/y", 1.f)};

	cocos2d::CCRect rect = hazard->m_objectRect;

	if (rect.origin.x == 0 && rect.origin.y == 0 || rect.size.width == 0 && rect.size.height == 0)
		return;

	if (!memberRectForObject.contains(hazard))
		memberRectForObject[hazard] = rect;
	else
		rect = memberRectForObject[hazard];

	scaleRect(&rect, scaleHazards[0], scaleHazards[1]);

	hazard->m_objectRect = rect;
}

class $modify(GameObject)
{
	struct Fields
	{
		cocos2d::CCRect m_scaledRect;
	};

	CCRect const& getObjectRect()
	{
		bool enabled = Settings::get<bool>("level/hitbox_multiplier", false);
		
		if (!enabled)
			return GameObject::getObjectRect();

		bool all = Settings::get<bool>("level/hitbox_multiplier/scale_all", false);

		float x = 1, y = 1;
		
		float scaleSlopes[2] = {Settings::get<float>("level/hitbox_multiplier/scale_slopes/x", 1.f), Settings::get<float>("level/hitbox_multiplier/scale_slopes/y", 1.f)};
		float scalePlayer[2] = {Settings::get<float>("level/hitbox_multiplier/scale_player/x", 1.f), Settings::get<float>("level/hitbox_multiplier/scale_player/y", 1.f)};

		if ((PlayerObject*)this == GameManager::get()->getGameLayer()->m_player1 || (PlayerObject*)this == GameManager::get()->getGameLayer()->m_player2)
		{
			x = scalePlayer[0];
			y = scalePlayer[1];
		}
		else
		{
			switch(this->m_objectType)
			{
				default:
					return GameObject::getObjectRect();
				case GameObjectType::Slope:
					x = scaleSlopes[0];
					y = scaleSlopes[1];
					break;
			}
		}

		cocos2d::CCRect rect = GameObject::getObjectRect();
		scaleRect(&rect, x, y);
		m_fields->m_scaledRect = rect;
		return m_fields->m_scaledRect;
	}
};

$execute
{
	Common::sectionLoopFunctions.push_back({HitboxMultiplier::scaleHazard, "level/hitbox_multiplier"});
}