#pragma once

#ifdef _WIN32
  #if defined(KEYHOOK_STATIC)
    #define KEYHOOK_API
  #elif defined(KEYHOOK_EXPORTS)
    #define KEYHOOK_API __declspec(dllexport)
  #else
    #define KEYHOOK_API __declspec(dllimport)
  #endif
#else
  #define KEYHOOK_API
#endif

extern "C" {

// Matches Python expectations: start_hook, configure_keys, set_active, set_hold, stop_hook, set_restrict_to_emulators
KEYHOOK_API void start_hook();
KEYHOOK_API void stop_hook();
KEYHOOK_API void set_active(int active);
KEYHOOK_API void set_hold(int hold);
KEYHOOK_API void set_restrict_to_emulators(int restrict_flag);
KEYHOOK_API void configure_keys(const char* jump_phys,
                               const char* jump_virt,
                               const char* crouch_phys,
                               const char* crouch_virt,
                               const char* trigger,
                               const char* hold1,
                               const char* hold2);

}
