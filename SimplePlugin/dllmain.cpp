#include "../plugin_sdk/plugin_sdk.hpp"
#include "galio.h"

PLUGIN_NAME("OuijAIO");
PLUGIN_TYPE(plugin_type::champion);

SUPPORTED_CHAMPIONS(champion_id::Galio);

PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);

    switch (myhero->get_champion())
    {
    case champion_id::Galio:
        galio::load();
        return true;
    default:
        break;
    }

    return false;;
}

PLUGIN_API void on_sdk_unload()
{
    switch (myhero->get_champion())
    {
    case champion_id::Galio:
        galio::unload();
    default:
        break;
    }
}