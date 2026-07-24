#ifdef PLUTO_UPDATER

extern "C" {
    #include "pc/platform.h"
}

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "saturn/libs/cjson/cJSON.h"
#include "saturn/libs/httplib.h"
#include "saturn/saturn_version.h"
#include "saturn/ui/studio_notifications.h"
#include "pc/utils/miniz/miniz.h"

#include <thread>
#include <string.h>
#include <errno.h>

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
    __attribute__((cleanup(mz_zip_reader_end))) mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_mem(&zip_archive, inbuf, insiz, 0)) return NULL;

    int file_index = mz_zip_reader_locate_file(&zip_archive, TARGET_NAME, NULL, 0);
    if (file_index < 0) return NULL;

    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, file_index, &file_stat)) return NULL;

    *outsiz = file_stat.m_uncomp_size;
    char* outbuf = (char*)malloc(*outsiz);
    if (!mz_zip_reader_extract_to_mem(&zip_archive, file_index, outbuf, *outsiz, 0)) {
        free(outbuf);
        return NULL;
    }

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

        std::string path = format("/" OWNER "/" REPO "/releases/download/%s/" TARGET_OS ".zip", latest_version.c_str());
        httplib::Client client("https://github.com");
        client.set_follow_location(true);
        auto res = client.Get(path, [&notif](size_t len, size_t total) {
            notif->set_progress(total == 0 ? 0 : (double)len / total);
            return true;
        });
        notif->set_progress(1);

        if (res->status >= 200 && res->status <= 299) {
            std::string new_path = format("%s/" TARGET_NAME ".update", sys_user_path());
            std::string old_path = format("%s/" TARGET_NAME ".old",    sys_user_path());
            exe_path = GetExePath();

            size_t outsiz;
            char* out = unzip(res->body.data(), res->body.size(), &outsiz);
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
            Notif::create_message(NotifColor::COL_ERROR, "Pluto Update", format("There was an error downloading the latest update.\nHTTP error code: %d", res->status));
    });
    update_thread.detach();
}

void CheckForUpdates() {
    update_thread = std::thread([]() {
        httplib::Client client("https://api.github.com");
        client.set_follow_location(true);
        auto res = client.Get("/repos/" OWNER "/" REPO "/releases/latest");
        if (res->status < 200 || res->status > 299) return;
        cJSON* obj = cJSON_Parse(res->body.c_str());
        obj = cJSON_GetObjectItem(obj, "name");
        latest_version = cJSON_GetStringValue(obj);

        if (latest_version != SATURN_VERSION) Notif::create_confirm("Pluto Update",
            format("Pluto %s is available!", latest_version.c_str()),
            CheckUpdateCallback, "Update", "Cancel"
        );
    });
    update_thread.detach();
}

#else
void CheckForUpdates() {}
#endif
