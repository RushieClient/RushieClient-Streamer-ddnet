#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif
//Auto change
MACRO_CONFIG_STR(PlayerClanNoDummy, player_clan_no_dummy, 12, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Thats is clan when dummy not connected")
MACRO_CONFIG_STR(PlayerClanWithDummy, player_clan_with_dummy, 12, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Thats is clan when dummy connected")
MACRO_CONFIG_INT(PlayerClanAutoChange, player_clan_auto_change, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change clan when dummy connected")

//Chat
MACRO_CONFIG_INT(RiShowBlockedWordInConsole, ri_show_blocked_word_in_console, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show blocked word with regex in console")
MACRO_CONFIG_COL(RiBlockedWordConsoleColor, ri_blocked_word_console_color, 0x99ffff, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color of blocked word messages in console")
MACRO_CONFIG_INT(RiEnableCensorList, ri_enable_censor_list, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable word block list")
MACRO_CONFIG_INT(RiMultipleReplacementChar, ri_multiple_replacement_char, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Multiple replacement char on blocked word len")
MACRO_CONFIG_STR(RiBlockedContentReplacementChar, ri_blocked_content_replacement_char, 64, "*", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Character used to replace blocked content")
MACRO_CONFIG_STR(RiRegexPlayerWhitelist, ri_regex_player_whitelist, 512, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Chat filer whitelist")
MACRO_CONFIG_INT(RiFilterChangeWholeWord, ri_filter_change_whole_word, 1, 0, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change whole word to replacement character")
MACRO_CONFIG_STR(RiBlockedContentPartialReplacementChar, ri_blocked_content_partial_replacement_char, 64, "*", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Character used to replace partial blocked content")

//Scoreboard
MACRO_CONFIG_INT(RiResetPopupScoreboardOnUntab, ri_reset_popup_scoreboard_on_untab, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Reset popup scoreboard on untab")
MACRO_CONFIG_INT(RiToggleScoreboardMouse, ri_toggle_scoreboard_mouse, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle mouse scoreboard or work on hold")

//Lasers
MACRO_CONFIG_INT(RiLaserGlowIntensity, ri_laser_ench, 0, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(RiBetterLasers, ri_better_laser, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "")

//Tracker
MACRO_CONFIG_INT(RiShowLastPosHud, ri_show_last_pos_hud, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show last pos in hud target")
MACRO_CONFIG_INT(RiChangeTargetColorWhenXTargetEqualXPlayer, ri_change_target_color_when_x_equal, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change color pos x hud target to green when x target = x player")
MACRO_CONFIG_INT(RiChangePlayerColorWhenXTargetEqualXPlayer, ri_change_player_color_when_x_equal, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change color pos x hud player to green when x target = x player")

//Others pages
MACRO_CONFIG_INT(RiRClientSettingsTabs, ri_rclient_settings_tabs, 0, 0, 65536, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Bit flags to disable settings tabs")

//Controls
MACRO_CONFIG_INT(RiPrevMouseMaxDistance45degrees, ri_prev_mouse_max_distance_45_degrees, 400, 0, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_INSENSITIVE, "Previous maximum cursor distance for 45 degrees")
MACRO_CONFIG_INT(RiPrevInpMousesens45degrees, ri_prev_inp_mousesens_45_degrees, 200, 1, 100000, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Previous mouse sensitivity for 45 degrees")
MACRO_CONFIG_INT(RiToggle45degrees, ri_toggle_45_degrees, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle 45 degrees bind or not")
MACRO_CONFIG_INT(RiPrevInpMousesensSmallsens, ri_prev_inp_mousesens_small_sens, 200, 1, 100000, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Previous mouse sensitivity for small sens")
MACRO_CONFIG_INT(RiToggleSmallSens, ri_toggle_small_sens, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle small sens bind or not")

//Hud
MACRO_CONFIG_INT(RiShowMilliSecondsTimer, ri_show_milliseconds_timer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show milliseconds in game timer")
MACRO_CONFIG_INT(RiHeartSize, ri_heart_size, 75, 0, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Edit size of friend heart")
MACRO_CONFIG_INT(RiShowAfkEmoteInMenu, ri_show_afk_emote_menu, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Shows afk emote when player in menu (only client)")

//Dummy
MACRO_CONFIG_INT(RiShowhudDummyPosition, ri_showhud_dummy_position, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show ingame HUD (Dummy Position)")
MACRO_CONFIG_INT(RiShowLastPosHudDummy, ri_show_last_pos_hud_dummy, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show last pos in hud dummy")
MACRO_CONFIG_INT(RiChangeDummyColorWhenXDummyEqualXPlayer, ri_change_dummy_color_when_x_equal_dummy, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change color pos x hud dummy to green when x dummy = x player")
MACRO_CONFIG_INT(RiChangePlayerColorWhenXDummyEqualXPlayer, ri_change_player_color_when_x_equal_dummy, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change color pos x hud player to green when x dummy = x player")
MACRO_CONFIG_INT(RiAdvancedShowhudDummyActions, ri_advanced_showhud_dummy_actions, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show advanced ingame HUD (Advanced Dummy Actions)")

//Nameplates
MACRO_CONFIG_INT(RiShowFire, ri_show_fire, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Fire in nameplates (0=off, 1 = only dummy, 2 = both, 3 = only your own)")
MACRO_CONFIG_INT(RiFireDetectionSize, ri_fire_detection_size, 30, -50, 100, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Size of fire detection icons")
MACRO_CONFIG_INT(RiShowHook, ri_show_hook, 0, 0, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Hook in nameplates (0=off, 1 = other players', 2 = everyone, 3 = only your own)")
MACRO_CONFIG_INT(RiHookDetectionSize, ri_hook_detection_size, 30, -50, 100, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Size of hook detection icons")
MACRO_CONFIG_STR(RiNamePlateScheme, ri_nameplate_scheme, 32, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "The order in which to show nameplate items (p=ping i=ignore m=ID n=name c=clan d=direction f=friend h=hook r=reason s=skin H=HookName F=FireName l=newline)")
MACRO_CONFIG_INT(RiShowFireDynamic, ri_show_fire_dynamic, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Fire in nameplates will dynamically change pos")
MACRO_CONFIG_INT(RiShowHookDynamic, ri_show_fire_dynamic, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Hook in nameplates will dynamically change pos")
MACRO_CONFIG_INT(RiShowRclientIndicator, ri_show_rclient_indicator, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Rclient player indicator in nameplates")
MACRO_CONFIG_INT(RiRclientIndicatorSize, ri_rclient_indicator_size, 30, -50, 100, CFGFLAG_SAVE | CFGFLAG_CLIENT, "Size of rclient indicator icons")
MACRO_CONFIG_INT(RiShowIndicatorDynamic, ri_show_indicator_dynamic, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rclient indicator in nameplates will dynamically change pos")
MACRO_CONFIG_INT(RiRclientIndicatorAboveSelf, ri_rclient_indicator_above_self, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Rclient indicator above you")

//Changed TClient
MACRO_CONFIG_INT(RiIndicatorTransparentOffset, ri_indicator_transparent_offset, 42, 16, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "(16-128) Offset of indicator transparent position")
MACRO_CONFIG_INT(RiIndicatorTransparentOffsetMax, ri_indicator_transparent_offset_max, 100, 16, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "(16-128) Max indicator transparent offset for variable offset setting")
MACRO_CONFIG_INT(RiIndicatorTransparentToggle, ri_indicator_transparent_toggle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle player indicator transparent")
MACRO_CONFIG_INT(RiIndicatorTransparentMin, ri_indicator_transparent_min, 0, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Minimal transparent when offset max reached")
MACRO_CONFIG_INT(RiFrozenHudPosX, ri_frozen_hud_pos_x, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change frozen hud pos x")

// Chat Anim
MACRO_CONFIG_INT(RiChatAnim, ri_chat_anim, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle for chat animation")
MACRO_CONFIG_INT(RiChatAnimMs, ri_chat_anim_ms, 300, 100, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Chat animation long")

// Chat Bubbles
MACRO_CONFIG_INT(RiChatBubbles, ri_chat_bubbles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles above players")
MACRO_CONFIG_INT(RiChatBubblesSelf, ri_chat_bubbles_self, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles above you")
MACRO_CONFIG_INT(RiChatBubblesDemo, ri_chat_bubbles_demo, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles in demoplayer")
MACRO_CONFIG_INT(RiChatBubbleSize, ri_chat_bubble_size, 20, 20, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of the chat bubble")
MACRO_CONFIG_INT(RiChatBubbleShowTime, ri_chat_bubble_showtime, 200, 200, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long to show the bubble for")
MACRO_CONFIG_INT(RiChatBubbleFadeOut, ri_chat_bubble_fadeout, 35, 15, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long it fades out")
MACRO_CONFIG_INT(RiChatBubbleFadeIn, ri_chat_bubble_fadein, 15, 15, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "how long it fades in")

// Discord
MACRO_CONFIG_INT(RiDiscordMapStatus, ri_discord_map_status, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show What Map you're on")
MACRO_CONFIG_STR(RiDiscordOnlineStatus, ri_discord_online_status, 25, "Online", CFGFLAG_CLIENT | CFGFLAG_SAVE, "discord Online Status")
MACRO_CONFIG_STR(RiDiscordOfflineStatus, ri_discord_offline_status, 25, "Offline", CFGFLAG_CLIENT | CFGFLAG_SAVE, "discord Offline Status")

// RCON
MACRO_CONFIG_INT(RiPlaySounds, ri_play_sounds, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Plays sound when do command")

//Edge helper
MACRO_CONFIG_COL(RiEdgeInfoColorFreeze, ri_edge_info_color_freeze, 9930605, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Freeze color in edge info")
MACRO_CONFIG_COL(RiEdgeInfoColorKill, ri_edge_info_color_kill, 65461, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Kill color in edge info")
MACRO_CONFIG_COL(RiEdgeInfoColorSafe, ri_edge_info_color_safe, 5594535, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Safe color in edge info")
MACRO_CONFIG_INT(RiEdgeInfoCords, ri_edge_info_cords, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show upper panel of edge info")
MACRO_CONFIG_INT(RiEdgeInfoJump, ri_edge_info_jump, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show lower panel of edge info")
MACRO_CONFIG_INT(RiEdgeInfoPosX, ri_edge_info_pos_x, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change edge info pos x")
MACRO_CONFIG_INT(RiEdgeInfoPosY, ri_edge_info_pos_y, 56, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Change edge info pos y")

//Menus
MACRO_CONFIG_INT(RiBrFilterPlayers, ri_br_filter_players, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show only empty servers")

//Pulse Socket
MACRO_CONFIG_INT(RiCrossChatAutoConnect, ri_cross_chat_autoconnect, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Automatically connect to CrossChat on startup")
MACRO_CONFIG_INT(RiCrossChatInGameChat, ri_cross_chat_ingame_chat, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable CrossChat integration with in-game chat")
MACRO_CONFIG_INT(RiCrossChatDebug, ri_cross_chat_debug, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable CrossChat debug messages")
MACRO_CONFIG_STR(RiPulseSocketNameserver, ri_pulse_ntwr_nameserver, 256, "localhost:3000", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Pulse Nameserver for websockets")

//RClient Socket
MACRO_CONFIG_STR(RiIndicatorServerUrl, ri_indicator_server_url, 256, "localhost:3000", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Pulse Nameserver for websockets")

//Config tab
MACRO_CONFIG_INT(RiUiShowRClient, ri_ui_show_rclient, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show RClient domain in Configs tab")

//Spectator move
MACRO_CONFIG_INT(RiSpectatorMoveSpeed, ri_spectator_move_speed, 100, 10, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Speed of spectator freeview movement")
MACRO_CONFIG_INT(RiSpectatorMoveEnable, ri_spectator_move_enable, 100, 10, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable spectator movement binds")