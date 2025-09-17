#ifndef RESOURCES_H
#define RESOURCES_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define IDR_IOSEVKA_REGULAR 101

inline bool GetEmbeddedResource(
    const char* resourceName, const char* resourceType, void*& data, size_t& size
) {
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) {
        return false;
    }

    HRSRC hResInfo = FindResourceA(hModule, resourceName, resourceType);
    if (!hResInfo) {
        return false;
    }

    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    if (!hResData) {
        return false;
    }

    data = LockResource(hResData);
    if (!data) {
        return false;
    }

    size = SizeofResource(hModule, hResInfo);
    if (size == 0) {
        return false;
    }

    return true;
}

#endif  // RESOURCES_H
