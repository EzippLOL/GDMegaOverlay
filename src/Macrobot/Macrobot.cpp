#include "../Common.h"
#include "PathCompat.h"
#include "../ConstData.h"
#include "../GUI/GUI.h"
#include "../Hacks/ShowTrajectory.h"

#include "../util.hpp"
#include "../Settings.hpp"

#include <fstream>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <imgui-cocos.hpp>
#include <iostream>

#include "Macrobot.h"
#include "../Hacks/PhysicsBypass.h"
#include "MacroEditor.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CheckpointObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GameManager.hpp>

using namespace geode::prelude;
using namespace Macrobot;

class $modify(CCKeyboardDispatcher)
{
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool arr, double timestamp)
	{
		if (key == enumKeyCodes::KEY_D || key == enumKeyCodes::KEY_ArrowRight)
			direction = down ? 1 : 0;

		if (key == enumKeyCodes::KEY_A || key == enumKeyCodes::KEY_ArrowLeft)
			direction = down ? -1 : 0;
		
		int advanceKey = Settings::get<int>("macrobot/frame_step/key", ImGuiKey_G);

		int convertedKey = ConvertKeyEnum(key);

		if(convertedKey == advanceKey)
			holdingAdvance = down;

		if (!down || arr)
			return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, arr, timestamp);

		if (convertedKey == advanceKey && PlayLayer::get())
			targetSteps = PlayLayer::get()->m_gameState.m_currentProgress + Settings::get<int>("macrobot/frame_step/steps", 1) * 2;

		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, arr, timestamp);
	}
};

class $modify(PlayLayer)
{
	void onQuit()
	{
		PlayLayer::onQuit();
		checkpoints.clear();
	}

	void onExit()
	{
		botInput = false;
		PlayLayer::onExit();
	}

	void loadFromCheckpoint(CheckpointObject * checkpoint)
	{
		resetFromStart = false;
		if (checkpoints.contains(checkpoint) && playerMode != DISABLED && PlayLayer::get())
		{
			CheckpointData checkpointData = checkpoints[checkpoint];

			if(playerMode == RECORDING)
			{
				const auto check = [&](const Action &action) -> bool { return action.frame > checkpointData.frame; };
				macro.inputs.erase(std::remove_if (macro.inputs.begin(), macro.inputs.end(), check), macro.inputs.end());
			}
			
			PlayLayer::loadFromCheckpoint(checkpoint);

			checkpointData.p1.apply(this->m_player1, true);
			if (this->m_gameState.m_isDualMode)
				checkpointData.p2.apply(this->m_player2, true);
		}
		else
			PlayLayer::loadFromCheckpoint(checkpoint);
	}

	void resetLevel()
	{
		holdingAdvance = false;
		targetSteps = 0;
		
		if (playerMode != DISABLED)
		{
			actionIndex = 0;

			resetFromStart = true;
			resetFrame = true;

			downForKey1.clear();
			downForKey2.clear();
			timeForKey1.clear();
			timeForKey2.clear();

			PlayLayer::resetLevel();

			GJGameLevel *level = PlayLayer::get()->m_level;
			macro.levelInfo.id = level->m_levelID.value();
			macro.levelInfo.name = level->m_levelName;

			if (resetFromStart)
			{
				if(playerMode == RECORDING)
				{
					checkpoints.clear();
					macro.inputs.clear();

					// frames are counted in the game's own progress units: 2 per physics step
					macro.framerate = Common::getTPS() * 2.f;
				}
			}

			resetFrame = false;

			if (this->m_player1->m_isPlatformer)
			{
				if ((ImGui::IsKeyDown(ImGuiKey_RightArrow) || ImGui::IsKeyDown(ImGuiKey_D)) &&
					!(ImGui::IsKeyDown(ImGuiKey_LeftArrow) || ImGui::IsKeyDown(ImGuiKey_A)))
					this->m_player1->pushButton(PlayerButton::Right);
				else if (!(ImGui::IsKeyDown(ImGuiKey_RightArrow) || ImGui::IsKeyDown(ImGuiKey_D)) &&
						(ImGui::IsKeyDown(ImGuiKey_LeftArrow) || ImGui::IsKeyDown(ImGuiKey_A)))
					this->m_player1->pushButton(PlayerButton::Left);
			}

			if (playerMode == RECORDING)
			{
				//TODO only do this when necessary
				bool isDual = this->m_gameState.m_isDualMode;

				Macrobot::recordAction(PlayerButton::Jump, this->m_gameState.m_currentProgress + 1, false, true);
				if (isDual)
					Macrobot::recordAction(PlayerButton::Jump, this->m_gameState.m_currentProgress + 1, false, false);
			}
		}
		else
			PlayLayer::resetLevel();
	}
};

