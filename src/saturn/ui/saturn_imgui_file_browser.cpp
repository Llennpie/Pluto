#include "saturn_imgui_file_browser.h"

#include <string>
#include <filesystem>
#include <functional>
#include <map>
#include <vector>
#include <algorithm>
#include "saturn/libs/imgui/imgui.h"
#include "saturn/ui/saturn_imgui_models.h"
#include "saturn/ui/saturn_imgui_colors.h"
#include "saturn/ui/saturn_imgui.h"
#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_animations.h"
#include "saturn/saturn_keyframe.h"

class FileBrowserEntry {
private:
    std::string _name;
    bool _dir = false;
    std::vector<FileBrowserEntry> entries = {};
    FileBrowserEntry* _parent;
public:
    FileBrowserEntry(std::string name, bool dir, FileBrowserEntry* parent) {
        _name = name;
        _dir = dir;
        _parent = parent;
    }
    bool is_dir() const { return _dir; }
    const std::vector<FileBrowserEntry>& dir() const { return entries; }
    FileBrowserEntry* parent() { return _parent; }
    const std::string& name() const { return _name; }
    void clear() { entries.clear(); }
    void add_file(const std::string& name) { entries.push_back(FileBrowserEntry(name, false, this)); }
    FileBrowserEntry* add_dir(const std::string& name) {
        entries.push_back(FileBrowserEntry(name, true, this));
        return &entries[entries.size() - 1];
    }
    void merge(FileBrowserEntry* entry) {
        for (FileBrowserEntry& e : entry->entries) {
            entries.push_back(e);
        }
    }
};

FileBrowserEntry root = FileBrowserEntry("root", true, nullptr);
FileBrowserEntry* curr = &root;
int browser_height = 150;
std::filesystem::path selected_path;
std::vector<std::string> extension_filters = {};
std::map<std::string, char*> search_terms = {};
std::map<std::string, std::string> selected_paths = {};
std::map<std::filesystem::path, FileBrowserEntry*> scanned_paths = {};
std::filesystem::path last_scanned_path;
std::function<void(std::string)> drag_callback = nullptr;
std::map<std::string, std::vector<std::string>> browser_flat_lists = {};
std::map<std::string, int> browser_timeline_index = {};

static void flatten_browser_entries(const FileBrowserEntry& dir, const std::string& path, std::vector<std::string>& out) {
    for (const FileBrowserEntry& entry : dir.dir()) {
        if (entry.is_dir()) flatten_browser_entries(entry, path + entry.name() + "/", out);
        else out.push_back(path + entry.name());
    }
}

void saturn_file_browser_item(std::string item) {
    curr->add_file(item);
}

void saturn_file_browser_tree_node(std::string item) {
    curr = curr->add_dir(item);
}

void saturn_file_browser_tree_node_end() {
    curr = curr->parent();
}

void saturn_file_browser_scan_directory_internal(std::filesystem::path dir, bool recursive, FileBrowserEntry* currfolder) {
    std::vector<std::string> dirs = {};
    std::vector<std::string> files = {};
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::filesystem::path path = entry.path();
        if (std::filesystem::is_directory(path)) {
            if (!recursive) continue;
            dirs.push_back(path.filename().string());
            continue;
        }
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (!extension_filters.empty()) {
            bool matched = false;
            for (const auto& f : extension_filters) if (ext == "." + f) { matched = true; break; }
            if (!matched) continue;
        }
        files.push_back(path.filename().string());
    }
    auto stringcomp = [](std::string a, std::string b) {
        return a < b;
    };
    std::sort(dirs.begin(), dirs.end(), stringcomp);
    std::sort(files.begin(), files.end(), stringcomp);
    for (std::string dirname : dirs) {
        FileBrowserEntry* folder = currfolder->add_dir(dirname);
        saturn_file_browser_scan_directory_internal(dir / dirname, true, folder);
    }
    for (std::string filename : files) {
        currfolder->add_file(filename);
    }
}

void saturn_file_browser_scan_directory(std::filesystem::path dir, bool recursive) {
    last_scanned_path = dir;
    FileBrowserEntry* entry;
    if (scanned_paths.find(dir) != scanned_paths.end()) entry = scanned_paths[dir];
    else {
        entry = new FileBrowserEntry("root", true, nullptr);
        saturn_file_browser_scan_directory_internal(dir, recursive, entry);
        scanned_paths.insert({ dir, entry });
    }
    curr->merge(entry);
}

