#include "./FieldTypes.h"
#include "./BuildinSchema.h"

#include <cstdlib>

namespace eokas::RoseAxion {

    namespace {

        bool isDigit(char c)
        {
            return c >= '0' && c <= '9';
        }

        String lowerAscii(const String& text)
        {
            String out = String::empty;
            for (size_t i = 0; i < text.length(); ++i)
            {
                char c = text.at(i);
                if (c >= 'A' && c <= 'Z')
                {
                    c = static_cast<char>(c - 'A' + 'a');
                }
                out = out + String(c);
            }
            return out;
        }

        bool leapYear(int year)
        {
            return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        }

        int daysInMonth(int year, int month)
        {
            static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (month < 1 || month > 12)
            {
                return 0;
            }
            if (month == 2 && leapYear(year))
            {
                return 29;
            }
            return days[month];
        }

        bool takeDigits(const String& text, size_t& index, int count, int& out)
        {
            if (index + static_cast<size_t>(count) > text.length())
            {
                return false;
            }
            int value = 0;
            for (int i = 0; i < count; ++i)
            {
                char c = text.at(index + static_cast<size_t>(i));
                if (!isDigit(c))
                {
                    return false;
                }
                value = value * 10 + (c - '0');
            }
            index += static_cast<size_t>(count);
            out = value;
            return true;
        }

        bool takeOne(const String& text, size_t& index, char a, char b)
        {
            if (index >= text.length())
            {
                return false;
            }
            char c = text.at(index);
            if (c != a && (b == 0 || c != b))
            {
                return false;
            }
            ++index;
            return true;
        }

        bool validTime(int hour, int minute, int second)
        {
            return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
        }

        bool validDate(int year, int month, int day)
        {
            return year >= 1 && month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth(year, month);
        }

        bool parseTimeClock(const String& text, size_t& index, int& hour, int& minute, int& second)
        {
            if (!takeDigits(text, index, 2, hour))
            {
                return false;
            }
            if (!takeOne(text, index, ':', 0))
            {
                return false;
            }
            if (!takeDigits(text, index, 2, minute))
            {
                return false;
            }
            second = 0;
            if (index < text.length() && text.at(index) == ':')
            {
                if (!takeOne(text, index, ':', 0))
                {
                    return false;
                }
                if (!takeDigits(text, index, 2, second))
                {
                    return false;
                }
            }
            return validTime(hour, minute, second);
        }

        bool parseDateParts(const String& text, int& year, int& month, int& day, int& hour, int& minute, int& second)
        {
            String value = trimCopy(text);
            size_t index = 0;
            if (!takeDigits(value, index, 4, year))
            {
                return false;
            }
            if (!takeOne(value, index, '-', '/'))
            {
                return false;
            }
            if (!takeDigits(value, index, 2, month))
            {
                return false;
            }
            if (!takeOne(value, index, '-', '/'))
            {
                return false;
            }
            if (!takeDigits(value, index, 2, day))
            {
                return false;
            }
            hour = 0;
            minute = 0;
            second = 0;
            if (index < value.length())
            {
                if (!takeOne(value, index, 'T', ' ') && !takeOne(value, index, 't', 0))
                {
                    return false;
                }
                if (!parseTimeClock(value, index, hour, minute, second))
                {
                    return false;
                }
            }
            if (index != value.length())
            {
                return false;
            }
            return validDate(year, month, day);
        }

        bool parseTimeOnly(const String& text, int& hour, int& minute, int& second)
        {
            String value = trimCopy(text);
            size_t index = 0;
            if (!parseTimeClock(value, index, hour, minute, second))
            {
                return false;
            }
            return index == value.length();
        }

        int hexNibble(char c)
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

        bool parseHexByte(const String& text, size_t index, int& out)
        {
            int hi = hexNibble(text.at(index));
            int lo = hexNibble(text.at(index + 1));
            if (hi < 0 || lo < 0)
            {
                return false;
            }
            out = hi * 16 + lo;
            return true;
        }

        int colorByte(float value)
        {
            if (value < 0.0f) value = 0.0f;
            if (value > 1.0f) value = 1.0f;
            return static_cast<int>(value * 255.0f + 0.5f);
        }

    }

