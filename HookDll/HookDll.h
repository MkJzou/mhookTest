#pragma once

void WriteLog(char* str, char* path);

#define APIHOOK_API extern "C" __declspec(dllexport) 

APIHOOK_API INT HookInstall();

APIHOOK_API INT HookUnstall();

INT HookOn();

INT HookOff();