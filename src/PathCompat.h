#pragma once

#include <filesystem>

#include <Geode/loader/Mod.hpp>

// Geode 6 returns asp::fs::Path from Mod::getSaveDir()/getResourcesDir() instead of
// std::filesystem::path. The rest of this mod uses std::filesystem, so convert here.
namespace compat
{
	inline std::filesystem::path toStd(const asp::fs::Path& path)
	{
		return std::filesystem::path(path.native());
	}

	inline std::filesystem::path saveDir()
	{
		return toStd(geode::Mod::get()->getSaveDir());
	}

	inline std::filesystem::path resourcesDir()
	{
		return toStd(geode::Mod::get()->getResourcesDir());
	}
}
