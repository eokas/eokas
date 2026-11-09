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

    const char* fieldKindKey(FieldKind kind)
    {
        switch (kind)
        {
        case FieldKind::Integer: return "integer";
        case FieldKind::Float: return "float";
        case FieldKind::Boolean: return "boolean";
        case FieldKind::Date: return "date";
        case FieldKind::Time: return "time";
        case FieldKind::DateTime: return "datetime";
        case FieldKind::Url: return "url";
        case FieldKind::Email: return "email";
        case FieldKind::SingleOption: return "singleOption";
        case FieldKind::MultiOption: return "multiOption";
        case FieldKind::Color: return "color";
        case FieldKind::FilePath: return "filePath";
        case FieldKind::FolderPath: return "folderPath";
        case FieldKind::Text:
        default:
            return "text";
        }
    }

    const char* fieldKindLabel(FieldKind kind)
    {
        switch (kind)
        {
        case FieldKind::Integer: return "整数";
        case FieldKind::Float: return "浮点数";
        case FieldKind::Boolean: return "布尔值";
        case FieldKind::Date: return "日期";
        case FieldKind::Time: return "时间";
        case FieldKind::DateTime: return "日期时间";
        case FieldKind::Url: return "网址";
        case FieldKind::Email: return "邮箱地址";
        case FieldKind::SingleOption: return "单选";
        case FieldKind::MultiOption: return "多选";
        case FieldKind::Color: return "颜色";
        case FieldKind::FilePath: return "文件路径";
        case FieldKind::FolderPath: return "目录路径";
        case FieldKind::Text:
        default:
            return "文本";
        }
    }

    FieldKind fieldKindFromKey(const String& key)
    {
        if (key == "integer") return FieldKind::Integer;
        if (key == "float") return FieldKind::Float;
        if (key == "boolean") return FieldKind::Boolean;
        if (key == "date") return FieldKind::Date;
        if (key == "time") return FieldKind::Time;
        if (key == "datetime") return FieldKind::DateTime;
        if (key == "url") return FieldKind::Url;
        if (key == "email") return FieldKind::Email;
        if (key == "singleOption") return FieldKind::SingleOption;
        if (key == "multiOption") return FieldKind::MultiOption;
        if (key == "color") return FieldKind::Color;
        if (key == "filePath") return FieldKind::FilePath;
        if (key == "folderPath") return FieldKind::FolderPath;
        return FieldKind::Text;
    }

    bool fieldKindIsText(FieldKind kind)
    {
        return kind != FieldKind::Integer && kind != FieldKind::Float && kind != FieldKind::Boolean;
    }

    const char* schemaNameFromFieldKind(FieldKind kind)
    {
        switch (kind)
        {
        case FieldKind::Integer: return "Int";
        case FieldKind::Float: return "Float";
        case FieldKind::Boolean: return "Bool";
        case FieldKind::Date: return "Date";
        case FieldKind::Time: return "Time";
        case FieldKind::DateTime: return "DateTime";
        case FieldKind::Url: return "URL";
        case FieldKind::Email: return "Email";
        case FieldKind::Color: return "Color";
        case FieldKind::FilePath: return "FilePath";
        case FieldKind::FolderPath: return "FolderPath";
        case FieldKind::Text:
        default:
            return "String";
        }
    }

    FieldKind fieldKindFromSchemaName(const String& name)
    {
        if (name == "Int" || name == "integer") return FieldKind::Integer;
        if (name == "Float" || name == "float") return FieldKind::Float;
        if (name == "Bool" || name == "boolean") return FieldKind::Boolean;
        if (name == "Date" || name == "date") return FieldKind::Date;
        if (name == "Time" || name == "time") return FieldKind::Time;
        if (name == "DateTime" || name == "datetime") return FieldKind::DateTime;
        if (name == "URL" || name == "Url" || name == "url") return FieldKind::Url;
        if (name == "Email" || name == "email") return FieldKind::Email;
        if (name == "Color" || name == "color") return FieldKind::Color;
        if (name == "FilePath" || name == "filePath") return FieldKind::FilePath;
        if (name == "FolderPath" || name == "folderPath") return FieldKind::FolderPath;
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
