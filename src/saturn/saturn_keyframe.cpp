#include "saturn_keyframe.h"

#include "ui/saturn_imgui.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_internal.h"
#include "libs/imgui/imgui_neo_sequencer.h"

#include <algorithm>
#include <unordered_set>
#include <math.h>
#include "types.h"

std::map<std::string, Timeline> timelines = {};
int timeline_position;
bool timeline_is_playing;
bool timeline_loop = false;
int timeline_end = 60;
static int prev_position;

bool TimelineButton(std::string name, float* ptr, bool lock_interpolation) {
    return TimelineButton<float>(name, ptr,
        [](float* out, float* a, float* b, float x) { *out = (*b - *a) * x + *a; },
        [](float* value) { ImGui::BeginTooltip(); ImGui::Text("%.3f", *value); ImGui::EndTooltip(); },
        lock_interpolation
    );
}

bool TimelineButton(std::string name, int* ptr, bool lock_interpolation) {
    return TimelineButton<int>(name, ptr,
        [](int* out, int* a, int* b, float x) { *out = (*b - *a) * x + *a; },
        [](int* value) { ImGui::BeginTooltip(); ImGui::Text("%d", *value); ImGui::EndTooltip(); },
        lock_interpolation
    );
}

bool TimelineButton(std::string name, bool* ptr) {
    return TimelineButton<bool>(name, ptr,
        [](bool* out, bool* a, bool* b, float x) { *out = x < 1 ? *a : *b; },
        [](bool* value) { ImGui::BeginTooltip(); ImGui::Checkbox("##preview_checkbox", value); ImGui::EndTooltip(); },
        true
    );
}

bool TimelineButton(std::string name, char* ptr, size_t maxLen) {
    return TimelineButton(name, (Timeline){
        (void*)ptr, maxLen, true, false,
        // Snap: hold value A until the keyframe is passed, then switch to B
        [maxLen](void* out, void* a, void* b, float x) {
            memcpy(out, x < 1.0f ? a : b, maxLen);
        },
        [maxLen](void* a, void* b) { return strncmp((char*)a, (char*)b, maxLen) == 0; },
        [](void* value) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted((char*)value);
            ImGui::EndTooltip();
        }
    });
}

bool TimelineButton(std::string name, Vec3f* ptr) {
    return TimelineButton(name, (Timeline){
        (void*)ptr, sizeof(Vec3f), false, true,
        [](void* out, void* a, void* b, float x) {
            float* o = (float*)out; float* fa = (float*)a; float* fb = (float*)b;
            o[0] = (fb[0] - fa[0]) * x + fa[0];
            o[1] = (fb[1] - fa[1]) * x + fa[1];
            o[2] = (fb[2] - fa[2]) * x + fa[2];
        },
        [](void* a, void* b) { return memcmp(a, b, sizeof(Vec3f)) == 0; },
        [](void* value) {
            float* v = (float*)value;
            ImGui::BeginTooltip(); ImGui::Text("(%.3f, %.3f, %.3f)", v[0], v[1], v[2]); ImGui::EndTooltip();
        }
    });
}

bool TimelineButton(std::string name, Timeline timeline) {
    auto it = timelines.find(name);
    bool automated = it != timelines.end();
    std::string btn_label = (automated ? "X##tl_" : "~##tl_") + name;
    if (automated) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0x56/255.f, 0xFF/255.f, 0xBB/255.f, 1.0f));
    bool clicked = ImGui::Button(btn_label.c_str());
    if (automated) ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (!automated) ImGui::Text("Automate");
        else ImGui::Text("Remove Timeline");
        ImGui::EndTooltip();
    }
    
    if (!clicked) return false;

    if (!automated) {
        show_window_timeline = true;
        // add a starting keyframe
        Keyframe start = Keyframe();
        if (timeline.lock_interpolation) start.interp_power = 0;
        memcpy(start.get(timeline.size), timeline.ptr, timeline.size); // use the current value
        timeline.keyframes.push_back(start);
        timelines.insert({ name, timeline });
    }
    else timelines.erase(it);

    return true;
}

static float PowerTransform(float x, float power, InterpolationType type) {
    if (power == 0) return 0;
    switch (type) {
        case InterpolationType::In: x = powf(x, power); break;
        case InterpolationType::Out: x = 1 - powf(1 - x, power); break;
        case InterpolationType::InOut: x = x < 0.5
            ? powf(2, power - 1) * powf(x, power)
            : 1 - powf(-2 * x + 2, power) / 2; break;
        case InterpolationType::Unknown: assert(false);
    }
    return x;
}

