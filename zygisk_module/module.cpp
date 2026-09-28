// language: C++17, file: zygisk_module/module.cpp, target: Android ARM64
// Zygisk module — injects Cryptonit into com.axlebolt.standoff2
// Requires: KernelSU + ZygiskNext, or Magisk + Zygisk enabled
// Zygisk API headers: https://github.com/topjohnwu/zygisk-api
#include "zygisk.hpp"
#include <android/log.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <cstring>

#define TAG "CryptonitZygisk"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

using namespace zygisk;

class CryptonitModule : public ModuleBase {
public:
    void onLoad(Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        // Only run in Standoff 2 process
        const char* pkg = env_->GetStringUTFChars(args->nice_name, nullptr);
        bool is_target = (pkg && strstr(pkg, "com.axlebolt.standoff2"));
        if (pkg) env_->ReleaseStringUTFChars(args->nice_name, pkg);

        if (!is_target) {
            // Not our target — tell Zygisk we need nothing
            api_->setOption(Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Request file access for loading our payload from module directory
        api_->setOption(Option::FORCE_DENYLIST_UNMOUNT);

        // Get module directory fd for loading payload .so
        module_dir_fd_ = api_->getModuleDir();
        LOGI("Targeting Standoff 2 — preAppSpecialize done");
    }

    void postAppSpecialize(const AppSpecializeArgs* args) override {
        if (module_dir_fd_ < 0) return;

        // Load our companion payload from the module data directory
        // File: /data/adb/modules/cryptonit/cryptonit_payload.so
        // Placed by Magisk module installer
        char path[256];
        snprintf(path, sizeof(path), "/proc/self/fd/%d/cryptonit_payload.so",
                 module_dir_fd_);

        // Fallback path
        if (access(path, R_OK) != 0) {
            strncpy(path, "/data/adb/modules/cryptonit/cryptonit_payload.so",
                    sizeof(path));
        }

        void* lib = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
        if (!lib) {
            LOGE("dlopen failed: %s", dlerror());
            return;
        }

        // Call our entry point
        auto entry = reinterpret_cast<void(*)()>(dlsym(lib, "cryptonit_main"));
        if (!entry) {
            LOGE("cryptonit_main not found: %s", dlerror());
            return;
        }

        LOGI("Launching Cryptonit payload...");
        entry();
    }

    void preServerSpecialize(ServerSpecializeArgs*) override {
        api_->setOption(Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api*    api_           = nullptr;
    JNIEnv* env_           = nullptr;
    int     module_dir_fd_ = -1;
};

REGISTER_ZYGISK_MODULE(CryptonitModule)
