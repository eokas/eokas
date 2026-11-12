#ifndef _EOKAS_ROSE_AXION_HOST_H_
#define _EOKAS_ROSE_AXION_HOST_H_

#include "./Header.h"

#include <functional>

namespace eokas::RoseAxion {

    struct RoseUiHost
    {
        std::function<void()> requestRebuild;
        std::function<void(const String&)> writeClipboard;
        std::function<String()> readClipboard;
        String font;
        float scale = 1.0f;
    };

    inline float hostScale(float scale)
    {
        return scale < 0.5f ? 1.0f : scale;
    }

}

#endif
