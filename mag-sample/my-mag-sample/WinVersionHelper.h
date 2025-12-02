#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <Shlwapi.h>
#include <ShlObj.h>

#pragma warning(disable : 4996) // for GetVersionEx

#define WIN32_WINNT_NT4      0x0400
#define WIN32_WINNT_WIN2K    0x0500
#define WIN32_WINNT_WINXP    0x0501
#define WIN32_WINNT_WS03     0x0502
#define WIN32_WINNT_WIN6     0x0600
#define WIN32_WINNT_VISTA    0x0600
#define WIN32_WINNT_WS08     0x0600
#define WIN32_WINNT_LONGHORN 0x0600
#define WIN32_WINNT_WIN7     0x0601
#define WIN32_WINNT_WIN8     0x0602
#define WIN32_WINNT_WINBLUE  0x0603
#define WIN32_WINNT_WIN10    0x0a00

#define Win10_1507 10240
#define Win10_1511 10586
#define Win10_1607 14393
#define Win10_1703 15063
#define Win10_1709 16299
#define Win10_1803 17134
#define Win10_1809 17763
#define Win10_1903 18362
#define Win10_1909 18363
#define Win10_2004 19041
#define Win10_20H2 19042
#define Win10_21H1 19043
#define Win10_21H2 19044
#define Win10_22H2 19045

#define WIN11_21H2 22000
#define WIN11_22H2 22621
#define WIN11_23H2 22631
#define WIN11_24H2 26100

namespace Platform
{

static const OSVERSIONINFOEX &GetWindowsVersion()
{
    struct OSVERINFO
    {
        OSVERSIONINFOEX osvi;
        SYSTEM_INFO si;

        BOOL bCompatMode;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        DWORD dwBuildNumber;
        DWORD dwProductType;
    };

    static OSVERINFO g_osver_info = { 0 };

    typedef void(WINAPI * GetNtVersionNumbersFunctor)(LPDWORD, LPDWORD, LPDWORD);
    typedef NTSTATUS(WINAPI * RtlGetVersionFunctor)(PRTL_OSVERSIONINFOEXW);

    OSVERSIONINFOEX &osvi = g_osver_info.osvi;
    if (osvi.dwOSVersionInfoSize == 0) {
        BOOL bGetVersionEx = FALSE;
        OSVERSIONINFO osi = { 0 };
        osi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (bGetVersionEx = ::GetVersionEx(&osi)) {
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
            bGetVersionEx = ::GetVersionEx((OSVERSIONINFO *)(&osvi));
        }

        RTL_OSVERSIONINFOEXW rtl_osvi = { 0 };
        rtl_osvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
        RtlGetVersionFunctor RtlGetVersion
            = (RtlGetVersionFunctor)(::GetProcAddress(::GetModuleHandle(L"ntdll.dll"), "RtlGetVersion"));
        BOOL bRtlGetVersion = (RtlGetVersion != NULL ? RtlGetVersion(&rtl_osvi) == 0 : FALSE);

        GetNtVersionNumbersFunctor GetNtVersionNumbers
            = (GetNtVersionNumbersFunctor)(::GetProcAddress(::GetModuleHandle(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
        BOOL bGetNtVersionNumbers = (GetNtVersionNumbers != NULL);
        if (bGetNtVersionNumbers) {
            GetNtVersionNumbers(&g_osver_info.dwMajorVersion, &g_osver_info.dwMinorVersion,
                                &g_osver_info.dwBuildNumber);

            if (osvi.dwMajorVersion > 5 && bRtlGetVersion) {
                osvi.dwPlatformId = rtl_osvi.dwPlatformId;
                osvi.dwMajorVersion = rtl_osvi.dwMajorVersion;
                osvi.dwMinorVersion = rtl_osvi.dwMinorVersion;
                wcscpy_s(osvi.szCSDVersion, _countof(osvi.szCSDVersion), rtl_osvi.szCSDVersion);
                osvi.wServicePackMajor = rtl_osvi.wServicePackMajor;
                osvi.wServicePackMinor = rtl_osvi.wServicePackMinor;
                osvi.wSuiteMask = rtl_osvi.wSuiteMask;
                osvi.wProductType = rtl_osvi.wProductType;
                osvi.dwBuildNumber = rtl_osvi.dwBuildNumber;
            }

            g_osver_info.bCompatMode = (osvi.dwMajorVersion != g_osver_info.dwMajorVersion
                                        || osvi.dwMinorVersion != g_osver_info.dwMinorVersion
                                        || osvi.dwBuildNumber != (g_osver_info.dwBuildNumber & 0xffff));
            g_osver_info.dwBuildNumber &= 0xffff;
        }
        else {
            g_osver_info.dwMajorVersion = osvi.dwMajorVersion;
            g_osver_info.dwMinorVersion = osvi.dwMinorVersion;
            g_osver_info.dwBuildNumber = osvi.dwBuildNumber;
        }
    }

    return osvi;
}

inline bool IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
    OSVERSIONINFOEX osvi = GetWindowsVersion();
    if ((osvi.dwMajorVersion >= wMajorVersion)
        || (osvi.dwMajorVersion == wMajorVersion && osvi.dwMinorVersion >= wMinorVersion)
        || (osvi.dwMajorVersion == wMajorVersion && osvi.dwMinorVersion == wMinorVersion
            && osvi.wServicePackMajor >= wServicePackMajor)) {
        return true;
    }
    else {
        return false;
    }
}

inline bool IsWindowsBuildNumberOrGreater(DWORD dwBuildNumber)
{
    OSVERSIONINFOEX osvi = GetWindowsVersion();
    return osvi.dwBuildNumber >= dwBuildNumber;
}

inline bool IsWindows2kOrLess()
{
    OSVERSIONINFOEX osvi = GetWindowsVersion();
    if (osvi.dwMajorVersion <= HIBYTE(WIN32_WINNT_WIN2K)) {
        return true;
    }
    else {
        return false;
    }
}

inline bool IsWindowsXPOrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WINXP), LOBYTE(WIN32_WINNT_WINXP), 0);
}

