#include "ui/ui.h"

ImFont *font = 0;
HHOOK oWndProc = 0;
char song_name_u8[256] = "SSSISANIGGER " FR_VERSION " is Loading!";
static MenuTab current_tab = MenuTab::Difficulty;

static void update_tab(const char *tab_name, MenuTab tab_type, bool highlight = false)
{
    ImGui::PushID(tab_name);
    if (highlight)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
    if (ImGui::BeginTabItem(tab_name))
    {
        current_tab = tab_type;
        ImGui::EndTabItem();
    }
    if (highlight)
        ImGui::PopStyleColor();
    ImGui::PopID();
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(int code, WPARAM wparam, LPARAM lparam)
{
    if (code < 0)
        return CallNextHookEx(oWndProc, code, wparam, lparam);

    MSG *message = (MSG *)lparam;

    if (wparam == PM_REMOVE)
    {
        if (ImGui_ImplWin32_WndProcHandler(message->hwnd, message->message, message->wParam, message->lParam))
        {
            message->message = WM_NULL;
            return CallNextHookEx(oWndProc, code, wparam, lparam);
        }
    }

    if (message->message == WM_LBUTTONUP && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive())
    {
        ImGui::GetIO().MouseDrawCursor = false;
        ImGui::ClosePopupsOverWindow(0, false);
    }

    if ((ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsPopupOpen((ImGuiID)0, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
         && ((message->message >= WM_MOUSEFIRST && message->message <= WM_MOUSELAST) || message->message == WM_CHAR))
    {
        message->message = WM_NULL;
        return CallNextHookEx(oWndProc, code, wparam, lparam);
    }

    return CallNextHookEx(oWndProc, code, wparam, lparam);
}

inline void init_imgui_styles()
{
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    
    // Window settings
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowBorderSize = 1.0f;
    style.WindowRounding = 4.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    
    // Frame settings
    style.FrameRounding = 4.0f;
    style.FrameBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    
    // Item settings
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    
    // Scrollbar settings
    style.ScrollbarSize = 8.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 4.0f;
    
    // Colors
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 0.65f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.2f, 0.6f, 0.9f, 0.3f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
}

inline void init_imgui_fonts()
{
    ImGuiIO &io = ImGui::GetIO();
    ImFontConfig config;
    config.OversampleH = config.OversampleV = 1;
    config.PixelSnapH = true;
    config.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();

    for (int size = 40; size >= 18; size -= 2)
    {
        config.SizePixels = size;
        ImFont *f = io.Fonts->AddFontFromMemoryCompressedBase85TTF(victor_mono_font_compressed_data_base85, size, &config);
        if (size == cfg_font_size)
            font = f;
    }
}

void init_ui()
{
    oWndProc = SetWindowsHookExA(WH_GETMESSAGE, &WndProc, GetModuleHandleA(nullptr), GetCurrentThreadId());

#ifdef FR_DEBUG
    IMGUI_CHECKVERSION();
#endif // FR_DEBUG
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ctx->SettingsHandlers.clear();

    set_imgui_ini_handler();
    io.IniFilename = get_imgui_ini_filename(g_module);
    if (io.IniFilename == 0)
        FR_ERROR("Couldn't get config path");
    else
        ImGui::LoadIniSettingsFromDisk(io.IniFilename);

    init_imgui_fonts();
    init_imgui_styles();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplOpenGL3_Init();
}

void init_ui(IDirect3DDevice9* pDevice)
{
    oWndProc = SetWindowsHookExA(WH_GETMESSAGE, &WndProc, GetModuleHandleA(nullptr), GetCurrentThreadId());

#ifdef FR_DEBUG
    IMGUI_CHECKVERSION();
#endif // FR_DEBUG
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ctx->SettingsHandlers.clear();

    set_imgui_ini_handler();
    io.IniFilename = get_imgui_ini_filename(g_module);
    if (io.IniFilename == 0)
        FR_ERROR("Couldn't get config path");
    else
        ImGui::LoadIniSettingsFromDisk(io.IniFilename);

    init_imgui_fonts();
    init_imgui_styles();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX9_Init(pDevice);
}

static void colored_if_null(const char *label, uintptr_t ptr, bool draw_label = true)
{
    uintptr_t found = ptr;
    if (!found)
        ImGui::PushStyleColor(ImGuiCol_Text, ITEM_UNAVAILABLE);

    char id_str[64] = {0};
    IM_ASSERT(strlen(label) < IM_ARRAYSIZE(id_str));
    ImFormatString(id_str, IM_ARRAYSIZE(id_str), "##%s", label);

    char ptr_str[32] = {0};
    ImFormatString(ptr_str, IM_ARRAYSIZE(ptr_str), "%08X", ptr);

    if (draw_label)
    {
        ImGui::Text(label);
        ImGui::SameLine(ImGui::GetFontSize() * 8.f);
    }
    ImGui::PushItemWidth(ImGui::GetFontSize() * 4.f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(BLACK_TRANSPARENT));
    ImGui::InputText(id_str, ptr_str, 32, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    if (!found)
        ImGui::PopStyleColor();
}

static inline bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
{
    ImGui::PushItemWidth(ImGui::GetFontSize() * 16.f);
    bool value_changed = ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
    ImGui::PopItemWidth();
    return value_changed;
}

static void parameter_slider(uintptr_t selected_song_ptr, DifficultySetting *p)
{
    const char *fmt;
    if (!p->found)
    {
        ImGui::BeginDisabled();
        fmt = p->error;
    }
    else
    {
        fmt = p->fmt;
    }
    if (!p->enabled)
    {
        if (p->found && selected_song_ptr)
        {
            uintptr_t param_ptr = 0;
            if (internal_memory_read(g_process, selected_song_ptr, &param_ptr))
            {
                param_ptr += p->offset;
                internal_memory_read(g_process, param_ptr, &p->value);
            }
        }
        ImGui::PushID(fmt);
        ImGui::BeginDisabled();
        SliderFloat("", &p->value, .0f, 11.0f, fmt);
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    else
    {
        ImGui::PushID(fmt);
        if (SliderFloat("", &p->value, .0f, 11.0f, fmt))
            p->apply_mods();
        ImGui::PopID();
        if (ImGui::IsItemDeactivatedAfterEdit())
            ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
    }
    ImGui::SameLine();
    ImGui::PushID(p->offset);
    if (ImGui::Checkbox("", &p->enabled))
    {
        p->enabled ? p->enable() : p->disable();
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
    }
    ImGui::PopID();
    if (!p->found)
        ImGui::EndDisabled();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
}

void update_ui()
{
    if (!cfg_mod_menu_visible)
        return;

    if (selected_song_ptr)
    {
        uintptr_t song_str_ptr = 0;
        if (internal_memory_read(g_process, selected_song_ptr, &song_str_ptr))
        {
            song_str_ptr += 0x80;
            static uintptr_t prev_song_str_ptr = 0;
            if (song_str_ptr != prev_song_str_ptr)
            {
                uintptr_t song_str = 0;
                if (internal_memory_read(g_process, song_str_ptr, &song_str))
                {
                    song_str += 0x4;
                    uint32_t song_str_length = 0;
                    if (internal_memory_read(g_process, song_str, &song_str_length))
                    {
                        song_str += 0x4;
                        int bytes_written = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)song_str, song_str_length, song_name_u8, 255, 0, 0);
                        song_name_u8[bytes_written] = '\0';
                    }
                }
                prev_song_str_ptr = song_str_ptr;
            }
        }
    }

    ImGui::PushFont(font);

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
    ImGui::Begin("SSSISANIGGER", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s", song_name_u8);

    if (memory_scan_progress < .99f)
    {
        static char overlay_buf[32] = {0};
        ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "Memory Scan: %.0f%%", memory_scan_progress * 100 + 0.01f);
        ImGui::ProgressBar(memory_scan_progress, ImVec2(ImGui::GetContentRegionAvail().x, .0f), overlay_buf);
    }

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + ImGui::GetWindowHeight()), ImGuiCond_Appearing);
    if (ImGui::BeginPopupContextItem("##settings"))
    {
        static MenuTab selected_tab = MenuTab::Difficulty;

        const auto update_tab = [](const char *tab_name, MenuTab tab_type, bool highlight = false)
        {
            bool is_selected = selected_tab == tab_type;
            if (!is_selected && highlight)
                ImGui::PushStyleColor(ImGuiCol_Text, SILVER);
            if (ImGui::Selectable(tab_name, is_selected, ImGuiSelectableFlags_NoAutoClosePopups))
            {
                selected_tab = tab_type;
                ImGui::SetNextWindowFocus();
            }
            if (!is_selected && highlight)
                ImGui::PopStyleColor();
        };

        const auto inactive_tab = [](const char *tab_name)
        {
            ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Text, LOG_ERROR);
            ImGui::Selectable(tab_name, false, ImGuiSelectableFlags_NoAutoClosePopups);
            ImGui::PopStyleColor();
            ImGui::EndDisabled();
        };

        update_tab("Difficulty", MenuTab::Difficulty, ar_setting.enabled || cs_setting.enabled || od_setting.enabled);

        beatmap_onload_offset ? update_tab("Relax",  MenuTab::Relax, cfg_relax_lock)  : inactive_tab("Relax");
        beatmap_onload_offset ? update_tab("Aimbot", MenuTab::Aimbot, cfg_aimbot_lock) : inactive_tab("Aimbot");
        beatmap_onload_offset ? update_tab("Aimbot Humanization", MenuTab::AimbotHumanization) : inactive_tab("Aimbot Humanization");
        selected_replay_offset ? update_tab("Replay", MenuTab::Replay, cfg_replay_enabled) : inactive_tab("Replay");

        update_tab("About", MenuTab::About);

        if (ImGui::Selectable("Debug", false, ImGuiSelectableFlags_NoAutoClosePopups))
        {
            cfg_show_debug_log = true;
            ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(ImGui::GetFontSize() * 18.f, ImGui::GetWindowHeight()));
        ImGui::SetNextWindowSize(ImVec2(.0f, .0f), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), ImGui::GetWindowPos().y), ImGuiCond_Always);
        ImGui::Begin("##tab_content", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::PopStyleVar();
        if (selected_tab == MenuTab::Difficulty)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetFontSize() * .25f, ImGui::GetFontSize() * .25f));
            parameter_slider(selected_song_ptr, &ar_setting);
            parameter_slider(selected_song_ptr, &cs_setting);
            parameter_slider(selected_song_ptr, &od_setting);
            ImGui::PopStyleVar();
        }
        if (selected_tab == MenuTab::Relax)
        {
            if (ImGui::Checkbox("Enable", &cfg_relax_lock))
            {
                cfg_relax_lock ? enable_notify_hooks() : disable_notify_hooks();
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::PushItemWidth(ImGui::CalcTextSize("X").x * 1.85f);
            ImGui::InputText("Left Click",  left_click,  2, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll);
            ImGui::InputText("Right Click", right_click, 2, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AutoSelectAll);
            ImGui::PopItemWidth();
            ImGui::Dummy(ImVec2(.0f, 5.f));
            if (ImGui::RadioButton("SingleTap", &cfg_relax_style, 's'))
            {
                FR_INFO("SingleTap Mode");
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Alternate", &cfg_relax_style, 'a'))
            {
                FR_INFO("Alternate Mode");
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(.0f, 5.f));
            
            // Add slider for alternate percentage
            ImGui::PushItemWidth(ImGui::GetFontSize() * 16.f);
            if (ImGui::SliderFloat("##alternate_percentage", &cfg_relax_alternate_percentage, 0.0f, 100.0f, "Alternate Percentage: %.1f%%"))
            {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::PopItemWidth();
            ImGui::Dummy(ImVec2(.0f, 5.f));
            
            if (ImGui::Checkbox("Variable Unstable Rate", &cfg_relax_checks_od))
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            ImGui::Dummy(ImVec2(.0f, 5.f));
            bool relax_checks_od = cfg_relax_checks_od;
            if (!relax_checks_od)
                ImGui::BeginDisabled();
            if (ImGui::Checkbox("Jumping Unstable Rate Window", &cfg_jumping_window))
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            if (!relax_checks_od)
                ImGui::EndDisabled();
        }
        if (selected_tab == MenuTab::Aimbot)
        {
            if (ImGui::Checkbox("Enable", &cfg_aimbot_lock))
            {
                cfg_aimbot_lock ? enable_notify_hooks() : disable_notify_hooks();
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            SliderFloat("##fraction_modifier", &cfg_fraction_modifier, .01f, 5.f, "Cursor Delay: %.2f");
            if (ImGui::IsItemDeactivatedAfterEdit())
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            ImGui::Dummy(ImVec2(.0f, .5f));
            ImGui::PushItemWidth(ImGui::GetFontSize() * 16.f);
            ImGui::SliderInt("##spins_per_minute", &cfg_spins_per_minute, 0, 600, "Spins Per Minute: %d");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Set To Zero To Disable");
            ImGui::PopItemWidth();
            if (ImGui::IsItemDeactivatedAfterEdit())
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        }
        if (selected_tab == MenuTab::AimbotHumanization)
        {
            // Only enable if aimbot is enabled
            if (!cfg_aimbot_lock)
                ImGui::BeginDisabled();
                
            ImGui::Text("Aimbot Humanization Settings");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            // Movement type selection
            const char* movement_types[] = { "Random", "Linear", "Bezier Curve", "Catmull-Rom Spline", "Acceleration Profile" };
            static int movement_type_idx = cfg_humanize_movement_type + 1; // +1 because -1 is random
            
            ImGui::Text("Movement Pattern");
            ImGui::PushItemWidth(ImGui::GetFontSize() * 16.f);
            if (ImGui::Combo("##movement_type", &movement_type_idx, movement_types, IM_ARRAYSIZE(movement_types))) {
                cfg_humanize_movement_type = movement_type_idx - 1; // -1 to adjust for random being -1
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::PopItemWidth();
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            // Reaction time
            ImGui::PushItemWidth(ImGui::GetFontSize() * 16.f);
            if (ImGui::SliderFloat("##reaction_delay", &cfg_humanize_reaction_delay, 0.0f, 100.0f, "Reaction Delay: %.1f ms")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            // Overshoot settings
            if (ImGui::SliderFloat("##overshoot_chance", &cfg_humanize_overshoot_chance, 0.0f, 1.0f, "Overshoot Chance: %.2f")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            
            if (ImGui::SliderFloat("##overshoot_amount", &cfg_humanize_overshoot_amount, 0.0f, 0.3f, "Overshoot Amount: %.2f")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            // Path settings
            if (ImGui::SliderFloat("##path_deviation", &cfg_humanize_path_deviation, 0.0f, 0.5f, "Path Deviation: %.2f")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            
            if (ImGui::SliderInt("##path_segments", &cfg_humanize_path_segments, 2, 10, "Path Segments: %d")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            // Micro-adjustment
            if (ImGui::SliderFloat("##micro_adjustment", &cfg_humanize_micro_adjustment, 0.0f, 10.0f, "Micro Adjustment: %.1f px")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            // Advanced settings
            ImGui::Text("Advanced Movement Settings");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            if (ImGui::SliderFloat("##acceleration", &cfg_humanize_acceleration, 1.0f, 4.0f, "Acceleration: %.1f")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            
            if (ImGui::SliderFloat("##deceleration", &cfg_humanize_deceleration, 1.0f, 4.0f, "Deceleration: %.1f")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            
            if (ImGui::SliderFloat("##jitter", &cfg_humanize_jitter, 0.0f, 2.0f, "Jitter Amount: %.1f")) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            ImGui::PopItemWidth();
            
            // Reset to defaults button
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            if (ImGui::Button("Reset to Defaults")) {
                cfg_humanize_reaction_delay = 20.0f;
                cfg_humanize_overshoot_chance = 0.25f;
                cfg_humanize_overshoot_amount = 0.08f;
                cfg_humanize_micro_adjustment = 3.0f;
                cfg_humanize_path_deviation = 0.15f;
                cfg_humanize_path_segments = 4;
                cfg_humanize_acceleration = 2.0f;
                cfg_humanize_deceleration = 1.5f;
                cfg_humanize_jitter = 0.7f;
                cfg_humanize_movement_type = -1;
                movement_type_idx = 0; // Reset combobox to Random
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            
            if (!cfg_aimbot_lock)
                ImGui::EndDisabled();
        }
        if (selected_tab == MenuTab::Replay)
        {
            ImGui::Text("%s", current_replay.song_name_u8);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Selected Replay");
            ImGui::Text("%s - %.2f%% - %ux - %s", current_replay.author, current_replay.accuracy, current_replay.combo, current_replay.mods);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Player, Accuracy, Mods");
            ImGui::Dummy(ImVec2(.0f, 2.f));
            if (ImGui::Checkbox("Enable", &cfg_replay_enabled))
            {
                cfg_replay_enabled ? enable_replay_hooks() : disable_replay_hooks();
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Usage: Open Replay Preview in-game to Select a Replay");
            ImGui::SameLine(ImGui::GetFontSize() * 8.f);
            if (!cfg_replay_enabled)
                ImGui::BeginDisabled();
            if (ImGui::Checkbox("Hardrock", &cfg_replay_hardrock))         current_replay.toggle_hardrock();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Convert Replay to/from Hardrock");
            ImGui::Dummy(ImVec2(.0f, 2.f));
            if (ImGui::Checkbox("Replay Aim", &cfg_replay_aim))            ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Aim According to Replay Data");
            ImGui::SameLine(ImGui::GetFontSize() * 8.f);
            if (ImGui::Checkbox("Replay Keys", &cfg_replay_keys))          ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Press Keys According to Replay Data");
            if (!cfg_replay_enabled)
                ImGui::EndDisabled();
        }
        if (selected_tab == MenuTab::About)
        {
            ImGui::Text("SSSISANIGGER's SSSISANIGGER " FR_VERSION);
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
        }
        ImGui::End(); // tab_content
        ImGui::EndPopup();
    }

    ImGui::End(); // SSSISANIGGER
    ImGui::PopFont();
}

void destroy_ui()
{
    UnhookWindowsHookEx(oWndProc);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void draw_debug_log()
{
    // if (cfg_show_debug_log)
    // {
    //     ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), ImGui::GetWindowPos().y), ImGuiCond_Once);
    //     ImGui::SetNextWindowSize(ImVec2(640.f,480.f), ImGuiCond_Once);
    //     ImGui::PushFont(font);
    //     ImGui::Begin("Debug", &cfg_show_debug_log, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    //     ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.f, ImGui::GetStyle().FramePadding.y));
    //     if (ImGui::BeginTabBar("##debug_tabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTabListScrollingButtons))
    //     {
    //         if (ImGui::BeginTabItem("Log"))
    //         {
    //             ImGui::PopStyleVar(); // FramePadding
    //             debug_log.draw();
    //             ImGui::EndTabItem();
    //         }
    //         if (ImGui::BeginTabItem("Game"))
    //         {
    //             ImGui::PopStyleVar();
    //             ImGui::BeginChild("##debug_game", ImVec2(.0f, -30.f));
    //             ImGui::Text("Audio Time: %d", audio_time_ptr ? *(int32_t *)audio_time_ptr : 0);
    //             const auto scene_ptr_to_str = [](Scene *s){
    //                 if (!s) return "Unknown";
    //                 Scene scene = *s;
    //                 switch (scene)
    //                 {
    //                     case Scene::MAIN_MENU: return "Main Menu";
    //                     case Scene::EDITOR: return "Editor";
    //                     case Scene::GAME: return "Game";
    //                     case Scene::EXIT: return "Exit";
    //                     case Scene::EDITOR_BEATMAP_SELECT: return "Editor Beatmap Select";
    //                     case Scene::BEATMAP_SELECT: return "Beatmap Select";
    //                     case Scene::BEATMAP_SELECT_DRAWINGS: return "Beatmap Select Drawings";
    //                     case Scene::REPLAY_PREVIEW: return "Replay Preview";
    //                     default:
    //                         return "Unknown";
    //                 }
    //             };
    //             ImGui::Text("Current Scene: %s", scene_ptr_to_str(current_scene_ptr));
    //             ImGui::EndChild();
    //             ImGui::EndTabItem();
    //         }
    //         ImGui::EndTabBar();
    //     }
    //     ImGui::PopStyleVar(); // FramePadding
    //     ImGui::End();
    // }
}

void draw_mod_menu()
{
    // ... existing code ...

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTabListScrollingButtons))
    {
        update_tab("Difficulty", MenuTab::Difficulty, ar_setting.enabled || cs_setting.enabled || od_setting.enabled);
        update_tab("Relax", MenuTab::Relax, cfg_relax_lock);
        update_tab("Aimbot", MenuTab::Aimbot, cfg_aimbot_lock);
        // update_tab("Timewarp", MenuTab::Timewarp, cfg_timewarp_enabled);
        update_tab("Replay", MenuTab::Replay, cfg_replay_enabled);
        // update_tab("Mods", MenuTab::Mods, cfg_flashlight_enabled || cfg_hidden_remover_enabled || cfg_score_multiplier_enabled);
        // update_tab("Misc", MenuTab::Misc, cfg_drpc_enabled);
        update_tab("About", MenuTab::About);

        // if (ImGui::Selectable("Debug", false, ImGuiSelectableFlags_NoAutoClosePopups))
        // {
        //     cfg_show_debug_log = true;
        //     ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        // }

        // ... rest of the code ...
    }
}
