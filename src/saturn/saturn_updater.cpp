extern "C" {
    #include "pc/platform.h"
}

#include "saturn/libs/cjson/cJSON.h"
#include "saturn/libs/downloader.h"
#include "saturn/saturn_version.h"
#include "saturn/ui/studio_notifications.h"

#include <thread>
#include <string.h>
#include <errno.h>
#include <zip.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define OWNER "Llennpie"
#define REPO "Pluto"

static std::string latest_version;
static std::string exe_path;
static std::thread update_thread;

#ifdef _WIN32
#define TARGET_OS "windows"
#define readlink(proc_self_exe, name, max) GetModuleFileName(NULL, name, MAX_PATH)
#define rename(src, dst) MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING)
#define chmod(...)
#else
#define TARGET_OS "linux"
#endif

static char* unzip(char* inbuf, size_t insiz, size_t* outsiz) {
    zip_error_t error;
    zip_stat_t file_stat;
    zip_source_t* zip_src = zip_source_buffer_create(inbuf, insiz, 0, &error);
    zip_t* zip_archive = zip_open_from_source(zip_src, 0, &error);
    zip_int64_t file_index = zip_name_locate(zip_archive, TARGET_NAME, ZIP_FL_NOCASE);
    zip_file_t* zip_file = zip_fopen_index(zip_archive, file_index, 0);
    zip_stat_index(zip_archive, file_index, 0, &file_stat);
    *outsiz = file_stat.size;
    char* outbuf = (char*)malloc(*outsiz);
    zip_fread(zip_file, outbuf, *outsiz);
    zip_fclose(zip_file);
    zip_close(zip_archive);
    return outbuf;
}

static std::string GetExePath() {
    char exe_path[PATH_MAX];
    readlink("/proc/self/exe", exe_path, PATH_MAX);
    return exe_path;
}

static void ReloadCallback(bool now) {
    if (!now) return;
#ifdef _WIN32
    // what the fuck is this microsoft
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    CreateProcess(NULL, (LPSTR)exe_path.c_str(), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    ExitProcess(0);
#else
    char* argv[] = {NULL};
    execv(exe_path.c_str(), argv);
#endif
}

static void CheckUpdateCallback(bool confirmed) {
    if (!confirmed) return;
    update_thread = std::thread([]() {
        Notif* notif = Notif::create_progress("Pluto Update", "Downloading Pluto...");

        std::string url = format("https://github.com/" OWNER "/" REPO "/releases/download/%s/" TARGET_OS ".zip", latest_version.c_str());
        Downloader downloader(url);
        downloader.progress([notif](double now, double total) {
            notif->set_progress(total == 0 ? 0 : now / total);
        });
        downloader.download();
        notif->set_progress(1);

        if (downloader.status >= 200 && downloader.status <= 299) {
            std::string new_path = format("%s/" TARGET_NAME ".update", sys_user_path());
            std::string old_path = format("%s/" TARGET_NAME ".old",    sys_user_path());
            exe_path = GetExePath();

            size_t outsiz;
            char* out = unzip(downloader.data.data(), downloader.data.size(), &outsiz);
            FILE* f = fopen(new_path.c_str(), "wb");
            if (!f)
                Notif::create_message(NotifColor::COL_ERROR, "Pluto Update", format("Could not update Pluto\n%s", strerror(errno)));
            else {
                fwrite(out, outsiz, 1, f);
                fclose(f);
#ifdef _WIN32
                rename(exe_path.c_str(), old_path.c_str());
#endif
                rename(new_path.c_str(), exe_path.c_str());
                chmod(exe_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
                Notif::create_confirm("Pluto Update", "Pluto successfully updated!\nRestart now?", ReloadCallback, "Now", "Later");
            }
            free(out);
        }
        else
            Notif::create_message(NotifColor::COL_ERROR, "Pluto Update", format("There was an error downloading the latest update.\nHTTP error code: %d", downloader.status));
    });
    update_thread.detach();
}

void CheckForUpdates() {
    update_thread = std::thread([]() {
        Downloader downloader("https://api.github.com/repos/" OWNER "/" REPO "/releases/latest");
        downloader.download();
        if (downloader.status < 200 || downloader.status > 299) return;
        cJSON* obj = cJSON_Parse(downloader.data.data());
        obj = cJSON_GetObjectItem(obj, "name");
        latest_version = cJSON_GetStringValue(obj);

        if (latest_version != SATURN_VERSION) Notif::create_confirm("Pluto Update",
            format("Pluto %s is available!", latest_version.c_str()),
            CheckUpdateCallback, "Update", "Cancel"
        );
    });
    update_thread.detach();
}
