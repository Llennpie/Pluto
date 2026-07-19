#ifndef SaturnKeyframe
#define SaturnKeyframe

#include <string>
#include <map>
#include <vector>
#include <functional>

#include <string.h>
#include <stdlib.h>
#include "include/types.h"

using Interpolate = std::function<void(void*, void*, void*, float)>;
using Compare = std::function<bool(void*, void*)>;
using OnHover = std::function<void(void*)>;

enum class InterpolationType {
    In, Out, InOut, Unknown
};

struct Keyframe {
private:
    void* data = nullptr;
    size_t data_size = 0;

public:
    int position = 0;
    float interp_power = 1;
    InterpolationType interp_type = InterpolationType::InOut;
    bool pause_playback = false;

    Keyframe() {}
    ~Keyframe() { free(data); }

    Keyframe(Keyframe&& other) {
        memcpy((void*)this, &other, sizeof(Keyframe));
        other.data = nullptr;
        other.data_size = 0;
    }

    Keyframe(const Keyframe& other) {
        memcpy((void*)this, &other, sizeof(Keyframe));
        void* new_data = calloc(data_size, 1);
        memcpy(new_data, data, data_size);
        data = new_data;
    }

    Keyframe& operator=(Keyframe&& other) {
        if (this == &other) return *this;

        free(data);
        memcpy((void*)this, &other, sizeof(Keyframe));
        other.data = nullptr;
        other.data_size = 0;

        return *this;
    }

    Keyframe& operator=(const Keyframe& other) {
        if (this == &other) return *this;

        free(data);
        memcpy((void*)this, &other, sizeof(Keyframe));
        void* new_data = calloc(data_size, 1);
        memcpy(new_data, data, data_size);
        data = new_data;

        return *this;
    }

    template<typename T> T* get() {
        return (T*)get(sizeof(T));
    }

    void* get(size_t size) {
        if (!data) {
            data_size = size;
            data = calloc(size, 1);
        }
        if (size > data_size) {
            data_size = size;
            data = realloc(data, size);
        }
        return data;
    }
};

struct Timeline {
    void* ptr;
    size_t size;
    bool lock_interpolation;
    bool require_active; // if true, only create NEW keyframes when an ImGui item is active
    Interpolate interpolate;
    Compare compare;
    OnHover on_hover;

    std::vector<Keyframe> keyframes;
};

extern std::map<std::string, Timeline> timelines;
extern int timeline_position;
extern bool timeline_is_playing;
extern bool timeline_loop;
extern int timeline_end;

bool TimelineButton(std::string name, Timeline timeline);

template<typename T> bool TimelineButton(std::string name, T* ptr, std::function<void(T*, T*, T*, float)> interpolate, std::function<void(T*)> on_hover = nullptr, bool lock_interpolation = false) {
    return TimelineButton(name, (Timeline){
        (void*)ptr, sizeof(T), lock_interpolation, false,
        [=](void* out, void* a, void* b, float x) { return interpolate((T*)out, (T*)a, (T*)b, x); },
        [=](void* a, void* b) { return *(T*)a == *(T*)b; },
        [=](void* value) { on_hover((T*)value); }
    });
}

bool TimelineButton(std::string name, float* ptr, bool lock_interpolation = false);
bool TimelineButton(std::string name, int* ptr, bool lock_interpolation = false);
bool TimelineButton(std::string name, bool* ptr);
bool TimelineButton(std::string name, Vec3f* ptr);
bool TimelineButton(std::string name, char* ptr, size_t maxLen);

void PlayTimeline();
void RenderTimelineWidget();
void UpdateTimelines();

#endif