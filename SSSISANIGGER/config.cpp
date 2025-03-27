#include "ui/config.h"

int cfg_font_size = 30;
int cfg_spins_per_minute = 300;
bool cfg_mod_menu_visible = true;
float cfg_fraction_modifier = .5f;
bool cfg_replay_enabled = false;
bool cfg_replay_aim = true;
bool cfg_replay_keys = true;
bool cfg_replay_hardrock = false;
int cfg_relax_style = 'a'; // alternate
bool cfg_score_multiplier_enabled = false;
float cfg_score_multiplier_value = 1.f;
bool cfg_drpc_enabled = false;
bool cfg_flashlight_enabled = false;
bool cfg_timewarp_enabled = false;
double cfg_timewarp_playback_rate = 200.0;
bool cfg_relax_checks_od = true;
bool cfg_jumping_window = false;
bool cfg_relax_lock = false;
float cfg_relax_alternate_percentage = 50.0f;  // Default to 50% alternate
bool cfg_aimbot_lock = false;
bool cfg_hidden_remover_enabled = false;
bool cfg_show_debug_log = false;

// Aimbot humanization parameters with reasonable defaults
float cfg_humanize_reaction_delay = 20.0f;      // Reaction delay in milliseconds
float cfg_humanize_overshoot_chance = 0.25f;    // 25% chance to overshoot
float cfg_humanize_overshoot_amount = 0.08f;    // Up to 8% overshoot
float cfg_humanize_micro_adjustment = 3.0f;     // 3px final adjustment
float cfg_humanize_path_deviation = 0.15f;      // 15% path deviation
int cfg_humanize_path_segments = 4;             // 4 path segments
float cfg_humanize_acceleration = 2.0f;         // Acceleration factor
float cfg_humanize_deceleration = 1.5f;         // Deceleration factor
float cfg_humanize_jitter = 0.7f;               // Jitter amount 
int cfg_humanize_movement_type = -1;            // Movement type (-1 = random)

char cfg_drpc_state[512] = {0};
char cfg_drpc_large_text[512] = {0};
char cfg_drpc_small_text[512] = {0};
wchar_t drpc_state_wchar[512] = {0};
wchar_t drpc_large_text_wchar[512] = {0};
wchar_t drpc_small_text_wchar[512] = {0};

const char *get_imgui_ini_filename(HMODULE hMod)
{
    static wchar_t module_path[MAX_PATH * 2];
    DWORD module_path_length = GetModuleFileNameW(hMod, module_path, MAX_PATH * 2);
    if (module_path_length == 0)
    {
        FR_ERROR("GetModuleFileName (0x%X)", GetLastError());

        // NOTE(SSSISANIGGER): config path from SSSISANIGGER_injector
        extern LPVOID g_config_path;
        if (g_config_path == NULL)
            return 0;

        uint8_t test_byte = 0;
        if (!internal_memory_read(g_process, (uintptr_t)g_config_path, &test_byte))
            return 0;

        module_path_length = (DWORD)wcslen((wchar_t *)g_config_path);
        if (module_path_length == 0)
            return 0;

        memcpy(module_path, g_config_path, (module_path_length + 1) * sizeof(wchar_t));
        SecureZeroMemory(g_config_path, (module_path_length + 1) * sizeof(wchar_t));
        VirtualFreeEx(g_process, g_config_path, 0, MEM_RELEASE);
    }

    static char module_path_u8[MAX_PATH * 2];
    int module_path_u8_length = WideCharToMultiByte(CP_UTF8, 0, module_path, module_path_length, module_path_u8, MAX_PATH * 2, 0, 0);
    if (module_path_u8_length == 0)
        return 0;

    module_path_u8[module_path_u8_length] = '\0';

    DWORD backslash_index = module_path_u8_length - 1;
    while (backslash_index)
        if (module_path_u8[--backslash_index] == '\\')
            break;

    memcpy(module_path_u8 + backslash_index + 1, "config.ini", sizeof("config.ini"));
    FR_INFO("config.ini path: %s", module_path_u8);

    return (const char *)&module_path_u8;
}

