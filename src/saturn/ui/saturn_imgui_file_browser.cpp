#include "saturn_imgui_file_browser.h"

#include <string>
#include <filesystem>
#include <map>
#include <vector>
#include <algorithm>
#include "saturn/libs/imgui/imgui.h"
#include "saturn/ui/saturn_imgui_models.h"
#include "saturn/ui/saturn_imgui_colors.h"
#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_animations.h"

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
    bool is_dir() { return _dir; }
    std::vector<FileBrowserEntry> dir() { return entries; }
    FileBrowserEntry* parent() { return _parent; }
    std::string name() { return _name; }
    void clear() { entries.clear(); }
    void add_file(std::string name) { entries.push_back(FileBrowserEntry(name, false, this)); }
    FileBrowserEntry* add_dir(std::string name) {
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
std::string extension_filter = "";
std::map<std::string, char*> search_terms = {};
std::map<std::string, std::string> selected_paths = {};
std::map<std::filesystem::path, FileBrowserEntry*> scanned_paths = {};
std::filesystem::path last_scanned_path;

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
        if (extension_filter != "" && ext != ("." + extension_filter)) continue;
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
        free(scanned_paths[dir]);
        scanned_paths.erase(dir);
    }
    saturn_file_browser_scan_directory(dir, recursive);
}

void saturn_file_browser_filter_extension(std::string extension) {
    extension_filter = std::string(extension);
    std::transform(extension_filter.begin(), extension_filter.end(), extension_filter.begin(), ::tolower);
}

void saturn_file_browser_height(int height) {
    browser_height = height;
}

void saturn_file_browser_clear() {
    root.clear();
    browser_height = 150;
    extension_filter = "";
}

bool saturn_file_browser_create_imgui(FileBrowserEntry dir, std::string path, std::string browser_id, bool do_search, int exp_index) {
    bool clicked = false;
    for (FileBrowserEntry& entry : dir.dir()) {
        if (entry.name() == "eyes") continue;
        if (entry.is_dir()) {
            if (ImGui::TreeNode(entry.name().c_str())) {
                clicked |= saturn_file_browser_create_imgui(entry, path + entry.name() + "/", browser_id, do_search, exp_index);
                ImGui::TreePop();
            }
        } else {
            std::string filename = entry.name();
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
            if (do_search) { if (filename.find(search_terms[browser_id]) == std::string::npos) continue; }
            std::string fullpath = path + entry.name();

            if (exp_index >= 0) {
                // Expression selector
                Expression* expression = &current_expressions[exp_index];
                if (expression->Textures.size() <= 0) continue;
                
                for (TexturePath& texture : expression->Textures) {
                    if (texture.SmallExpressionPath(expression->Name) == fullpath) {
                        bool selected;
                        selected =  (expression->CurrentIndex == &texture - &expression->Textures[0]) ||
                                    (expression->BlinkIndex[0] != -1 && expression->BlinkIndex[0] == &texture - &expression->Textures[0]) ||
                                    (expression->BlinkIndex[1] != -1 && expression->BlinkIndex[1] == &texture - &expression->Textures[0]);

                        if (ImGui::Selectable(entry.name().c_str(), &selected)) {
                            selected_path = selected_paths[browser_id] = fullpath;
                            clicked = true;
                        }
                        if (ImGui::IsItemHovered()) {
                            OpenExpressionPreview(&texture);
                            // Right-click reloads the texture
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                current_expressions[exp_index].Textures = LoadExpressionTextures(expression->FolderPath, *expression);
                                selected_path = selected_paths[browser_id] = fullpath;
                                clicked = true;
                            }
                        }
                    }
                }
            }  else {
                // General file browser
                bool selected = (selected_paths[browser_id] == fullpath);
                if (ImGui::Selectable(entry.name().c_str(), &selected)) {
                    selected_path = selected_paths[browser_id] = fullpath;
                    clicked = true;
                }
            }
        }
    }
    return clicked;
}

void saturn_file_browser_tools(std::string id, bool search, int exp_index) {
    ImGui::SetNextItemWidth(35);
    if (ImGui::Button(("Refresh###refresh_file_browser" + id).c_str())) {
        if (scanned_paths.find(last_scanned_path) != scanned_paths.end()) {
            free(scanned_paths[last_scanned_path]);
            scanned_paths.erase(last_scanned_path);
        }
        if (id == "eyes") {
            current_expressions[exp_index].Refresh();
            UpdateEditorLabels();
        }
    }
    if (search) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth()-35);
        ImGui::InputTextWithHint(("###searchbar_file_browser_" + id).c_str(), "Search...", search_terms[id], 256);
    }
    ImGui::Separator();
}

bool saturn_file_browser_show(std::string id, int exp_index) {
    if (id == "eyes") {
        if (custom_eyes) {
            if (current_expressions[exp_index].Textures.size() <= 0) return false;
            if (current_expressions[exp_index].Textures[current_expressions[exp_index].CurrentIndex].RawData == 0) return false;
        }
    }

    selected_path = "";
    if (selected_paths.find(id) == selected_paths.end()) selected_paths.insert({ id, "" });
    if (search_terms.find(id) == search_terms.end()) {
        char* search = (char*)malloc(256);
        search[0] = 0;
        search_terms.insert({ id, search });
    }
    ImGui::BeginChild(("###file_browser_" + id).c_str(), ImVec2(200, browser_height), ImGuiChildFlags_Border);
    saturn_file_browser_tools(id, true, exp_index);
    bool result = saturn_file_browser_create_imgui(root, "", id, true, exp_index);
    ImGui::EndChild();
    saturn_file_browser_clear();
    return result;
}

bool saturn_file_browser_show_tree(std::string id, int exp_index) {
    selected_path = "";
    if (selected_paths.find(id) == selected_paths.end()) selected_paths.insert({ id, "" });
    if (search_terms.find(id) == search_terms.end()) {
        char* search = (char*)malloc(256);
        search[0] = 0;
        search_terms.insert({ id, search });
    }
    saturn_file_browser_tools(id, false, exp_index);
    bool result = saturn_file_browser_create_imgui(root, "", id, false, exp_index);
    saturn_file_browser_clear();
    return result;
}

std::filesystem::path saturn_file_browser_get_selected() {
    return selected_path;
}