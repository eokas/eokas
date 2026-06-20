#include "sema-diagnostics.h"
#include <cstdarg>

namespace eokas
{
    void sema_diagnostics_t::error(const String& context, const char* fmt, ...)
    {
        String message;
        _FormatVA(message, fmt);
        this->push(sema_diagnostic_level_t::SEVERE, context, message);
    }

    void sema_diagnostics_t::warning(const String& context, const char* fmt, ...)
    {
        String message;
        _FormatVA(message, fmt);
        this->push(sema_diagnostic_level_t::WARNING, context, message);
    }

    void sema_diagnostics_t::info(const String& context, const char* fmt, ...)
    {
        String message;
        _FormatVA(message, fmt);
        this->push(sema_diagnostic_level_t::INFO, context, message);
    }

    void sema_diagnostics_t::push(sema_diagnostic_level_t level, const String& context, const String& message)
    {
        sema_diagnostic_t d;
        d.level = level;
        d.context = context;
        d.message = message;
        this->diagnostics.push_back(d);
        if (level == sema_diagnostic_level_t::SEVERE)
            this->errorCount += 1;
    }

    String sema_diagnostics_t::dump() const
    {
        String result;
        for (const auto& d : this->diagnostics)
        {
            const char* tag = "info";
            switch (d.level)
            {
                case sema_diagnostic_level_t::SEVERE: tag = "error"; break;
                case sema_diagnostic_level_t::WARNING: tag = "warning"; break;
                case sema_diagnostic_level_t::INFO: tag = "info"; break;
            }

            if (d.context.isEmpty())
                result += String::format("[%s] %s\n", tag, d.message.cstr());
            else
                result += String::format("[%s] (%s) %s\n", tag, d.context.cstr(), d.message.cstr());
        }
        return result;
    }
}
