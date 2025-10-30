#pragma once
#include <string>

struct AppConfig {
	std::string trigger_key;
	std::string hold_key; // user key (W is implicit)
	std::string jump_physical_key;
	std::string jump_virtual_key;
	std::string crouch_physical_key;
	std::string crouch_virtual_key;
	std::string weapon_swap_key;
	bool start_with_windows = false;
	bool start_minimized = false;
};

bool LoadConfig(AppConfig& cfg);
bool SaveConfig(const AppConfig& cfg);
std::wstring GetConfigPathW();
bool SetRunAtStartup(bool enable);
bool IsRunAtStartupEnabled();
