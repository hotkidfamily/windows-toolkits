#pragma once
#ifndef __MAGFUNCDEFINE_H__
#define __MAGFUNCDEFINE_H__

#include <magnification.h>

// Public Functions
typedef decltype(&::MagInitialize) fnMagInitialize;
typedef decltype(&::MagUninitialize) fnMagUninitialize;
typedef decltype(&::MagSetWindowSource) fnMagSetWindowSource;
typedef decltype(&::MagGetWindowSource) fnMagGetWindowSource;
typedef decltype(&::MagSetWindowTransform) fnMagSetWindowTransform;
typedef decltype(&::MagGetWindowTransform) fnMagGetWindowTransform;
typedef decltype(&::MagSetWindowFilterList) fnMagSetWindowFilterList;
typedef decltype(&::MagGetWindowFilterList) fnMagGetWindowFilterList;
typedef decltype(&::MagSetImageScalingCallback) fnMagSetImageScalingCallback;
typedef decltype(&::MagGetImageScalingCallback) fnMagGetImageScalingCallback;
typedef decltype(&::MagSetColorEffect) fnMagSetColorEffect;
typedef decltype(&::MagGetColorEffect) fnMagGetColorEffect;

struct MagInterface
{
    fnMagInitialize Initialize;
    fnMagUninitialize Uninitialize;
    fnMagSetWindowSource SetWindowSource;
    fnMagGetWindowSource GetWindowSource;
    fnMagSetWindowTransform SetWindowTransform;
    fnMagGetWindowTransform GetWindowTransform;
    fnMagSetWindowFilterList SetWindowFilterList;
    fnMagGetWindowFilterList GetWindowFilterList;
    fnMagSetImageScalingCallback SetImageScalingCallback;
    fnMagGetImageScalingCallback GetImageScalingCallback;
    fnMagSetColorEffect SetColorEffect;
    fnMagGetColorEffect GetColorEffect;
};

#endif //__MAGFUNCDEFINE_H__