class $modify(PlayerObject)
{
	bool pushButton(PlayerButton btn)
	{
		bool ret = PlayerObject::pushButton(btn);

		if(ShowTrajectory::isSimulation)
			return ret;
		
		auto pl = PlayLayer::get();

		if (pl && playerMode == RECORDING && !resetFrame)
		{
			if (!pl->m_levelSettings->m_twoPlayerMode && this == pl->m_player2)
				return ret;

			recordAction(btn, pl->m_gameState.m_currentProgress, true, this == pl->m_player1);
		}

		return ret;
	}

	bool releaseButton(PlayerButton btn)
	{
		bool ret = PlayerObject::releaseButton(btn);

		if(ShowTrajectory::isSimulation)
			return ret;

		auto pl = PlayLayer::get();

		if (pl && playerMode == RECORDING && !resetFrame)
		{
			if (!pl->m_levelSettings->m_twoPlayerMode && this == pl->m_player2)
				return ret;

			if (btn == PlayerButton::Right && direction == 1)
				return ret;

			if (btn == PlayerButton::Left && direction == -1)
				return ret;

			recordAction(btn, pl->m_gameState.m_currentProgress, false, this == pl->m_player1);
		}

		return ret;
	}
};

Macrobot::Action* Macrobot::recordAction(PlayerButton key, uint32_t frame, bool press, bool player1)
{
	Action ac(frame, (int)key, !player1, press);

	PlayLayer* pl = PlayLayer::get();
	PlayerObject* player = player1 ? pl->m_player1 : pl->m_player2;

	if ((CorrectionType)Settings::get<int>("macrobot/corrections") == ACTION && (player->m_position.x != 0 && player->m_position.y != 0))
	{
		ac.correction = Correction(frame, !player1);
		ac.correction->checkpoint.fromPlayer(player, false);
	}

	macro.inputs.push_back(ac);

	return &macro.inputs[macro.inputs.size() - 1];
}

class $modify(GJBaseGameLayer)
{
	void update(float dt)
	{
		if (!Settings::get<bool>("macrobot/frame_step/enabled", false) || !PlayLayer::get())
		{
			GJBaseGameLayer::update(dt);
			return;
		}

		float speedhack = Settings::get<bool>("general/speedhack/enabled") ? Settings::get<float>("general/speedhack/value") : 1.f;

		if (Settings::get<bool>("macrobot/frame_step/hold", false) && holdingAdvance)
			advanceHoldTime += dt / speedhack;
		else
			advanceHoldTime = 0.0f;

		if (PlayLayer::get()->m_gameState.m_currentProgress < targetSteps)
			GJBaseGameLayer::update(dt);

		if (advanceHoldTime > 0.5)
			targetSteps = PlayLayer::get()->m_gameState.m_currentProgress + 2;
	}


	void handleButton(bool down, int button, bool player1)
	{
		if ((playerMode == PLAYBACK && !botInput) || resetFrame)
			return;
		
		GJBaseGameLayer::handleButton(down, button, player1);
	}

