#ifndef _EOKAS_ROSE_AXION_FIELD_TYPES_H_
#define _EOKAS_ROSE_AXION_FIELD_TYPES_H_

#include "./Header.h"

#include <vector>

namespace eokas::RoseAxion {

    enum class FieldKind
    {
        Text,
        Integer,
        Float,
        Boolean,
        Date,
        Time,
        DateTime,
        Url,
        Email,
        SingleOption,
        MultiOption,
        Color,
        FilePath,
        FolderPath,
    };

    const char* fieldKindKey(FieldKind kind);
    const char* fieldKindLabel(FieldKind kind);
    FieldKind fieldKindFromKey(const String& key);
    bool fieldKindIsText(FieldKind kind);
    const char* schemaNameFromFieldKind(FieldKind kind);
    FieldKind fieldKindFromSchemaName(const String& name);
    String canonicalizeColumnType(const String& stored);
    String schemaLabel(const String& schemaName);

    String trimCopy(const String& text);
    bool validateField(FieldKind kind, const String& text);
    String canonicalText(FieldKind kind, const String& text);
    String displayDate(const String& stored);
    String displayTime(const String& stored);
    String displayDateTime(const String& stored);
    bool parseDate(const String& text, int& year, int& month, int& day);
    bool parseTime(const String& text, int& hour, int& minute, int& second);
    bool parseDateTime(const String& text, int& year, int& month, int& day, int& hour, int& minute, int& second);
    String formatDate(int year, int month, int day);
    String formatTime(int hour, int minute, int second);
    String formatDateTime(int year, int month, int day, int hour, int minute, int second);

    bool parseInteger(const String& text, i64_t& out);
    bool parseFloat(const String& text, f64_t& out);
    bool parseBool(const String& text, bool& out);
    bool parseColor(const String& text, Color& out);
    String formatColor(const Color& value);

    String joinSelected(const std::vector<String>& labels);
    std::vector<String> splitSelected(const String& text);
    bool selectedContains(const std::vector<String>& labels, const String& label);

}

#endif//_EOKAS_ROSE_AXION_FIELD_TYPES_H_
