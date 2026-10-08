#pragma once
#include <array>
#include <string_view>
namespace mw3gf::detail {
// Key/button metadata verified against Xbox SP resources. Only identifiers
// and icon IDs are stored; displayed text comes from the current locale.
// Action parameters take precedence over this fixed-context catalogue.
enum class FixedPromptKind { label, marked_input, icon_only, parameter };
struct FixedPrompt {
    std::string_view key, glyphs;
    FixedPromptKind kind;
};
inline constexpr std::array fixed_prompts{
    FixedPrompt{"NY_HARBOR_PLATFORM_HINT_DRIVE_SDV_3", "\x11", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_BACK", "\x02", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_BACK_CAPS", "\x02", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_CHANGE_FILTER_CAPS", "\x04", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_CLEAR_BUTTON", "\x04", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_CLOSE_CAPS", "\x01", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_DEMO_CLEAR_ALL_SEGMENTS", "\x04", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_DEMO_CONTROLS_NEXT_PLAYER_KEY", "\x06", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_CONTROLS_PREVIOUS_PLAYER_KEY", "\x05", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DELETE_SEGMENT", "\x04", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_DEMO_DVR_FAST_MOTION", "\x14", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DVR_JUMP_BACK", "\x12", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DVR_JUMP_FORWARD", "\x13", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DVR_SLOW_MOTION", "\x15", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DVR_SWITCH_PLAYER_NEXT", "\x06", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DVR_SWITCH_PLAYER_PREV", "\x05", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_DEMO_DVR_TOGGLE_CONTROLS_HUD", "\x10", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_DEMO_PLACE_SEGMENT", "\x01", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_DEMO_PREVIEW_CLIP", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_DEMO_PREVIEW_SEGMENT", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_EOG_PRESS_ESC", "\x02", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_FILTER", "\x04", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_FRIENDS_ONLINE_VAULT_BROWSE", "\x01", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_GAME_SUMMARY_CAPS", "\x04", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_INVITE", "\x01", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_INVITE_TO_GAME", "\x01", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_INVITE_TO_PARTY", "\x01", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_JOIN", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_LOCSEL_DIR_CONTROLS", "\x12 \x11", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_MW3_BACK", "\x02", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_NO", "\x02", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_PAGE_DOWN", "\x15", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_PAGE_DOWN_CAPS", "\x15", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_PAGE_UP", "\x14", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_PAGE_UP_CAPS", "\x14", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_PREDATOR_MISSILE_AIM", "\x11", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_PRESS_TO_SET_AIRSTRIKE", "\x13", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_RELOAD", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_SHOW_GROUPS", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_TOP", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_TOP_OF_LIST_CAPS", "\x03", FixedPromptKind::label},
    FixedPrompt{"PLATFORM_UI_CANCEL_RIGHT", "\x02", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_CLEAR_ATTACHMENTS", "\x04", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_UI_CLEAR_DEATHSTREAKS", "\x04", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_UI_CLEAR_KILLSTREAKS", "\x04", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_UI_CLEAR_PERKS", "\x04", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_UI_PRESS_BUTTONSELECTCHOICE", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_PRESS_TO_CONTINUE", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_SELECTBUTTON", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_SELECTBUTTON_DESELECT", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_SELECTBUTTON_DISABLE", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_SELECTBUTTON_ENABLE", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_SELECTBUTTON_SELECT", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UI_SELECTBUTTON_TOGGLE", "\x01", FixedPromptKind::parameter},
    FixedPrompt{"PLATFORM_UNLOCK_KILLSTREAK", "\x01", FixedPromptKind::icon_only},
    FixedPrompt{"PLATFORM_VEH_BOOST", "\x01", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_VEH_BRAKE", "\x10", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_VEH_FIRE", "\x13", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_VEH_THROTTLE", "\x10", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_VIEW_CHALLENGES", "\x03", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_VIEW_CHALLENGE_DETAILS", "\x03", FixedPromptKind::marked_input},
    FixedPrompt{"PLATFORM_YES", "\x01", FixedPromptKind::label},
    FixedPrompt{"SCRIPT_PLATFORM_STEER_DRONE", "\x11", FixedPromptKind::icon_only},
};
} // namespace mw3gf::detail
