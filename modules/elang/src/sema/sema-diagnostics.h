#ifndef _EOKAS_SEMA_DIAGNOSTICS_H_
#define _EOKAS_SEMA_DIAGNOSTICS_H_

#include "sema-header.h"

namespace eokas
{
    /**
     * A single diagnostic message produced during semantic analysis.
     *
     * The AST does not currently attach source positions to its nodes, so the
     * diagnostic locates the problem by a human readable context string (such as
     * a symbol or function name) rather than line / column.
     */
    struct sema_diagnostic_t
    {
        sema_diagnostic_level_t level = sema_diagnostic_level_t::SEVERE;
        String context = "";
        String message = "";
    };

    class sema_diagnostics_t
    {
    public:
        sema_diagnostics_t() = default;
        ~sema_diagnostics_t() { diagnostics.clear(); }

        void clear()
        {
            diagnostics.clear();
            errorCount = 0;
        }

        void error(const String& context, const char* fmt, ...);
        void warning(const String& context, const char* fmt, ...);
        void info(const String& context, const char* fmt, ...);

        bool has_errors() const { return errorCount > 0; }
        size_t count() const { return diagnostics.size(); }
        const std::vector<sema_diagnostic_t>& items() const { return diagnostics; }

        // Renders all diagnostics into a single, human readable string.
        String dump() const;

    private:
        void push(sema_diagnostic_level_t level, const String& context, const String& message);

    private:
        std::vector<sema_diagnostic_t> diagnostics = {};
        size_t errorCount = 0;
    };
}

#endif //_EOKAS_SEMA_DIAGNOSTICS_H_