    namespace {

        struct FieldKindMeta
        {
            FieldKind kind;
            const char* key;
            const char* label;
            const char* schemaName;
        };

        const FieldKindMeta kFieldKinds[] = {
            { FieldKind::Text, "text", "Text", "String" },
            { FieldKind::Integer, "integer", "Integer", "Int" },
            { FieldKind::Float, "float", "Float", "Float" },
            { FieldKind::Boolean, "boolean", "Boolean", "Bool" },
            { FieldKind::Date, "date", "Date", "Date" },
            { FieldKind::Time, "time", "Time", "Time" },
            { FieldKind::DateTime, "datetime", "DateTime", "DateTime" },
            { FieldKind::Url, "url", "URL", "URL" },
            { FieldKind::Email, "email", "Email", "Email" },
            { FieldKind::SingleOption, "singleOption", "Single choice", "String" },
            { FieldKind::MultiOption, "multiOption", "Multiple choice", "String" },
            { FieldKind::Color, "color", "Color", "Color" },
            { FieldKind::FilePath, "filePath", "FilePath", "FilePath" },
            { FieldKind::FolderPath, "folderPath", "FolderPath", "FolderPath" },
        };

        const FieldKindMeta& fieldKindMeta(FieldKind kind)
        {
            for (const FieldKindMeta& meta : kFieldKinds)
            {
                if (meta.kind == kind)
                {
                    return meta;
                }
            }
            return kFieldKinds[0];
        }

    }

    const char* fieldKindKey(FieldKind kind)
    {
        return fieldKindMeta(kind).key;
    }

    const char* fieldKindLabel(FieldKind kind)
    {
        return fieldKindMeta(kind).label;
    }

    String fontWarmupText()
    {
        String text = String::empty;
        for (const FieldKindMeta& meta : kFieldKinds)
        {
            text = text + String(meta.label);
        }
        text = text + String("StructListOptions");
        text = text + String("FileHelpNewTableSchemaFieldSettingsCloseDeleteDetailsSelect");
        return text;
    }

    FieldKind fieldKindFromKey(const String& key)
    {
        for (const FieldKindMeta& meta : kFieldKinds)
        {
            if (key == meta.key)
            {
                return meta.kind;
            }
        }
        return FieldKind::Text;
    }

    bool fieldKindIsText(FieldKind kind)
    {
        return kind != FieldKind::Integer && kind != FieldKind::Float && kind != FieldKind::Boolean;
    }

    const char* schemaNameFromFieldKind(FieldKind kind)
    {
        if (kind == FieldKind::SingleOption || kind == FieldKind::MultiOption)
        {
            return "String";
        }
        return fieldKindMeta(kind).schemaName;
    }

    FieldKind fieldKindFromSchemaName(const String& name)
    {
        if (name == "Url")
        {
            return FieldKind::Url;
        }
        for (const FieldKindMeta& meta : kFieldKinds)
        {
            if (meta.kind == FieldKind::SingleOption || meta.kind == FieldKind::MultiOption)
            {
                continue;
            }
            if (name == meta.schemaName || name == meta.key)
            {
                return meta.kind;
            }
        }
        return FieldKind::Text;
    }

    String canonicalizeColumnType(const String& stored)
    {
        if (stored.isEmpty())
        {
            return String("String");
        }
        return stored;
    }

    String schemaLabel(const String& schemaName)
    {
        String canonical = canonicalizeColumnType(schemaName);
        if (isBuildinSchema(canonical))
        {
            return String(fieldKindLabel(fieldKindFromSchemaName(canonical)));
        }
        return schemaName;
    }

    const char* schemaTypeLabel(rose::SchemaType type)
    {
        if (type == rose::SchemaType::Struct)
        {
            return "Struct";
        }
        if (type == rose::SchemaType::List)
        {
            return "List";
        }
        if (type == rose::SchemaType::Options)
        {
            return "Options";
        }
        return "";
    }