	// processCommands raises m_currentProgress and then applies the queued clicks, so clicks
	// recorded at a progress value are injected right after it returns with that same value
	void processCommands(float dt, bool isHalfTick, bool isLastTick)
	{
		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

		if (playerMode == PLAYBACK && PlayLayer::get())
		{
			uint32_t gameFrame = m_gameState.m_currentProgress;

			while (actionIndex < macro.inputs.size() && (gameFrame >= macro.inputs[actionIndex].frame))
			{
				Macrobot::handleAction(macro.inputs[actionIndex]);
				actionIndex++;
			}
		}
	}
};

class $modify(CheckpointObject)
{
	bool init()
	{
		bool res = CheckpointObject::init();

		if (playerMode != DISABLED && PlayLayer::get() && PlayLayer::get()->m_gameState.m_currentProgress > 0)
		{
			CheckpointData data;
			data.frame = PlayLayer::get()->m_gameState.m_currentProgress;
			data.p1.fromPlayer(PlayLayer::get()->m_player1, true);
			data.p2.fromPlayer(PlayLayer::get()->m_player2, true);

			checkpoints[this] = data;
		}

		return res;
	}
};

void Macrobot::handleAction(Action& action)
{
	auto pl = PlayLayer::get();

	int button = action.button;
	bool down = action.down;
	bool player1 = !action.player2;

	botInput = true;
	pl->handleButton(down, button, player1);
	botInput = false;

	if ((CorrectionType)Settings::get<int>("macrobot/corrections") == ACTION && action.correction.has_value())
		action.correction->checkpoint.apply(action.correction->player2 ? pl->m_player2 : pl->m_player1, false);
}

void Macrobot::MPlayerCheckpoint::fromPlayer(PlayerObject *player, bool fullCapture)
{
	if (!player)
		return;

	this->yVel = player->m_yVelocity;
	this->rotation = player->getRotation();
	this->xVel = player->m_platformerXVelocity;
	this->xPos = player->m_position.x;
	this->yPos = player->m_position.y;
	this->rotationRate = player->m_rotationSpeed;
	this->lastSnappedTo = player->m_objectSnappedTo;
	this->isOnSlope = player->m_isOnSlope;
	this->wasOnSlope = player->m_wasOnSlope;
	this->isOnGround = player->m_isOnGround;
}

void Macrobot::MPlayerCheckpoint::apply(PlayerObject* player, bool fullRestore)
{
	if (!player || PlayLayer::get()->m_gameState.m_currentProgress <= 0)
		return;

	if (fullRestore)
		player->m_objectSnappedTo = this->lastSnappedTo;

	player->m_yVelocity = this->yVel;
	player->setRotation(this->rotation);

	player->setPositionX(this->xPos);
	player->setPositionY(this->yPos);

	player->m_position = cocos2d::CCPoint(this->xPos, this->yPos);

	player->m_platformerXVelocity = this->xVel;

	player->m_rotationSpeed = this->rotationRate;

	player->m_isOnSlope = this->isOnSlope;
	player->m_wasOnSlope = this->wasOnSlope;
	player->m_isOnGround = this->isOnGround;
}

void Macrobot::save(const std::string& file)
{
	if (macro.inputs.size() <= 0)
	{
		Common::showWithPriority(FLAlertLayer::create("Error", "No inputs in macro!", "Ok"));
		return;
	}

	if (file == "")
	{
		Common::showWithPriority(FLAlertLayer::create("Error", "Macro name is empty!", "Ok"));
		return;
	}

	std::ofstream f(compat::saveDir() / "macros" / (file + ".gdr"), std::ios::binary);

	if (!f)
	{
		Common::showWithPriority(FLAlertLayer::create("Error", "Could not save macro!\n" + (compat::saveDir() / "macros" / (file + ".gdr")).string() + "!", "Ok"));
		f.close();
		return;
	}

	macro.author = GJAccountManager::sharedState()->m_username;
	macro.description = macroDescription;
	macro.duration = macro.inputs[macro.inputs.size() - 1].frame / macro.framerate;
	macro.gameVersion = 2.204f;
	macro.version = 1.0f;

	auto data = macro.exportData(false);

	f.write(reinterpret_cast<const char *>(data.data()), data.size());
	f.close();

	Common::showWithPriority(FLAlertLayer::create("Info", fmt::format("{} saved with {} inputs.", file, macro.inputs.size()), "Ok"));
}