inline bool IsWindowsXPSP1OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WINXP), LOBYTE(WIN32_WINNT_WINXP), 1);
}

inline bool IsWindowsXPSP2OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WINXP), LOBYTE(WIN32_WINNT_WINXP), 2);
}

inline bool IsWindowsXPSP3OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WINXP), LOBYTE(WIN32_WINNT_WINXP), 3);
}

inline bool IsWindowsVistaOrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_VISTA), LOBYTE(WIN32_WINNT_VISTA), 0);
}

inline bool IsWindowsVistaSP1OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_VISTA), LOBYTE(WIN32_WINNT_VISTA), 1);
}

inline bool IsWindowsVistaSP2OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_VISTA), LOBYTE(WIN32_WINNT_VISTA), 2);
}

inline bool IsWindows7OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WIN7), LOBYTE(WIN32_WINNT_WIN7), 0);
}

inline bool IsWindows7SP1OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WIN7), LOBYTE(WIN32_WINNT_WIN7), 1);
}

inline bool IsWindows8OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WIN8), LOBYTE(WIN32_WINNT_WIN8), 0);
}

inline bool IsWindows8Point1OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WINBLUE), LOBYTE(WIN32_WINNT_WINBLUE), 0);
}

inline bool IsWindows10OrGreater()
{
    return IsWindowsVersionOrGreater(HIBYTE(WIN32_WINNT_WIN10), LOBYTE(WIN32_WINNT_WIN10), 0);
}

inline bool IsWin10_1507OrGreater() // Threshold	¡ª
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1507));
}
inline bool IsWin10_1511OrGreater() // Threshold 2	November Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1511));
}
inline bool IsWin10_1607OrGreater() // Redstone Anniversary Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1607));
}
inline bool IsWin10_1703OrGreater() // Redstone 2 Creators Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1703));
}
inline bool IsWin10_1709OrGreater() // Redstone 3 Fall Creators Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1709));
}
inline bool IsWin10_1803OrGreater() // Redstone 4 April 2018 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1803));
}
inline bool IsWin10_1809OrGreater() // Redstone 5 October 2018 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1809));
}
inline bool IsWin10_1903OrGreater() // 19H1 May 2019 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1903));
}
inline bool IsWin10_1909OrGreater() // 19H2 November 2019 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_1909));
}
inline bool IsWin10_2004OrGreater() // 20H1 May 2020 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_2004));
}
inline bool IsWin10_20H2OrGreater() // 20H2 October 2020 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_20H2));
}
inline bool IsWin10_21H1OrGreater() // 21H1 May 2021 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_21H1));
}
inline bool IsWin10_21H2OrGreater() // 21H2 November 2021 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_21H2));
}
inline bool IsWin10_22H2OrGreater() // 22H2 2022 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(Win10_22H2));
}

inline bool IsWin11_21H2OrGreater() //	Sun Valley	¡ª
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(WIN11_21H2));
}
inline bool IsWin11_22H2OrGreater() //	Sun Valley 2	2022 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(WIN11_22H2));
}
inline bool IsWin11_23H2OrGreater() //	Sun Valley 3	2023 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(WIN11_23H2));
}
inline bool IsWin11_24H2OrGreater() //	Hudson Valley	2024 Update
{
    return (IsWindows10OrGreater() && IsWindowsBuildNumberOrGreater(WIN11_24H2));
}

inline bool IsWindows11OrGreater()
{
    return IsWin11_21H2OrGreater();
}

inline bool IsWindowsServer()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0, 0, VER_NT_WORKSTATION };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL);

    return !VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, dwlConditionMask);
}

} // namespace Platform
