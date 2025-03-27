#include "legacy/features/aimbot.h"
#include <random>
#include <cmath>
#include <algorithm>
#include "ui/config.h"

// Random number generator for humanization
static std::mt19937 rng(std::random_device{}());

// Enhanced smoothing parameters
struct SmoothingParameters {
    float approach_speed;     // Movement speed coefficient (lower = more human-like)
    float max_distance;       // Maximum distance to apply targeting
    float deviation_amount;   // How much to randomly deviate from perfect aim
    float jitter_amount;      // Subtle mouse jitter amount
    float acceleration;       // How quickly to reach target speed
    float deceleration;       // How quickly to slow down near target
    float overshoot_chance;   // Chance to overshoot the target
    float overshoot_amount;   // How much to overshoot by
    bool enable_assist;       // Only assist, don't take full control
};

// Global variables
float elapsed_lerp = 0;
static SmoothingParameters smooth_params;
static Vector2<float> current_pos;
static Vector2<float> velocity;
static Vector2<float> target_pos;
static bool target_active = false;
static float current_speed = 0.0f;
static float target_speed = 0.0f;

// Vector operations
static float vector_length(const Vector2<float>& v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

static Vector2<float> vector_normalize(const Vector2<float>& v) {
    float length = vector_length(v);
    if (length < 0.001f) return {0.0f, 0.0f};
    return {v.x / length, v.y / length};
}

static Vector2<float> vector_add(const Vector2<float>& a, const Vector2<float>& b) {
    return {a.x + b.x, a.y + b.y};
}

static Vector2<float> vector_sub(const Vector2<float>& a, const Vector2<float>& b) {
    return {a.x - b.x, a.y - b.y};
}

static Vector2<float> vector_scale(const Vector2<float>& v, float scale) {
    return {v.x * scale, v.y * scale};
}

static float vector_dot(const Vector2<float>& a, const Vector2<float>& b) {
    return a.x * b.x + a.y * b.y;
}

static Vector2<float> vector_perpendicular(const Vector2<float>& v) {
    return {-v.y, v.x};
}

// Calculate a point on a quadratic Bezier curve
static Vector2<float> quadratic_bezier(const Vector2<float>& p0, const Vector2<float>& p1, const Vector2<float>& p2, float t) {
    float one_minus_t = 1.0f - t;
    return vector_add(
        vector_add(
            vector_scale(p0, one_minus_t * one_minus_t),
            vector_scale(p1, 2.0f * one_minus_t * t)
        ),
        vector_scale(p2, t * t)
    );
}

// Calculate optimal control point for smooth movement
static Vector2<float> calculate_control_point(const Vector2<float>& start, const Vector2<float>& end, float distance) {
    Vector2<float> direction = vector_sub(end, start);
    float dir_length = vector_length(direction);
    if (dir_length < 0.001f) return start;
    
    direction = vector_normalize(direction);
    Vector2<float> perp = vector_perpendicular(direction);
    
    // Calculate control point offset based on distance
    float offset = distance * 0.5f;
    
    // Add some randomness to the control point
    float random_offset = (static_cast<float>(rng() % 100) - 50.0f) / 100.0f * offset;
    
    // Calculate mid point
    Vector2<float> mid_point = vector_scale(vector_add(start, end), 0.5f);
    
    // Add perpendicular offset with randomness
    Vector2<float> offset_vector = vector_scale(perp, offset + random_offset);
    return vector_add(mid_point, offset_vector);
}

// Initialize smoothing parameters
void init_smoothing_parameters() {
    smooth_params.approach_speed = cfg_humanize_acceleration;
    smooth_params.max_distance = cfg_humanize_overshoot_amount * 500.0f;
    smooth_params.deviation_amount = cfg_humanize_path_deviation * 10.0f;
    smooth_params.jitter_amount = cfg_humanize_jitter;
    smooth_params.acceleration = cfg_humanize_acceleration;
    smooth_params.deceleration = cfg_humanize_deceleration;
    smooth_params.overshoot_chance = cfg_humanize_overshoot_chance;
    smooth_params.overshoot_amount = cfg_humanize_overshoot_amount;
    smooth_params.enable_assist = (cfg_humanize_movement_type != 0);
}

// Enhanced smoothed movement with Bezier curves and velocity control
static Vector2<float> calculate_smoothed_position(const Vector2<float>& start, const Vector2<float>& end, float delta_time) {
    Vector2<float> direction = vector_sub(end, start);
    float distance = vector_length(direction);
    
    if (distance < 1.0f) {
        current_speed = 0.0f;
        return start;
    }
    
    // Calculate optimal speed based on distance
    target_speed = smooth_params.approach_speed * distance * delta_time * 3.0f;
    
    // Apply acceleration/deceleration
    if (distance < 50.0f) {
        // Decelerate near target
        current_speed = (std::max)(0.0f, current_speed - smooth_params.deceleration * delta_time);
    } else {
        // Accelerate when far
        current_speed = (std::min)(target_speed, current_speed + smooth_params.acceleration * delta_time);
    }
    
    // Calculate control point for Bezier curve
    Vector2<float> control_point = calculate_control_point(start, end, distance);
    
    // Calculate progress along the curve
    float progress = current_speed / target_speed;
    
    // Add slight randomness to progress for more natural movement
    float random_progress = (static_cast<float>(rng() % 100) - 50.0f) / 100.0f * 0.1f;
    progress = (std::max)(0.0f, (std::min)(1.0f, progress + random_progress));
    
    // Calculate position on Bezier curve
    Vector2<float> new_pos = quadratic_bezier(start, control_point, end, progress);
    
    // Add subtle jitter
    float jitter_scale = smooth_params.jitter_amount * (1.0f - progress) * 0.5f;
    Vector2<float> jitter = {
        (static_cast<float>(rng() % 200) - 100.0f) / 100.0f * jitter_scale,
        (static_cast<float>(rng() % 200) - 100.0f) / 100.0f * jitter_scale
    };
    new_pos = vector_add(new_pos, jitter);
    
    return new_pos;
}

// Main function to handle aimbot movement
static void move_mouse_with_assist(const Vector2<float>& target, const Vector2<float>& cursor_pos, float delta_time) {
    // Add randomization within the circle's radius (assuming 32px radius)
    float circle_radius = 32.0f;
    float random_angle = (static_cast<float>(rng() % 360)) * 3.14159f / 180.0f;
    float random_distance = (static_cast<float>(rng() % 97)) / 100.0f * circle_radius; // Use 97% of radius
    
    Vector2<float> randomized_target = {
        target.x + random_distance * cosf(random_angle),
        target.y + random_distance * sinf(random_angle)
    };
    
    Vector2<float> target_on_screen = playfield_to_screen(randomized_target);
    
    // Calculate distance to target
    Vector2<float> diff = vector_sub(target_on_screen, cursor_pos);
    float distance = vector_length(diff);
    
    // If target is too far, don't assist
    if (distance > smooth_params.max_distance) {
        target_active = false;
        current_speed = 0.0f;
        return;
    }
    
    // Calculate new position with enhanced smoothing
    Vector2<float> new_pos = calculate_smoothed_position(cursor_pos, target_on_screen, delta_time);
    
    // If in assist mode, blend user's cursor with calculated position
    if (smooth_params.enable_assist) {
        // The closer to the target, the stronger the assistance
        float assist_factor = 1.0f - (distance / smooth_params.max_distance);
        assist_factor = assist_factor * assist_factor; // Square for more natural falloff
        
        // Blend current position with calculated position
        Vector2<float> movement = vector_sub(new_pos, cursor_pos);
        movement = vector_scale(movement, assist_factor);
        new_pos = vector_add(cursor_pos, movement);
    }
    
    // Move mouse to new position
    move_mouse_to(new_pos.x, new_pos.y);
}

// Main aimbot function
void update_aimbot(Circle &circle, const int32_t audio_time)
{
    if (!cfg_aimbot_lock || !current_beatmap.ready)
        return;

    static float last_time = 0.0f;
    float current_time = ImGui::GetTime();
    float delta_time = current_time - last_time;
    last_time = current_time;

    // Get current mouse position
    Vector2<float> cursor_pos = mouse_position();

    // Handle different hit object types
    Vector2<float> target_pos;
    if (circle.type == HitObjectType::Circle) {
        target_pos = circle.position;
    }
    else if (circle.type == HitObjectType::Slider) {
        // Get slider ball position
        uintptr_t osu_manager = *(uintptr_t *)(osu_manager_ptr);
        if (!osu_manager) { FR_ERROR("Aimbot: osu_manager"); return; }
        uintptr_t hit_manager_ptr = *(uintptr_t *)(osu_manager + OSU_MANAGER_HIT_MANAGER_OFFSET);
        if (!hit_manager_ptr) { FR_ERROR("Aimbot: hit_manager_ptr"); return; }
        uintptr_t hit_objects_list_ptr = *(uintptr_t *)(hit_manager_ptr + OSU_HIT_MANAGER_HIT_OBJECTS_LIST_OFFSET);
        if (!hit_objects_list_ptr) { FR_ERROR("Aimbot: hit_objects_list_ptr"); return; }
        uintptr_t hit_objects_list_items_ptr = *(uintptr_t *)(hit_objects_list_ptr + 0x4);
        if (!hit_objects_list_items_ptr) { FR_ERROR("Aimbot: hit_objects_list_items_ptr"); return; }
        uintptr_t hit_object_ptr = *(uintptr_t *)(hit_objects_list_items_ptr + 0x8 + 0x4 * current_beatmap.hit_object_idx);
        if (!hit_object_ptr) { FR_ERROR("Aimbot: hit_object_ptr"); return; }
        uintptr_t animation_ptr = *(uintptr_t *)(hit_object_ptr + OSU_HIT_OBJECT_ANIMATION_OFFSET);
        if (!animation_ptr) { FR_ERROR("Aimbot: animation_ptr"); return; }
        float slider_ball_x = *(float *)(animation_ptr + OSU_ANIMATION_SLIDER_BALL_X_OFFSET);
        float slider_ball_y = *(float *)(animation_ptr + OSU_ANIMATION_SLIDER_BALL_Y_OFFSET);
        target_pos = Vector2<float>(slider_ball_x, slider_ball_y);
    }
    else {
        return; // Skip spinners
    }

    // Check if target is on screen
    Vector2<float> screen_pos = playfield_to_screen(target_pos);
    if (screen_pos.x < 0 || screen_pos.x > window_size.x || 
        screen_pos.y < 0 || screen_pos.y > window_size.y) {
        target_active = false;
        return;
    }

    // If we're not already targeting this object, initialize targeting
    if (!target_active) {
        current_pos = cursor_pos;
        velocity = {0.0f, 0.0f};
        current_speed = 0.0f;
        target_active = true;
    }

    // Move mouse to target with assist
    move_mouse_with_assist(target_pos, cursor_pos, delta_time);
}

void aimbot_on_beatmap_load() {
    if (cfg_aimbot_lock) {
        init_smoothing_parameters();
        target_active = false;
    }
}

void aimbot_on_advance_hit_object() {
    target_active = false;
}

void update_aimbot()
{
    // if (cfg_timewarp_enabled)
    // {
    //     double timewarp_playback_rate = cfg_timewarp_playback_rate;
    //     // ... rest of timewarp code ...
    // }
}
