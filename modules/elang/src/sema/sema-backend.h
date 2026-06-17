#ifndef _EOKAS_SEMA_BACKEND_H_
#define _EOKAS_SEMA_BACKEND_H_

#include "sema-header.h"

namespace eokas
{
    /**
     * Abstract code-generation backend. A backend consumes the analyzed,
     * fully-typed semantic model of a single module and produces target source
     * code. Multiple backends can be plugged in; the sema layer is agnostic of
     * the concrete target language.
     */
    class sema_backend_t
    {
    public:
        virtual ~sema_backend_t() = default;

        // Consumes an analyzed module and produces target source. On failure
        // returns an empty string and sets the error message (see error()).
        virtual String generate(sema_module_t* module) = 0;

        virtual const String& error() const = 0;
    };
}

#endif //_EOKAS_SEMA_BACKEND_H_
