#include "../Common.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "Settings.hpp"

using namespace geode::prelude;

// Called for every object in the visible sections (see Common.cpp).
// Older versions also re-attached objects to the right batch node by z layer; the 2.2 layer
// system manages that itself, so this only hides decoration and keeps the gameplay objects solid.
void hideObject(GameObject* obj)
{
	if ((obj->getParent() && !obj->isVisible()) || obj->m_objectID == 44 || obj->m_objectID == 749 || obj->m_objectID == 12 || obj->m_objectID == 38 ||
					 obj->m_objectID == 47 || obj->m_objectID == 111 || obj->m_objectID == 8 || obj->m_objectID == 13 ||
					 obj->m_objectID == 660 || obj->m_objectID == 745 || obj->m_objectID == 101 || obj->m_objectID == 99 ||
					 obj->m_objectID == 1331)
		return;

	switch(obj->m_objectType)
	{
		case GameObjectType::Decoration:
			obj->setVisible(false);
			break;
		case GameObjectType::Solid:
		case GameObjectType::Hazard:
		case GameObjectType::AnimatedHazard:
		case GameObjectType::Slope:
			obj->setOpacity(255);
			obj->setVisible(true);
			break;
		default:
			break;
	}
}

class $modify(PlayLayer)
{
	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		if (!Settings::get<bool>("level/layout_mode", false))
			return;

		ccColor3B layoutColor = {40, 62, 255};

		if (m_background)
			m_background->setColor(layoutColor);
	}
};

class $modify(ShaderLayer)
{
	void visit()
	{
		bool enabled = Settings::get<bool>("level/layout_mode", false);
		if (enabled)
		{
			CCNode::visit();
			return;
		}

		ShaderLayer::visit();
	}
};

class $modify(EffectGameObject)
{
	void triggerObject(GJBaseGameLayer* p0, int p1, std::vector<int> const* p2)
	{
		if (!Settings::get<bool>("level/layout_mode", false))
			return EffectGameObject::triggerObject(p0, p1, p2);

		int id = this->m_objectID;

		if (id == 899 || id == 1006 || id == 1007 || id == 105 || id == 29 || id == 56 || id == 915 || id == 30 || id == 58)
			return;

		EffectGameObject::triggerObject(p0, p1, p2);
	}
};

class $modify(GJBaseGameLayer)
{
	void lightningFlash(CCPoint from, CCPoint to, ccColor3B color, float lineWidth, float duration, int displacement, bool flash, float opacity)
	{
		if (Settings::get<bool>("level/layout_mode", false))
			return;

		GJBaseGameLayer::lightningFlash(from, to, color, lineWidth, duration, displacement, flash, opacity);
	}
};

$execute
{
	Common::sectionLoopFunctions.push_back({hideObject, "level/layout_mode"});
}
