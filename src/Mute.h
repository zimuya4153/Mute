#pragma once

#include <ll/api/plugin/NativePlugin.h>
#include <ll/api/i18n/I18n.h>

namespace Mute {

class mute {

public:
    static mute& getInstance();

    mute(ll::plugin::NativePlugin& self) : mSelf(self) {}

    [[nodiscard]] ll::plugin::NativePlugin& getSelf() const { return mSelf; }

    /// @return True if the plugin is loaded successfully.
    bool load();

    /// @return True if the plugin is enabled successfully.
    bool enable();

    /// @return True if the plugin is disabled successfully.
    bool disable();

    // TODO: Implement this method if you need to unload the plugin.
    // /// @return True if the plugin is unloaded successfully.
    bool unload();

private:
    ll::plugin::NativePlugin& mSelf;
};

} // namespace Mute