std::optional<Macro> Macrobot::loadMacro(const std::string& file, bool inputs)
{
	std::ifstream f(compat::saveDir() / "macros" / (file + ".gdr"), std::ios::binary);

	if (!f)
	{
		f.close();
		return std::nullopt;
	}

	f.seekg(0, std::ios::end);
	size_t fileSize = f.tellg();
	f.seekg(0, std::ios::beg);

	std::vector<std::uint8_t> macroData(fileSize);

	f.read(reinterpret_cast<char *>(macroData.data()), fileSize);
	f.close();

	return Macro::importData(macroData, inputs);
}


bool Macrobot::load(const std::string& file)
{
	auto opt = loadMacro(file);

	if (!opt)
	{
		Common::showWithPriority(FLAlertLayer::create("Error", "Could not load macro!\n" + (compat::saveDir() / "macros" / (file + ".gdr")).string() + "!", "Ok"));
		return false;
	}

	macro = opt.value();

	if (macro.botInfo.name == "Macrobot" && macro.botInfo.version != "1.1")
	{
		macro = Macro();
		Common::showWithPriority(FLAlertLayer::create("Error", "This macro was recorded on an older macrobot version!", "Ok"));
		return false;
	}

	Common::showWithPriority(FLAlertLayer::create("Info", fmt::format("{} loaded with {} inputs.", file, macro.inputs.size()), "Ok"));
	PhysicsBypass::calculateTickrate();
	
	return true;
}

void Macrobot::remove(const std::string& file)
{
	std::filesystem::remove(compat::saveDir() / "macros" / (file + ".gdr"));
}

void Macrobot::drawMacroTable()
{
	static int macroIndex = 0;
	static std::string searchText = "";

	bool needReload = false;

	GUI::inputText("Search", &searchText);

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {10, 10});
	if (!ImGui::BeginTable("MacroTable", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_Resizable |
									ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody))
	{
		ImGui::PopStyleVar();
		return;
	}

	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("Author");
	ImGui::TableSetupColumn("FPS");
	ImGui::TableSetupColumn("Duration");
	ImGui::TableSetupColumn("Level");
	ImGui::TableSetupColumn("Actions");
	ImGui::TableHeadersRow();

	for(auto& pair : macroList)
	{
		Macro& macro = pair.second;

		if (!searchText.empty() && pair.first.find(searchText) == std::string::npos)
			continue;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text(pair.first.c_str());
		ImGui::TableNextColumn();
		ImGui::Text(macro.author.c_str());
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", macro.framerate);
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", macro.duration);
		ImGui::TableNextColumn();
		ImGui::Text("%s (%i)", macro.levelInfo.name.c_str(), macro.levelInfo.id);
		ImGui::TableNextColumn();
		if (GUI::button(("Load##macro" + pair.first).c_str()))
		{
			load(pair.first);
			ImGui::CloseCurrentPopup();
			GUI::toggle();
		}
		ImGui::SameLine();
		if (GUI::button(("Remove##macro" + pair.first).c_str()))
		{
			remove(pair.first);
			needReload = true;
		}
		ImGui::SameLine();
		if (GUI::button(("Play##macro" + pair.first).c_str()))
		{

			GameLevelManager::get()->downloadLevel(macro.levelInfo.id, true, 0);

			auto level = GameLevelManager::get()->getSavedLevel(macro.levelInfo.id);

			if (level && load(pair.first))
			{
				auto layer = PlayLayer::create(level, false);
				auto scene = CCScene::create();
				scene->addChild(layer);

				cocos2d::CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));

				Macrobot::playerMode = PLAYBACK;

				Common::calculateFramerate();
				PhysicsBypass::calculateTickrate();

				layer->stopAllActions();
				layer->startGame();
				layer->resetLevelFromStart();

				ImGui::CloseCurrentPopup();
				GUI::toggle();
			}
		}
	}
	
	ImGui::EndTable();
	ImGui::PopStyleVar();

	if (needReload)
		getMacros();
}