void saturn_file_browser_rescan_directory(std::filesystem::path dir, bool recursive) {
    if (scanned_paths.find(dir) != scanned_paths.end()) {
        delete scanned_paths[dir];
        scanned_paths.erase(dir);
    }
    saturn_file_browser_scan_directory(dir, recursive);
}

void saturn_file_browser_filter_extension(std::string extension) {
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    extension_filters.push_back(extension);
}

void saturn_file_browser_height(int height) {
    browser_height = height;
}

void saturn_file_browser_set_drag_callback(std::function<void(std::string)> callback) {
    drag_callback = callback;
}

void saturn_file_browser_clear() {
    root.clear();
    browser_height = 150;
    extension_filters.clear();
    drag_callback = nullptr;
}

std::map<std::string, bool> was_searching = {};
static bool saturn_file_browser_create_imgui_impl(
    const FileBrowserEntry& dir,
    const std::string& path,
    const std::string& browser_id,
    bool do_search,
    int exp_index,
    const char* search_term,
    size_t search_len,
    std::string& selected_ref,
    bool& was_searching_ref)
{
    bool clicked = false;
    for (const FileBrowserEntry& entry : dir.dir()) {
        if (entry.name() == "eyes") continue;
        if (entry.is_dir()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(search_len > 0 ? ImGuiCol_TextDisabled : ImGuiCol_Text));

            if (search_len > 0) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                was_searching_ref = true;
            } else if (search_len == 0 && was_searching_ref) {
                ImGui::SetNextItemOpen(false, ImGuiCond_Always);
            }

            if (ImGui::TreeNode(entry.name().c_str())) {
                ImGui::PopStyleColor();
                std::string sub_path = path + entry.name() + "/";
                clicked |= saturn_file_browser_create_imgui_impl(entry, sub_path, browser_id, do_search, exp_index, search_term, search_len, selected_ref, was_searching_ref);
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(search_len > 0 ? ImGuiCol_TextDisabled : ImGuiCol_Text));
                ImGui::TreePop();
            }
            ImGui::PopStyleColor();
        } else {
            if (do_search && search_len > 0) {
                std::string filename = entry.name();
                std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                if (filename.find(search_term) == std::string::npos) continue;
            }
            const std::string& entryName = entry.name();
            std::string fullpath = path + entryName;

            if (exp_index >= 0) {
                // Expression selector
                Expression* expression = &current_expressions[exp_index];
                if (expression->Textures.size() <= 0) continue;

                for (TexturePath& texture : expression->Textures) {
                    if (texture.SmallExpressionPath(expression->Name) == fullpath) {
                        int texIdx = &texture - &expression->Textures[0];
                        bool selected = (expression->CurrentIndex == texIdx) ||
                                        (expression->BlinkIndex[0] == texIdx) ||
                                        (expression->BlinkIndex[1] == texIdx);

                        if (ImGui::Selectable(entryName.c_str(), &selected)) {
                            selected_path = selected_ref = fullpath;
                            clicked = true;
                        }
                        if (ImGui::IsItemHovered()) {
                            OpenExpressionPreview(&texture);
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                current_expressions[exp_index].Textures = LoadExpressionTextures(expression->FolderPath, *expression);
                                selected_path = selected_ref = fullpath;
                                clicked = true;
                            }
                        }
                    }
                }
            } else {
                // General file browser
                bool selected = (selected_ref == fullpath);
                if (ImGui::Selectable(entryName.c_str(), &selected)) {
                    selected_path = selected_ref = fullpath;
                    clicked = true;
                }
                if (drag_callback && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    drag_callback(fullpath);
                    ImGui::EndDragDropSource();
                }
            }
        }
    }
    if (search_len == 0) {
        was_searching_ref = false;
    }
    return clicked;
}

bool saturn_file_browser_create_imgui(const FileBrowserEntry& dir, const std::string& path, const std::string& browser_id, bool do_search, int exp_index) {
    if (search_terms.find(browser_id) == search_terms.end()) {
        char* s = (char*)malloc(256); s[0] = 0;
        search_terms[browser_id] = s;
    }
    if (selected_paths.find(browser_id) == selected_paths.end())
        selected_paths[browser_id] = "";
    if (was_searching.find(browser_id) == was_searching.end())
        was_searching[browser_id] = false;    const char* search_term = search_terms[browser_id];
    size_t search_len = strlen(search_term);
    return saturn_file_browser_create_imgui_impl(dir, path, browser_id, do_search, exp_index,
        search_term, search_len, selected_paths[browser_id], was_searching[browser_id]);
}

