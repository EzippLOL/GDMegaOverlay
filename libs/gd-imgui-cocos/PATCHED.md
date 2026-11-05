# gd-imgui-cocos (patched copy)

Upstream: https://github.com/maxnut/gd-imgui-cocos at commit 6c8e0d1 (MIT, see LICENSE).

Changes for Geode 6 / Geometry Dash 2.2091:
- removed `$on_mod(Unloaded)` (the event no longer exists)
- `CCKeyboardDispatcher::dispatchKeyboardMSG` hook takes the extra `double timestamp`
- `CCEGLView::toggleFullScreen` hook takes the extra `bool fix`
- the imgui.ini path no longer relies on `std::filesystem::path` coming from Geode
