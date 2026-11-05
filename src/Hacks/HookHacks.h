#pragma once

#include <string>

// Hook-based replacements for the old raw byte patches (res/hacks/*.json).
// The old patches pointed at fixed addresses in the 2.204 executable and cannot be
// used on newer versions, so every hack that is still offered is implemented as a hook.
namespace HookHacks
{
	enum Id
	{
		IconBypass,
		CharacterFilter,
		TextLength,
		LoadFailed,
		NoClip,
		NoDeathEffect,
		NoSpiderDash,
		NoRobotFire,
		NoMirrorPortal,
		NoCameraZoom,
		NoCameraMove,
		NoHideTrigger,
		ForcePlatformer,
		Count
	};

	inline bool flag[Count] = {};

	// true if the hack with this (JSON) name has a hook implementation
	bool isImplemented(const std::string& name);

	// turns the hook behind a hack on/off (called whenever its checkbox changes)
	void set(const std::string& name, bool value);
}
