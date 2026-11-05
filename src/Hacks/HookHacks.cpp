#include "HookHacks.h"

#include <climits>
#include <unordered_map>

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/LevelTools.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace
{
	const std::unordered_map<std::string, HookHacks::Id>& table()
	{
		static const std::unordered_map<std::string, HookHacks::Id> t = {
			{"Icon Bypass", HookHacks::IconBypass},
			{"Character Filter", HookHacks::CharacterFilter},
			{"Text Length", HookHacks::TextLength},
			{"Load Failed", HookHacks::LoadFailed},
			{"NoClip", HookHacks::NoClip},
			{"No Death Effect", HookHacks::NoDeathEffect},
			{"No Spider Dash", HookHacks::NoSpiderDash},
			{"No Robot Fire", HookHacks::NoRobotFire},
			{"No Mirror Portal", HookHacks::NoMirrorPortal},
			{"No Camera Zoom", HookHacks::NoCameraZoom},
			{"No Camera Move", HookHacks::NoCameraMove},
			{"No Hide Trigger", HookHacks::NoHideTrigger},
			{"Force Platformer", HookHacks::ForcePlatformer},
		};
		return t;
	}
}

bool HookHacks::isImplemented(const std::string& name)
{
	return table().contains(name);
}

void HookHacks::set(const std::string& name, bool value)
{
	auto it = table().find(name);

	if (it != table().end())
		flag[it->second] = value;
}

// ---- bypass ----

class $modify(HHGameManager, GameManager)
{
	bool isIconUnlocked(int id, IconType type)
	{
		if (HookHacks::flag[HookHacks::IconBypass])
			return true;

		return GameManager::isIconUnlocked(id, type);
	}

	bool isColorUnlocked(int id, UnlockType type)
	{
		if (HookHacks::flag[HookHacks::IconBypass])
			return true;

		return GameManager::isColorUnlocked(id, type);
	}
};

class $modify(HHTextInputNode, CCTextInputNode)
{
	bool onTextFieldInsertText(CCTextFieldTTF* pSender, char const* text, int nLen, enumKeyCodes keyCodes)
	{
		bool noFilter = HookHacks::flag[HookHacks::CharacterFilter];
		bool noLimit = HookHacks::flag[HookHacks::TextLength];

		if (!noFilter && !noLimit)
			return CCTextInputNode::onTextFieldInsertText(pSender, text, nLen, keyCodes);

		std::string savedChars = m_allowedChars;

		if (noFilter)
		{
			std::string everything;

			for (int c = 1; c < 256; c++)
				everything.push_back(static_cast<char>(c));

			m_allowedChars = everything;
		}

		if (noLimit)
			m_maxLabelLength = 10000;

		bool ret = CCTextInputNode::onTextFieldInsertText(pSender, text, nLen, keyCodes);

		if (noFilter)
			m_allowedChars = savedChars;

		return ret;
	}
};

class $modify(HHLevelTools, LevelTools)
{
	static bool verifyLevelIntegrity(std::string str, int id)
	{
		if (HookHacks::flag[HookHacks::LoadFailed])
			return true;

		return LevelTools::verifyLevelIntegrity(str, id);
	}
};

// ---- player ----

class $modify(HHPlayLayer, PlayLayer)
{
	void destroyPlayer(PlayerObject* player, GameObject* object)
	{
		// the anti-cheat spike must still kill the player, like the old NoClip patch
		if (HookHacks::flag[HookHacks::NoClip] && object != m_anticheatSpike)
			return;

		PlayLayer::destroyPlayer(player, object);
	}
};

class $modify(HHPlayerObject, PlayerObject)
{
	void playDeathEffect()
	{
		if (HookHacks::flag[HookHacks::NoDeathEffect])
			return;

		PlayerObject::playDeathEffect();
	}

	void playSpiderDashEffect(CCPoint from, CCPoint to)
	{
		if (HookHacks::flag[HookHacks::NoSpiderDash])
			return;

		PlayerObject::playSpiderDashEffect(from, to);
	}

	void update(float dt)
	{
		PlayerObject::update(dt);

		if (HookHacks::flag[HookHacks::NoRobotFire] && m_robotFire)
			m_robotFire->setVisible(false);
	}
};

class $modify(HHBaseGameLayer, GJBaseGameLayer)
{
	void toggleFlipped(bool flip, bool noEffects)
	{
		if (HookHacks::flag[HookHacks::NoMirrorPortal])
			return;

		GJBaseGameLayer::toggleFlipped(flip, noEffects);
	}

	void updateZoom(float zoom, float duration, int easing, float rate, int uniqueID, int controlID)
	{
		if (HookHacks::flag[HookHacks::NoCameraZoom])
			return;

		GJBaseGameLayer::updateZoom(zoom, duration, easing, rate, uniqueID, controlID);
	}

	void updateCameraOffsetX(float offsetX, float duration, int easingType, float easingRate, int uniqueID, int controlID)
	{
		if (HookHacks::flag[HookHacks::NoCameraMove])
			return;

		GJBaseGameLayer::updateCameraOffsetX(offsetX, duration, easingType, easingRate, uniqueID, controlID);
	}

	void updateCameraOffsetY(float offsetY, float duration, int easingType, float easingRate, int uniqueID, int controlID)
	{
		if (HookHacks::flag[HookHacks::NoCameraMove])
			return;

		GJBaseGameLayer::updateCameraOffsetY(offsetY, duration, easingType, easingRate, uniqueID, controlID);
	}

	void loadLevelSettings()
	{
		// the original copies this flag into the game layer and both players, so set it first
		if (HookHacks::flag[HookHacks::ForcePlatformer] && m_levelSettings)
			m_levelSettings->m_platformerMode = true;

		GJBaseGameLayer::loadLevelSettings();
	}
};

class $modify(HHEffectGameObject, EffectGameObject)
{
	void triggerObject(GJBaseGameLayer* layer, int uniqueID, std::vector<int> const* remapKeys)
	{
		// 1612 / 1613 are the show / hide player triggers, blocking both keeps the player visible
		if (HookHacks::flag[HookHacks::NoHideTrigger] && (m_objectID == 1612 || m_objectID == 1613))
			return;

		EffectGameObject::triggerObject(layer, uniqueID, remapKeys);
	}
};