bool Macrobot::clickBetweenFramesCheck()
{
	bool res = Loader::get()->isModLoaded("syzzi.click_between_frames");

	if(res)
	{
		Common::showWithPriority(FLAlertLayer::create("Warning", "Disable the click between frames mod before using macrobot!", "Ok" ));
		Macrobot::playerMode = DISABLED;
		GUI::toggle();
	}

	return res;
}

void Macrobot::drawWindow()
{
	if (GUI::shouldRender())
	{
		if (ImGui::RadioButton("Disable", (int*)&Macrobot::playerMode, (int)DISABLED))
		{
			Common::calculateFramerate();
			PhysicsBypass::calculateTickrate();
		}
		if (ImGui::RadioButton("Record", (int*)&Macrobot::playerMode, (int)RECORDING) && !clickBetweenFramesCheck())
		{
			Common::calculateFramerate();
			PhysicsBypass::calculateTickrate();
			if (PlayLayer::get())
				PlayLayer::get()->resetLevel();
		}
		if (ImGui::RadioButton("Play", (int*)&Macrobot::playerMode, (int)PLAYBACK) && !clickBetweenFramesCheck())
		{
			Common::calculateFramerate();
			PhysicsBypass::calculateTickrate();
		}

		GUI::inputText("Name##macro", &macroName);
		GUI::inputText("Description##macro", &macroDescription);

		if (GUI::button("Save##macro"))
			save(macroName);
		GUI::sameLine();
		if (GUI::button("Load##macropopup"))
		{
			getMacros();
			ImGui::OpenPopup("Load Macro");
		}

		GUI::modalPopup("Load Macro", []{
			if (macroList.size() > 0)
				drawMacroTable();
			else
				ImGui::Text("There are no macros to load! Create some macros first.");
		});

		if (GUI::button("Editor"))
		{
			if(macro.inputs.size() <= 0)
			{
				Common::showWithPriority(FLAlertLayer::create("Error", "No macro loaded!", "Ok"));
				GUI::toggle();
			}
			else
				ImGui::OpenPopup("Macro Editor");
		}

		GUI::modalPopup("Macro Editor", []{
				MacroEditor::renderWindow();
		}, 0);

		int corrections = Settings::get<int>("macrobot/corrections");

		if (GUI::combo("Corrections", &corrections, correctionType, 2))
			Mod::get()->setSavedValue<int>("macrobot/corrections", corrections);

		GUI::tooltip("The type of correction the macro will use.\nNone: the bot will not correct on playback.\nEvery action: the bot will correct player's position, velocity and rotation every action.");

	}

	GUI::checkbox("Frame Step", "macrobot/frame_step/enabled");
	GUI::tooltip("Makes the game only advance physics steps when the frame step button is pressed.");

	GUI::arrowButton("Frame Step Settings");
	GUI::modalPopup(
		"Frame Step Settings",
		[] {
			int key = Settings::get<int>("macrobot/frame_step/key", ImGuiKey_G);
			if (GUI::hotkey("Advance", &key))
				Mod::get()->setSavedValue<int>("macrobot/frame_step/key", key);

			GUI::inputInt("Advance Steps", "macrobot/frame_step/steps", 1);

			GUI::checkbox("Hold to Advance", "macrobot/frame_step/hold");
		},
		ImGuiWindowFlags_AlwaysAutoResize);

	GUI::marker("[INFO]", "Corrections are recommended to be safe, but the bot also works decently without.");
}

void Macrobot::getMacros()
{
	macroList.clear();

	std::filesystem::path macroPath = compat::saveDir() / "macros";

	for (const auto& entry : std::filesystem::directory_iterator(macroPath))
	{
		if (entry.path().extension() == ".gdr")
		{
			std::string name = string::wideToUtf8(entry.path().stem().wstring());
			auto macroOpt = loadMacro(name, false);
			if (macroOpt)
				macroList.insert({name, macroOpt.value()});
		}
	}
}