void saturn_file_browser_tools(std::string id, bool search, int exp_index) {
    ImGui::SetNextItemWidth(35);
    if (ImGui::Button(("Refresh###refresh_file_browser" + id).c_str())) {
        if (scanned_paths.find(last_scanned_path) != scanned_paths.end()) {
            delete scanned_paths[last_scanned_path];
            scanned_paths.erase(last_scanned_path);
        }
        if (id == "eyes") {
            current_expressions[exp_index].Refresh();
        }
    }
    ImGui::SameLine();
    if (exp_index >= 0 && exp_index < (int)current_expressions.size()) {
        TimelineButton(current_expressions[exp_index].Name, &current_expressions[exp_index].CurrentIndex, true);
    } else if (exp_index == -1) {
        if (browser_timeline_index.find(id) == browser_timeline_index.end())
            browser_timeline_index[id] = 0;
        //TimelineButton("ColorCode_" + id, &browser_timeline_index[id], true);
    }
    if (search) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth()-35);
        ImGui::InputTextWithHint(("###searchbar_file_browser_" + id).c_str(), "Search...", search_terms[id], 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsLowercase);
    }
    ImGui::Separator();
}

bool saturn_file_browser_show(std::string id, int exp_index) {
    if (id == "eyes") {
        if (custom_eyes) {
            if (current_expressions[exp_index].Textures.size() <= 0) { saturn_file_browser_clear(); return false; }
            if (current_expressions[exp_index].Textures[current_expressions[exp_index].CurrentIndex].RawData == 0) { saturn_file_browser_clear(); return false; }
        }
    }

    selected_path = "";

    if (selected_paths.find(id) == selected_paths.end()) selected_paths[id] = "";
    if (search_terms.find(id) == search_terms.end()) {
        char* s = (char*)malloc(256); s[0] = 0;
        search_terms[id] = s;
    }
    if (was_searching.find(id) == was_searching.end()) was_searching[id] = false;

    // Build flat file list for color code timeline index control, only when a timeline is active
    if (exp_index == -1) {
        std::string tl_key = "ColorCode_" + id;
        if (timelines.count(tl_key)) {
            browser_flat_lists[id].clear();
            flatten_browser_entries(root, "", browser_flat_lists[id]);
        }
    }

    ImGui::BeginChild(("###file_browser_" + id).c_str(), ImVec2(200 * ui_scale, browser_height * ui_scale), ImGuiChildFlags_Border);
    saturn_file_browser_tools(id, true, exp_index);
    bool result = saturn_file_browser_create_imgui(root, "", id, true, exp_index);
    ImGui::EndChild();

    // If a color code timeline is active, check if the index changed and trigger a load
    if (exp_index == -1) {
        std::string tl_key = "ColorCode_" + id;
        if (timelines.count(tl_key) && browser_timeline_index.find(id) != browser_timeline_index.end()) {
            auto& flat = browser_flat_lists[id];
            if (!flat.empty()) {
                int idx = std::max(0, std::min(browser_timeline_index[id], (int)flat.size() - 1));
                browser_timeline_index[id] = idx;
                if (flat[idx] != selected_paths[id]) {
                    selected_path = selected_paths[id] = flat[idx];
                    result = true;
                }
            }
        }
    }

    saturn_file_browser_clear();
    return result;
}

bool saturn_file_browser_show_tree(std::string id, int exp_index) {
    selected_path = "";

    if (selected_paths.find(id) == selected_paths.end()) selected_paths[id] = "";
    if (search_terms.find(id) == search_terms.end()) {
        char* s = (char*)malloc(256); s[0] = 0;
        search_terms[id] = s;
    }
    if (was_searching.find(id) == was_searching.end()) was_searching[id] = false;

    saturn_file_browser_tools(id, false, exp_index);
    bool result = saturn_file_browser_create_imgui(root, "", id, false, exp_index);
    saturn_file_browser_clear();
    return result;
}

std::filesystem::path saturn_file_browser_get_selected() {
    return selected_path;
}