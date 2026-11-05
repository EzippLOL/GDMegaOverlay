#include "SafeMode.h"

#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>

#include <Geode/Utils.hpp>
#include <Geode/platform/windows.hpp>

#include <Geode/Geode.hpp>

#include "../util.hpp"
#include "../Settings.hpp"

#include "Common.h"

using namespace geode::prelude;
using namespace SafeMode;

class $modify(EndLevelLayer)
{
	void customSetup()
	{
		EndLevelLayer::customSetup();

		if (
			!Settings::get<bool>("level/safe_mode/enabled") ||
			!Settings::get<bool>("level/safe_mode/endscreen_enabled")
			) return;

		auto layer = reinterpret_cast<CCLayer*>(this->getChildren()->objectAtIndex(0));
		CCLabelBMFont* endScreenMessageLabel = nullptr;
		TextArea* endScreenTextArea = nullptr;


		for (auto object : CCArrayExt<CCNode*>(layer->getChildren()))
		{
			if (auto textArea = dynamic_cast<TextArea*>(object); textArea)
				endScreenTextArea = textArea;
			else if (
				auto messageLabel = dynamic_cast<CCLabelBMFont*>(object);
				messageLabel
			) {
				std::string labelText = messageLabel->getString();

				if (
					!labelText.starts_with("Attempts: ") && !labelText.starts_with("Jumps: ") &&
					!labelText.starts_with("Time: ") && !labelText.starts_with("Points: ")
					)
					endScreenMessageLabel = messageLabel;
			}
		}

		// TODO: Create a CCLabelBMFont if both of these are false

		if (endScreenTextArea)
			endScreenTextArea->setString("- Safe Mode -");

		if (endScreenMessageLabel)
			endScreenMessageLabel->setString("- Safe Mode -");
	}
};

void SafeMode::updateAuto()
{
	if (Settings::get<bool>("level/safe_mode/auto", false))
		Mod::get()->setSavedValue<bool>("level/safe_mode/enabled", Common::isCheating);

	updateState();
}

void SafeMode::updateState()
{
	// nothing to patch anymore: the hooks below read the setting directly
}

static bool safeModeOn()
{
	return Settings::get<bool>("level/safe_mode/enabled", false);
}

// Safe mode used to patch the game so it would not save anything. It is now done with hooks:
// completing the level in test mode skips rewards/progress, and the percentage/jump saving is skipped.
class $modify(PlayLayer)
{
	void levelComplete()
	{
		if (!safeModeOn())
			return PlayLayer::levelComplete();

		bool wasTestMode = m_isTestMode;
		m_isTestMode = true;

		PlayLayer::levelComplete();

		m_isTestMode = wasTestMode;
	}

};

class $modify(GJGameLevel)
{
	void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid)
	{
		if (safeModeOn())
			return;

		GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);
	}
};
