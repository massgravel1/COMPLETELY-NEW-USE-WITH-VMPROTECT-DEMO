#pragma once

#include <windows.h>

#include "imgui.h"
#include "imgui_internal.h"

#ifdef FR_LAZER
#include "lazer/features/difficulty.h"
#else
#include "legacy/features/difficulty.h"
#endif // FR_LAZER

#include "memory.h"
#include "SSSISANIGGER.h"
#include "ui/debug_log.h"

extern int cfg_font_size;
extern int cfg_spins_per_minute;
extern bool cfg_mod_menu_visible;
extern float cfg_fraction_modifier;
extern bool cfg_replay_enabled;
extern bool cfg_replay_aim;
extern bool cfg_replay_keys;
extern bool cfg_replay_hardrock;
extern int cfg_relax_style;
extern bool cfg_relax_lock;
extern bool cfg_aimbot_lock;
extern bool cfg_relax_checks_od;
extern bool cfg_jumping_window;
extern float cfg_relax_alternate_percentage;  // Percentage of alternate clicks (0-100)
extern bool cfg_score_multiplier_enabled;
extern float cfg_score_multiplier_value;
extern bool cfg_drpc_enabled;
extern bool cfg_flashlight_enabled;
extern bool cfg_timewarp_enabled;
extern double cfg_timewarp_playback_rate;
extern bool cfg_hidden_remover_enabled;
extern bool cfg_show_debug_log;

// Aimbot humanization parameters
extern float cfg_humanize_reaction_delay;    // Reaction delay in milliseconds
extern float cfg_humanize_overshoot_chance;  // Chance to overshoot (0.0-1.0)
extern float cfg_humanize_overshoot_amount;  // Maximum overshoot percentage (0.0-1.0)
extern float cfg_humanize_micro_adjustment;  // Pixels for final micro-adjustment
extern float cfg_humanize_path_deviation;    // Path deviation amount (0.0-1.0)
extern int cfg_humanize_path_segments;       // Number of path segments
extern float cfg_humanize_acceleration;      // Acceleration factor
extern float cfg_humanize_deceleration;      // Deceleration factor
extern float cfg_humanize_jitter;            // Jitter amount (0.0-1.0)
extern int cfg_humanize_movement_type;       // Movement type (-1=random, 0=linear, 1=bezier, 2=catmull, 3=acceleration)

extern char cfg_drpc_state[512];
extern char cfg_drpc_large_text[512];
extern char cfg_drpc_small_text[512];
extern wchar_t drpc_state_wchar[512];
extern wchar_t drpc_large_text_wchar[512];
extern wchar_t drpc_small_text_wchar[512];

const char *get_imgui_ini_filename(HMODULE hMod);
void set_imgui_ini_handler();
