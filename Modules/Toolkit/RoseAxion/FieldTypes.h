#ifndef _EOKAS_ROSE_AXION_FIELD_TYPES_H_
#define _EOKAS_ROSE_AXION_FIELD_TYPES_H_

#include "./Header.h"

namespace eokas::RoseAxion {

    enum class FieldKind
    {
        Text,
        Integer,
        Float,
        Boolean,
        DateTime,
        Url,
        Email,
        SingleOption,
    };

    const char* fieldKindKey(FieldKind kind);
    const char* fieldKindLabel(FieldKind kind);
    FieldKind fieldKindFromKey(const String& key);
    bool fieldKindIsText(FieldKind kind);

    String trimCopy(const String& text);
    bool validateField(FieldKind kind, const String& text);
    String canonicalText(FieldKind kind, const String& text);
    String displayDateTime(const String& stored);

    bool parseInteger(const String& text, i64_t& out);
    bool parseFloat(const String& text, f64_t& out);
    bool parseBool(const String& text, bool& out);

}

#endif//_EOKAS_ROSE_AXION_FIELD_TYPES_H_
