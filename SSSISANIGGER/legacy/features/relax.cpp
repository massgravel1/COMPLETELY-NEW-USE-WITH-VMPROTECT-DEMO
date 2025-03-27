#include "legacy/features/relax.h"
#include "legacy/window.h"

float od_window = 5.f;
float od_window_left_offset = .0f;
float od_window_right_offset = .0f;
float od_check_ms = .0f;

float jumping_window_offset = .0f;

int wait_hitobjects_min = 2;
int wait_hitobjects_max = 5;

static char current_click = cfg_relax_style == 'a' ? right_click[0] : left_click[0];

// Helper function to check if cursor is on circle with some tolerance
static bool is_cursor_on_circle(const Vector2<float>& circle_pos, float radius) {
    Vector2<float> mouse_pos = mouse_position();
    Vector2<float> screen_pos = playfield_to_screen(circle_pos);
    
    // Calculate distance between cursor and circle center
    float dx = mouse_pos.x - screen_pos.x;
    float dy = mouse_pos.y - screen_pos.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    // Add a small tolerance (95% of radius) to account for slight movement
    return distance <= radius * 0.95f;
}

// Helper function to check if we're approaching a circle
static bool is_approaching_circle(const Vector2<float>& circle_pos, float radius) {
    Vector2<float> mouse_pos = mouse_position();
    Vector2<float> screen_pos = playfield_to_screen(circle_pos);
    
    // Calculate distance between cursor and circle center
    float dx = mouse_pos.x - screen_pos.x;
    float dy = mouse_pos.y - screen_pos.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    // Consider "approaching" if within 150% of radius
    return distance <= radius * 1.5f;
}

void calc_od_timing()
{
    const auto rand_range_f = [](float f_min, float f_max) -> float
    {
        float scale = rand() / (float)RAND_MAX;
        return f_min + scale * (f_max - f_min);
    };
    const auto rand_range_i = [](int i_min, int i_max) -> int
    {
        return rand() % (i_max + 1 - i_min) + i_min;
    };
    if (cfg_relax_checks_od && (od_check_ms == .0f))
    {
        od_check_ms = rand_range_f(od_window_left_offset, od_window_right_offset);
        if (cfg_jumping_window)
        {
            static uint32_t hit_objects_passed = current_beatmap.hit_object_idx;
            static int wait_hitojects_count = rand_range_i(wait_hitobjects_min, wait_hitobjects_max);
            if (current_beatmap.hit_object_idx - hit_objects_passed >= wait_hitojects_count)
            {
                // NOTE(SSSISANIGGER): move od window to the left
                if (rand_range_i(0, 1) >= 1)
                    jumping_window_offset = rand_range_f(.1337f, od_window - od_window_left_offset);
                else
                    jumping_window_offset = -rand_range_f(.1337f, od_window_right_offset);
                hit_objects_passed = current_beatmap.hit_object_idx;
                wait_hitojects_count = rand_range_i(wait_hitobjects_min, wait_hitobjects_max);
            }
            od_check_ms += jumping_window_offset;
        }
    }
}

void update_relax(Circle &circle, const int32_t audio_time)
{
    static double keydown_time = 0.0;
    static double keyup_delay = 0.0;
    static bool was_approaching = false;

    if (!cfg_relax_lock || !current_beatmap.ready)
        return;

    calc_od_timing();

    auto current_time = audio_time + od_check_ms;
    auto valid_timing = current_time >= circle.start_time;
    
    // Check if cursor is on circle
    bool cursor_on_circle = is_cursor_on_circle(circle.position, current_beatmap.scaled_hit_object_radius);
    bool approaching_circle = is_approaching_circle(circle.position, current_beatmap.scaled_hit_object_radius);
    
    // Calculate time until miss
    float time_until_miss = circle.end_time - current_time;
    bool about_to_miss = time_until_miss < 20.0f; // Reduced from 50ms to 20ms for more precision
    
    // Determine if we should click based on conditions
    bool should_click = false;
    
    // If we're approaching or on the circle, we should click
    if (approaching_circle || cursor_on_circle) {
        // Add very small randomness to simulate human reaction time (reduced to 5ms)
        float random_delay = (static_cast<float>(rand() % 100)) / 100.0f * 5.0f; // Random delay up to 5ms
        should_click = current_time >= (circle.start_time - random_delay);
    }
    
    if (should_click && !circle.clicked)
    {
        // Use random number to determine if we should alternate based on percentage
        float rand_val = (float)rand() / RAND_MAX * 100.0f;
        if (rand_val < cfg_relax_alternate_percentage)
        {
            current_click = current_click == left_click[0] ? right_click[0] : left_click[0];
        }

        send_keyboard_input(current_click, 0);
        FR_INFO("Relax hit %d!, %d %d", current_beatmap.hit_object_idx, circle.start_time, circle.end_time);
        
        // Very small random keyup delay for human-like behavior (reduced to 3ms)
        float random_keyup = (static_cast<float>(rand() % 100)) / 100.0f * 3.0f; // Random delay up to 3ms
        keyup_delay = circle.end_time ? (circle.end_time - circle.start_time + random_keyup) : 0.5;

        if (cfg_timewarp_enabled)
        {
            double timewarp_playback_rate_div_100 = cfg_timewarp_playback_rate / 100.0;
            keyup_delay /= timewarp_playback_rate_div_100;
        }
        else if (circle.type == HitObjectType::Slider || circle.type == HitObjectType::Spinner)
        {
            if (current_beatmap.mods & Mods::DoubleTime)
                keyup_delay /= 1.5;
            else if (current_beatmap.mods & Mods::HalfTime)
                keyup_delay /= 0.75;
        }
        keydown_time = ImGui::GetTime();
        circle.clicked = true;
        od_check_ms = .0f;
    }
    // If we were approaching but now we're not, reset the circle's clicked state
    else if (was_approaching && !approaching_circle)
    {
        circle.clicked = false;
    }
    
    was_approaching = approaching_circle;

    if (cfg_relax_lock && keydown_time && ((ImGui::GetTime() - keydown_time) * 1000.0 > keyup_delay))
    {
        keydown_time = 0.0;
        send_keyboard_input(current_click, KEYEVENTF_KEYUP);
    }
}

void relax_on_beatmap_load()
{
    if (cfg_relax_lock)
    {
        current_click = cfg_relax_style == 'a' ? right_click[0] : left_click[0];
    }
}