static Keyframe* RunTimeline(Timeline& timeline, void* out) {
    // find the keyframes between the current timeline position
    Keyframe* curr_kf = &timeline.keyframes[0];
    Keyframe* next_kf = &timeline.keyframes[0];
    for (Keyframe& kf : timeline.keyframes) {
        next_kf = &kf;
        if (kf.position > timeline_position) break;
        curr_kf = &kf;
    }

    // get linear interpolation position
    float pos = next_kf->position == curr_kf->position
        ? 0
        : PowerTransform(
            (timeline_position - curr_kf->position) / (float)(next_kf->position - curr_kf->position),
            curr_kf->interp_power, curr_kf->interp_type
        );

    // interpolate and apply
    void* a = curr_kf->get(timeline.size);
    void* b = next_kf->get(timeline.size);
    timeline.interpolate(out, a, b, pos);

    return curr_kf;
}

static void SortKeyframes(std::vector<Keyframe>& keyframes) {
    std::sort(keyframes.begin(), keyframes.end(), [](Keyframe& a, Keyframe& b) {
        return a.position < b.position; 
    });
}

void UpdateTimelines() {
    for (auto& tl_pair : timelines) {
        Timeline& timeline = tl_pair.second;

        char out[timeline.size];
        Keyframe* kf = RunTimeline(timeline, out);

        if (prev_position != timeline_position)
            // playhead moved; apply interpolated value
            memcpy(timeline.ptr, out, timeline.size);

        else if (!timeline.compare(timeline.ptr, out)) {
            // value is different than what we expect
            if (kf->position == timeline_position)
                // a keyframe exists at the current timeline position; overwrite its value
                memcpy(kf->get(timeline.size), timeline.ptr, timeline.size);
            else if (!timeline.require_active || ImGui::IsAnyItemActive()) {
                // create a new keyframe with the value
                timeline.keyframes.push_back(Keyframe());
                kf = &timeline.keyframes.back();
                kf->position = timeline_position;
                if (timeline.lock_interpolation) kf->interp_power = 0;
                memcpy(kf->get(timeline.size), timeline.ptr, timeline.size);

                SortKeyframes(timeline.keyframes);
            }
        }
    }

    prev_position = timeline_position;
    static float playback_accum = 0.f;
    if (timeline_is_playing) {
        // playback at 30 fps, accounts for varying fps
        playback_accum += ImGui::GetIO().DeltaTime;
        bool pause_key = false;
        while (playback_accum >= 1.f / 30.f && !pause_key) {
            playback_accum -= 1.f / 30.f;
            timeline_position++;
            if (timeline_position > timeline_end) {
                if (timeline_loop) timeline_position = 0;
                else { timeline_is_playing = false; timeline_position = timeline_end; playback_accum = 0.f; break; }
            }
            for (auto& tl : timelines) {
                for (auto& kf : tl.second.keyframes) {
                    if (kf.position == timeline_position && kf.pause_playback) {
                        pause_key = true;
                        break;
                    }
                }
                if (pause_key) break;
            }
        }
        if (pause_key) { timeline_is_playing = false; playback_accum = 0.f; }
    } else {
        playback_accum = 0.f;
    }
}

void PlayTimeline() {
    timeline_is_playing = !timeline_is_playing;
    if (timeline_is_playing && timeline_position >= timeline_end) timeline_position = 0;
}

