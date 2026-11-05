#include "JsonPatches.h"
#include "PathCompat.h"
#include "../GUI/GUI.h"
#include "../util.hpp"
#include "../Hacks/HookHacks.h"
#include "../Settings.hpp"
#include <fstream>
#include <imgui.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;

void JsonPatches::init()
{
	auto append_patches = [](JsonPatches::patch_group_type& patchGroup, nlohmann::json& mods)
	{
		for (const auto& jsonPatch : mods)
		{
			const std::string name = jsonPatch["name"];

			// the raw byte patches of older versions are gone: only hacks that have a hook are offered
			if (!HookHacks::isImplemented(name))
				continue;

			JPatch jpatch{};
			jpatch.description = jsonPatch["description"];

			patchGroup[name] = jpatch;

			togglePatch(patchGroup, name);
		}
	};

	auto read_or_default = [](const std::string& modsName, nlohmann::json& mods)
	{

		std::ifstream file(compat::resourcesDir() / modsName);

		if (file.peek() == std::ifstream::traits_type::eof())
		{
			file.close();
			std::ofstream file_write(compat::resourcesDir() / modsName);
			file_write << mods;
			file_write.close();
		}
		else
			file >> mods;
	};

	nlohmann::json patchesJson = nlohmann::json::object();

	read_or_default("bypass.json", patchesJson);
	append_patches(JsonPatches::bypass, patchesJson["mods"]);
	patchesJson = nlohmann::json::object();

	read_or_default("creator.json", patchesJson);
	append_patches(JsonPatches::creator, patchesJson["mods"]);
	patchesJson = nlohmann::json::object();

	read_or_default("global.json", patchesJson);
	append_patches(JsonPatches::global, patchesJson["mods"]);
	patchesJson = nlohmann::json::object();

	read_or_default("level.json", patchesJson);
	append_patches(JsonPatches::level, patchesJson["mods"]);
	patchesJson = nlohmann::json::object();

	read_or_default("player.json", patchesJson);
	append_patches(JsonPatches::player, patchesJson["mods"]);
	patchesJson = nlohmann::json::object();

	// read_or_default("variables.json", patchesJson);
	// append_patches(JsonPatches::variables, patchesJson["mods"]);
}

void JsonPatches::togglePatch(JsonPatches::patch_group_type& patchGroup, const std::string& name)
{
	if (!patchGroup.contains(name))
		return;

	HookHacks::set(name, Settings::get<bool>("patch/" + name, false));
}

void JsonPatches::togglePatch(JsonPatches::patch_group_type& patchGroup, const std::string& name, bool value)
{
	Mod::get()->setSavedValue<bool>("patch/" + name, value);

	togglePatch(patchGroup, name);
}

void JsonPatches::drawFromPatches(JsonPatches::patch_group_type& patchGroup)
{
	for (const auto& [name, jpatch] : patchGroup)
	{
		if (GUI::checkbox(name, "patch/" + name, false))
			togglePatch(patchGroup, name);

		GUI::tooltip(jpatch.description);
	}
}
