#pragma once

// WinSDKVer.h를 포함하여 Windows SDK 구성 요소를 제어합니다.
#include <WinSDKVer.h>

// 지원하려는 최하위 플랫폼을 Windows 8.1(WINBLUE)로 설정합니다.
// 0x0603은 Windows 8.1을 의미합니다.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0603
#endif

#ifndef WINVER
#define WINVER 0x0603
#endif

// 최하위 버전을 설정한 후 SDKDDKVer.h를 포함합니다.
#include <SDKDDKVer.h>