void RenderTimelineWidget() {
    static int start = 0;
    static std::vector<int> selected_keyframe_positions;
    static std::string selected_timeline_name;
    bool open_popup = false;

    if (ImGui::BeginPopup("##keyframe_menu")) {
        auto tl_it = timelines.find(selected_timeline_name);
        if (tl_it == timelines.end()) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }
        Timeline& sel_tl = tl_it->second;
        std::vector<Keyframe*> selected_keyframes;
        for (int pos : selected_keyframe_positions) {
            auto it = std::find_if(sel_tl.keyframes.begin(), sel_tl.keyframes.end(),
                [pos](const Keyframe& k) { return k.position == pos; });
            if (it != sel_tl.keyframes.end())
                selected_keyframes.push_back(&*it);
        }
        if (selected_keyframes.empty()) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }

        // find common interpolation power and type
        bool unk_power = false;
        float* power = &selected_keyframes[0]->interp_power;
        InterpolationType interp = selected_keyframes[0]->interp_type;
        for (Keyframe* kf : selected_keyframes) {
            if (kf->interp_power != *power) unk_power = true;
            if (kf->interp_type != interp) interp = InterpolationType::Unknown;
        }

        bool modified = false;
        auto modify = [&](std::function<void(Keyframe*)> func) {
            modified = true;
            for (Keyframe* kf : selected_keyframes)
                func(kf);
        };

        if (ImGui::MenuItem("Delete")) {
            for (Keyframe* kf : selected_keyframes)
                kf->position = -1;
            sel_tl.keyframes.erase(std::remove_if(
                sel_tl.keyframes.begin(), sel_tl.keyframes.end(),
                [](Keyframe& x) { return x.position == -1; }
            ), sel_tl.keyframes.end());
            prev_position = -1;
        }
        ImGui::MenuItem("Close");
        ImGui::Checkbox("Pause Playback", &selected_keyframes[0]->pause_playback);
        ImGui::SeparatorText("Interpolation");
        ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
        ImGui::BeginDisabled(sel_tl.lock_interpolation);
        if (ImGui::MenuItem("In", nullptr, interp == InterpolationType::In)) modify([&](Keyframe* kf) { interp = kf->interp_type = InterpolationType::In; });
        if (ImGui::MenuItem("Out", nullptr, interp == InterpolationType::Out)) modify([&](Keyframe* kf) { interp = kf->interp_type = InterpolationType::Out; });
        if (ImGui::MenuItem("InOut", nullptr, interp == InterpolationType::InOut)) modify([&](Keyframe* kf) { interp = kf->interp_type = InterpolationType::InOut; });
        ImGui::EndDisabled();
        ImGui::PopItemFlag();
        ImGui::Separator();
        ImGui::BeginDisabled(sel_tl.lock_interpolation);
        if (ImGui::SliderFloat("Power", power, 0, 4,
            sel_tl.lock_interpolation
                ? "Interpolation Disabled"
                : unk_power ? "???" : "%.3f"
        )) modify([&](Keyframe* kf) { unk_power = false; kf->interp_power = *power; });
        ImGui::EndDisabled();

        // draw the interpolation graph
        float values[101];
        for (int i = 0; i <= 100; i++) {
            values[i] = unk_power || interp == InterpolationType::Unknown ? 0 : PowerTransform(i / 100.f, *power, interp);
        }
        ImGui::PlotLines("##keyframe_graph", values, 101, 0, nullptr, 0, 1, { 0, 100 });

        // if modified, set prev_position to -1 so that the value gets updated rather than a new keyframe being created
        if (modified) prev_position = -1;

        ImGui::EndPopup();
    }
    
    if (ImGui::Button(timeline_is_playing ? "||" : ">"))
        PlayTimeline();

    ImGui::SameLine();
    if (ImGui::Button("|<")) { timeline_is_playing = false; timeline_position = 0; }
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &timeline_loop);
    ImGui::SameLine();
    if (ImGui::Button(">|")) { timeline_is_playing = false; timeline_position = timeline_end; }
    ImGui::SameLine(); ImGui::Dummy({ 5, 0 }); ImGui::SameLine();
    ImGui::TextDisabled("frame %d", timeline_position);

    if (ImGui::BeginNeoSequencer("Sequencer", &timeline_position, &start, &timeline_end, ImGui::GetContentRegionAvail(),
        ImGuiNeoSequencerFlags_EnableSelection |
        ImGuiNeoSequencerFlags_AllowLengthChanging |
        ImGuiNeoSequencerFlags_Selection_EnableDragging |
        ImGuiNeoSequencerFlags_Selection_EnableDeletion |
        (timeline_is_playing ? ImGuiNeoSequencerFlags_AutoScrollToFrame : 0),
        60
    )) {
        for (auto& tl_pair : timelines) {
            if (!ImGui::BeginNeoTimelineEx(tl_pair.first.c_str())) continue;

            Timeline& timeline = tl_pair.second;
            bool updated = false;
            // right click keyframe (so keyframes don't have to be selected before right clicking them)
            std::vector<int> cur_selected_in_tl;
            int right_clicked_kf_pos = -1;
            bool right_clicked_was_selected = false;
            for (auto& kf : timeline.keyframes) {
                int prev_kfpos = kf.position;
                ImGui::NeoKeyframe(&kf.position);

                // keyframe move checks
                if (prev_kfpos == 0) kf.position = 0; // make the initial keyframe not be able to be moved
                else if (kf.position < 1) kf.position = 1; // a keyframe position cannot be <1 besides the initial one
                if (prev_kfpos != kf.position) updated = true;

                bool kf_selected = ImGui::IsNeoKeyframeSelected();
                if (kf_selected)
                    cur_selected_in_tl.push_back(kf.position);
                if (ImGui::IsNeoKeyframeRightClicked()) {
                    right_clicked_kf_pos = kf.position;
                    right_clicked_was_selected = kf_selected;
                }
                if (ImGui::IsNeoKeyframeHovered()) {
                    if (timeline.on_hover) timeline.on_hover(kf.get(timeline.size));

                    if (ImGui::IsMouseDoubleClicked(0)) {
                        timeline_position = kf.position;
                        updated = true;
                    }
                }
            }
            if (right_clicked_kf_pos >= 0) {
                selected_timeline_name = tl_pair.first;
                selected_keyframe_positions.clear();
                selected_keyframe_positions.push_back(right_clicked_kf_pos);

                if (right_clicked_was_selected) {
                    for (int pos : cur_selected_in_tl)
                        if (pos != right_clicked_kf_pos)
                            selected_keyframe_positions.push_back(pos);
                }
                open_popup = true;
            }

            if (updated) {
                // update value and sort keyframes
                prev_position = -1;
                SortKeyframes(timeline.keyframes);
            }
            
            ImGui::EndNeoTimeLine();
        }
        ImGui::EndNeoSequencer();
    }

    if (open_popup) ImGui::OpenPopup("##keyframe_menu");
}
