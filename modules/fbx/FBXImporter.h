#ifndef _EOKAS_FBX_FBXIMPORTER_H_
#define _EOKAS_FBX_FBXIMPORTER_H_

#include "base/main.h"

namespace eokas
{
    class FBXImporter
    {
    public:
        FBXImporter();
        virtual ~FBXImporter();
        
        bool init();
        void quit();
        
    private:
        struct FBXImporterImpl* mImpl;
    };
}

#endif//_EOKAS_FBX_FBXIMPORTER_H_