static void ConfigHandler_ClearAll(ImGuiContext *, ImGuiSettingsHandler *) {}
static void ConfigHandler_ApplyAll(ImGuiContext *, ImGuiSettingsHandler *) {}
static void *ConfigHandler_ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *) { return (void *)1; }

static void ConfigHandler_WriteAll(ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
    buf->reserve(buf->size() + (1 + 4) * 2);
    buf->appendf("[%s][%s]\n", handler->TypeName, "Settings");
    buf->appendf("ar_lock=%d\n", (int)ar_setting.enabled);
    buf->appendf("ar_value=%.1f\n", ar_setting.value);
    buf->appendf("cs_lock=%d\n", (int)cs_setting.enabled);
    buf->appendf("cs_value=%.1f\n", cs_setting.value);
    buf->appendf("od_lock=%d\n", (int)od_setting.enabled);
    buf->appendf("od_value=%.1f\n", od_setting.value);
    buf->appendf("visible=%d\n", cfg_mod_menu_visible);
    buf->appendf("font_size=%d\n", cfg_font_size);
    buf->appendf("relax=%d\n", cfg_relax_lock);
    buf->appendf("relax_style=%c\n", (char)cfg_relax_style);
    buf->appendf("relax_alternate_percentage=%.1f\n", cfg_relax_alternate_percentage);
    buf->appendf("relax_checks_od=%d\n", (int)cfg_relax_checks_od);
    buf->appendf("aimbot=%d\n", cfg_aimbot_lock);
    buf->appendf("spins_per_minute=%d\n", cfg_spins_per_minute);
    buf->appendf("fraction_modifier=%.2f\n", cfg_fraction_modifier);
    buf->appendf("replay=%d\n", (int)cfg_replay_enabled);
    buf->appendf("replay_aim=%d\n", (int)cfg_replay_aim);
    buf->appendf("replay_keys=%d\n", (int)cfg_replay_keys);
    buf->appendf("sm_lock=%d\n", (int)cfg_score_multiplier_enabled);
    buf->appendf("sm_value=%.2f\n", cfg_score_multiplier_value);
    buf->appendf("drpc=%d\n", (int)cfg_drpc_enabled);
    buf->appendf("drpc_state=%s\n", cfg_drpc_state);
    buf->appendf("drpc_large=%s\n", cfg_drpc_large_text);
    buf->appendf("drpc_small=%s\n", cfg_drpc_small_text);
    buf->appendf("fl=%d\n", (int)cfg_flashlight_enabled);
    buf->appendf("hd=%d\n", (int)cfg_hidden_remover_enabled);
    buf->appendf("tw_lock=%d\n", (int)cfg_timewarp_enabled);
    buf->appendf("tw_value=%.1lf\n", cfg_timewarp_playback_rate);
    buf->appendf("jump_window=%d\n", (int)cfg_jumping_window);
    buf->appendf("show_debug=%d\n", (int)cfg_show_debug_log);
    
    // Aimbot humanization parameters
    buf->appendf("humanize_reaction_delay=%.1f\n", cfg_humanize_reaction_delay);
    buf->appendf("humanize_overshoot_chance=%.2f\n", cfg_humanize_overshoot_chance);
    buf->appendf("humanize_overshoot_amount=%.2f\n", cfg_humanize_overshoot_amount);
    buf->appendf("humanize_micro_adjustment=%.1f\n", cfg_humanize_micro_adjustment);
    buf->appendf("humanize_path_deviation=%.2f\n", cfg_humanize_path_deviation);
    buf->appendf("humanize_path_segments=%d\n", cfg_humanize_path_segments);
    buf->appendf("humanize_acceleration=%.1f\n", cfg_humanize_acceleration);
    buf->appendf("humanize_deceleration=%.1f\n", cfg_humanize_deceleration);
    buf->appendf("humanize_jitter=%.1f\n", cfg_humanize_jitter);
    buf->appendf("humanize_movement_type=%d\n", cfg_humanize_movement_type);
    
    buf->append("\n");
}