    String trimCopy(const String& text)
    {
        size_t begin = 0;
        size_t end = text.length();
        while (begin < end)
        {
            char c = text.at(begin);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                break;
            }
            ++begin;
        }
        while (end > begin)
        {
            char c = text.at(end - 1);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                break;
            }
            --end;
        }
        if (begin == 0 && end == text.length())
        {
            return text;
        }
        return text.substr(begin, end - begin);
    }

    bool parseInteger(const String& text, i64_t& out)
    {
        String value = trimCopy(text);
        if (value.isEmpty())
        {
            return false;
        }
        size_t index = 0;
        bool negative = false;
        if (value.at(0) == '+' || value.at(0) == '-')
        {
            negative = value.at(0) == '-';
            index = 1;
        }
        if (index >= value.length())
        {
            return false;
        }
        i64_t number = 0;
        for (; index < value.length(); ++index)
        {
            char c = value.at(index);
            if (!isDigit(c))
            {
                return false;
            }
            number = number * 10 + (c - '0');
        }
        out = negative ? -number : number;
        return true;
    }

    bool parseFloat(const String& text, f64_t& out)
    {
        String value = trimCopy(text);
        if (value.isEmpty())
        {
            return false;
        }
        const char* begin = value.cstr();
        char* end = nullptr;
        double number = std::strtod(begin, &end);
        if (end == begin || end == nullptr || *end != '\0')
        {
            return false;
        }
        if (number != number || number > 1.0e300 || number < -1.0e300)
        {
            return false;
        }
        out = number;
        return true;
    }

    bool parseBool(const String& text, bool& out)
    {
        String value = lowerAscii(trimCopy(text));
        if (value == "true" || value == "1")
        {
            out = true;
            return true;
        }
        if (value == "false" || value == "0")
        {
            out = false;
            return true;
        }
        return false;
    }

    bool parseColor(const String& text, Color& out)
    {
        String value = trimCopy(text);
        if (!value.isEmpty() && value.at(0) == '#')
        {
            value = value.substr(1);
        }
        if (value.length() != 6 && value.length() != 8)
        {
            return false;
        }
        int channels[4] = {0, 0, 0, 255};
        int count = value.length() == 8 ? 4 : 3;
        for (int i = 0; i < count; ++i)
        {
            if (!parseHexByte(value, static_cast<size_t>(i * 2), channels[i]))
            {
                return false;
            }
        }
        out = Color(
            channels[0] / 255.0f,
            channels[1] / 255.0f,
            channels[2] / 255.0f,
            channels[3] / 255.0f);
        return true;
    }

    String formatColor(const Color& value)
    {
        return String::format(
            "#%02X%02X%02X%02X",
            colorByte(value.r),
            colorByte(value.g),
            colorByte(value.b),
            colorByte(value.a));
    }

    bool validateField(FieldKind kind, const String& text)
    {
        String value = trimCopy(text);
        if (value.isEmpty())
        {
            return true;
        }
        switch (kind)
        {
        case FieldKind::Integer:
        {
            i64_t number = 0;
            return parseInteger(value, number);
        }
        case FieldKind::Float:
        {
            f64_t number = 0;
            return parseFloat(value, number);
        }
        case FieldKind::Boolean:
        {
            bool flag = false;
            return parseBool(value, flag);
        }
        case FieldKind::Date:
        {
            int year = 0;
            int month = 0;
            int day = 0;
            return parseDate(value, year, month, day);
        }
        case FieldKind::Time:
        {
            int hour = 0;
            int minute = 0;
            int second = 0;
            return parseTime(value, hour, minute, second);
        }
        case FieldKind::DateTime:
        {
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            return parseDateTime(value, year, month, day, hour, minute, second);
        }
        case FieldKind::Color:
        {
            Color color;
            return parseColor(value, color);
        }
        case FieldKind::Url:
            return value.find("http://") == 0 || value.find("https://") == 0;
        case FieldKind::Email:
        {
            if (value.find(' ') != String::npos)
            {
                return false;
            }
            size_t at = value.find('@');
            if (at == String::npos || at == 0 || at + 1 >= value.length())
            {
                return false;
            }
            size_t dot = value.find('.', at + 1);
            return dot != String::npos && dot > at + 1 && dot + 1 < value.length();
        }
        case FieldKind::Text:
        case FieldKind::SingleOption:
        case FieldKind::MultiOption:
        case FieldKind::FilePath:
        case FieldKind::FolderPath:
        default:
            return true;
        }
    }

    String canonicalText(FieldKind kind, const String& text)
    {
        String value = trimCopy(text);
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        switch (kind)
        {
        case FieldKind::Date:
            if (!parseDate(value, year, month, day))
            {
                return value;
            }
            return formatDate(year, month, day);
        case FieldKind::Time:
            if (!parseTime(value, hour, minute, second))
            {
                return value;
            }
            return formatTime(hour, minute, second);
        case FieldKind::DateTime:
            if (!parseDateTime(value, year, month, day, hour, minute, second))
            {
                return value;
            }
            return formatDateTime(year, month, day, hour, minute, second);
        case FieldKind::Color:
        {
            Color color;
            if (!parseColor(value, color))
            {
                return value;
            }
            return formatColor(color);
        }
        default:
            return value;
        }
    }

    String joinSelected(const std::vector<String>& labels)
    {
        String out = String::empty;
        for (const String& label : labels)
        {
            String piece = trimCopy(label);
            if (piece.isEmpty())
            {
                continue;
            }
            if (!out.isEmpty())
            {
                out = out + ", ";
            }
            out = out + piece;
        }
        return out;
    }

    std::vector<String> splitSelected(const String& text)
    {
        std::vector<String> out;
        size_t start = 0;
        while (start <= text.length())
        {
            size_t comma = text.find(',', start);
            if (comma == String::npos)
            {
                String piece = trimCopy(text.substr(start, text.length() - start));
                if (!piece.isEmpty())
                {
                    out.push_back(piece);
                }
                break;
            }
            String piece = trimCopy(text.substr(start, comma - start));
            if (!piece.isEmpty())
            {
                out.push_back(piece);
            }
            start = comma + 1;
        }
        return out;
    }

    bool selectedContains(const std::vector<String>& labels, const String& label)
    {
        for (const String& item : labels)
        {
            if (item == label)
            {
                return true;
            }
        }
        return false;
    }

    String displayDate(const String& stored)
    {
        int year = 0;
        int month = 0;
        int day = 0;
        if (!parseDate(stored, year, month, day))
        {
            return stored;
        }
        return String::format("%04d/%02d/%02d", year, month, day);
    }

    String displayTime(const String& stored)
    {
        int hour = 0;
        int minute = 0;
        int second = 0;
        if (!parseTime(stored, hour, minute, second))
        {
            return stored;
        }
        return formatTime(hour, minute, second);
    }

    String displayDateTime(const String& stored)
    {
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        if (!parseDateTime(stored, year, month, day, hour, minute, second))
        {
            return stored;
        }
        return String::format("%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    }

    bool parseDate(const String& text, int& year, int& month, int& day)
    {
        int hour = 0;
        int minute = 0;
        int second = 0;
        return parseDateParts(text, year, month, day, hour, minute, second);
    }

    bool parseTime(const String& text, int& hour, int& minute, int& second)
    {
        int year = 0;
        int month = 0;
        int day = 0;
        if (parseDateParts(text, year, month, day, hour, minute, second))
        {
            return true;
        }
        return parseTimeOnly(text, hour, minute, second);
    }

    bool parseDateTime(const String& text, int& year, int& month, int& day, int& hour, int& minute, int& second)
    {
        if (parseDateParts(text, year, month, day, hour, minute, second))
        {
            return true;
        }
        if (!parseTimeOnly(text, hour, minute, second))
        {
            return false;
        }
        TimePoint now;
        year = now.year();
        month = now.month();
        day = now.date();
        return validDate(year, month, day);
    }

    String formatDate(int year, int month, int day)
    {
        return String::format("%04d-%02d-%02d", year, month, day);
    }

    String formatTime(int hour, int minute, int second)
    {
        return String::format("%02d:%02d:%02d", hour, minute, second);
    }

    String formatDateTime(int year, int month, int day, int hour, int minute, int second)
    {
        return String::format("%04d-%02d-%02dT%02d:%02d:%02d", year, month, day, hour, minute, second);
    }

}