static void ConfigHandler_ReadLine(ImGuiContext *, ImGuiSettingsHandler *, void *, const char *line)
{
    int ar_lock_i, cs_lock_i, od_lock_i, mod_menu_visible_i, font_size_i,
        relax_lock_i, aimbot_lock_i, spins_per_minute_i, drpc_enabled_i,
        hidden_remover_enabled_i, flashlight_enabled_i, timewarp_enabled_i, relax_checks_od_i,
        jump_window_i, replay_i, replay_aim_i, replay_keys_i, score_multiplier_i,
        show_debug_i;
    float ar_value_f, cs_value_f, od_value_f, fraction_modifier_f, score_multiplier_value_f;
    double timewarp_playback_rate_d;
    char relax_style_c;
    
    // Humanization parameters
    float humanize_reaction_delay_f, humanize_overshoot_chance_f, humanize_overshoot_amount_f,
          humanize_micro_adjustment_f, humanize_path_deviation_f, humanize_acceleration_f,
          humanize_deceleration_f, humanize_jitter_f;
    int humanize_path_segments_i, humanize_movement_type_i;
    float relax_alternate_percentage_f;
    
    if (sscanf(line, "ar_lock=%d", &ar_lock_i) == 1)                          ar_setting.enabled = ar_lock_i;
    else if (sscanf(line, "ar_value=%f", &ar_value_f) == 1)                   ar_setting.value = ar_value_f;
    else if (sscanf(line, "cs_lock=%d", &cs_lock_i) == 1)                     cs_setting.enabled = cs_lock_i;
    else if (sscanf(line, "cs_value=%f", &cs_value_f) == 1)                   cs_setting.value = cs_value_f;
    else if (sscanf(line, "od_lock=%d", &od_lock_i) == 1)                     od_setting.enabled = od_lock_i;
    else if (sscanf(line, "od_value=%f", &od_value_f) == 1)                   od_setting.value = od_value_f;
    else if (sscanf(line, "visible=%d", &mod_menu_visible_i) == 1)            cfg_mod_menu_visible = mod_menu_visible_i;
    else if (sscanf(line, "font_size=%d", &font_size_i) == 1)                 cfg_font_size = font_size_i;
    else if (sscanf(line, "relax=%d", &relax_lock_i) == 1)                    cfg_relax_lock = relax_lock_i;
    else if (sscanf(line, "relax_style=%c", &relax_style_c) == 1)             cfg_relax_style = (int)relax_style_c;
    else if (sscanf(line, "relax_alternate_percentage=%f", &relax_alternate_percentage_f) == 1) cfg_relax_alternate_percentage = relax_alternate_percentage_f;
    else if (sscanf(line, "relax_checks_od=%d", &relax_checks_od_i) == 1)     cfg_relax_checks_od = relax_checks_od_i;
    else if (sscanf(line, "aimbot=%d", &aimbot_lock_i) == 1)                  cfg_aimbot_lock = aimbot_lock_i;
    else if (sscanf(line, "spins_per_minute=%d", &spins_per_minute_i) == 1)   cfg_spins_per_minute = spins_per_minute_i;
    else if (sscanf(line, "fraction_modifier=%f", &fraction_modifier_f) == 1) cfg_fraction_modifier = fraction_modifier_f;
    else if (sscanf(line, "replay=%d", &replay_i) == 1)                       cfg_replay_enabled = replay_i;
    else if (sscanf(line, "replay_aim=%d", &replay_aim_i) == 1)               cfg_replay_aim = replay_aim_i;
    else if (sscanf(line, "replay_keys=%d", &replay_keys_i) == 1)             cfg_replay_keys = replay_keys_i;
    else if (sscanf(line, "sm_lock=%d", &score_multiplier_i) == 1)            cfg_score_multiplier_enabled = score_multiplier_i;
    else if (sscanf(line, "sm_value=%f", &score_multiplier_value_f) == 1)     cfg_score_multiplier_value = score_multiplier_value_f;
    else if (sscanf(line, "drpc=%d", &drpc_enabled_i) == 1)  cfg_drpc_enabled = drpc_enabled_i;
    else if (sscanf(line, "drpc_state=%511[^\n]", cfg_drpc_state) == 1) {}
    else if (sscanf(line, "drpc_large=%511[^\n]", cfg_drpc_large_text) == 1) {}
    else if (sscanf(line, "drpc_small=%511[^\n]", cfg_drpc_small_text) == 1) {}
    else if (sscanf(line, "fl=%d", &flashlight_enabled_i) == 1)               cfg_flashlight_enabled = flashlight_enabled_i;
    else if (sscanf(line, "hd=%d", &hidden_remover_enabled_i) == 1)           cfg_hidden_remover_enabled = hidden_remover_enabled_i;
    else if (sscanf(line, "tw_lock=%d", &timewarp_enabled_i) == 1)            cfg_timewarp_enabled = timewarp_enabled_i;
    else if (sscanf(line, "tw_value=%lf", &timewarp_playback_rate_d) == 1)    cfg_timewarp_playback_rate = timewarp_playback_rate_d;
    else if (sscanf(line, "jump_window=%d", &jump_window_i) == 1)             cfg_jumping_window = jump_window_i;
    else if (sscanf(line, "show_debug=%d", &show_debug_i) == 1)               cfg_show_debug_log = show_debug_i;
    
    // Aimbot humanization parameters
    else if (sscanf(line, "humanize_reaction_delay=%f", &humanize_reaction_delay_f) == 1)      cfg_humanize_reaction_delay = humanize_reaction_delay_f;
    else if (sscanf(line, "humanize_overshoot_chance=%f", &humanize_overshoot_chance_f) == 1)   cfg_humanize_overshoot_chance = humanize_overshoot_chance_f;
    else if (sscanf(line, "humanize_overshoot_amount=%f", &humanize_overshoot_amount_f) == 1)   cfg_humanize_overshoot_amount = humanize_overshoot_amount_f;
    else if (sscanf(line, "humanize_micro_adjustment=%f", &humanize_micro_adjustment_f) == 1)   cfg_humanize_micro_adjustment = humanize_micro_adjustment_f;
    else if (sscanf(line, "humanize_path_deviation=%f", &humanize_path_deviation_f) == 1)       cfg_humanize_path_deviation = humanize_path_deviation_f;
    else if (sscanf(line, "humanize_path_segments=%d", &humanize_path_segments_i) == 1)         cfg_humanize_path_segments = humanize_path_segments_i;
    else if (sscanf(line, "humanize_acceleration=%f", &humanize_acceleration_f) == 1)           cfg_humanize_acceleration = humanize_acceleration_f;
    else if (sscanf(line, "humanize_deceleration=%f", &humanize_deceleration_f) == 1)           cfg_humanize_deceleration = humanize_deceleration_f;
    else if (sscanf(line, "humanize_jitter=%f", &humanize_jitter_f) == 1)                       cfg_humanize_jitter = humanize_jitter_f;
    else if (sscanf(line, "humanize_movement_type=%d", &humanize_movement_type_i) == 1)         cfg_humanize_movement_type = humanize_movement_type_i;
}

void set_imgui_ini_handler()
{
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Config";
    ini_handler.TypeHash = ImHashStr("Config");
    ini_handler.ClearAllFn = ConfigHandler_ClearAll;
    ini_handler.ReadOpenFn = ConfigHandler_ReadOpen;
    ini_handler.ReadLineFn = ConfigHandler_ReadLine;
    ini_handler.ApplyAllFn = ConfigHandler_ApplyAll;
    ini_handler.WriteAllFn = ConfigHandler_WriteAll;
    ImGui::AddSettingsHandler(&ini_handler);
}
