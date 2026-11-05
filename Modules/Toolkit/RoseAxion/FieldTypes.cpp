#include "./FieldTypes.h"

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
                if (!takeDigits(value, index, 2, hour))
                {
                    return false;
                }
                if (!takeOne(value, index, ':', 0))
                {
                    return false;
                }
                if (!takeDigits(value, index, 2, minute))
                {
                    return false;
                }
                if (index < value.length())
                {
                    if (!takeOne(value, index, ':', 0))
                    {
                        return false;
                    }
                    if (!takeDigits(value, index, 2, second))
                    {
                        return false;
                    }
                }
            }
            if (index != value.length())
            {
                return false;
            }
            if (year < 1 || month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month))
            {
                return false;
            }
            if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
            {
                return false;
            }
            return true;
        }

    }

    const char* fieldKindKey(FieldKind kind)
    {
        switch (kind)
        {
        case FieldKind::Integer: return "integer";
        case FieldKind::Float: return "float";
        case FieldKind::Boolean: return "boolean";
        case FieldKind::DateTime: return "datetime";
        case FieldKind::Url: return "url";
        case FieldKind::Email: return "email";
        case FieldKind::SingleOption: return "singleOption";
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
        case FieldKind::DateTime: return "日期时间";
        case FieldKind::Url: return "网址";
        case FieldKind::Email: return "邮箱地址";
        case FieldKind::SingleOption: return "单选";
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
        if (key == "datetime") return FieldKind::DateTime;
        if (key == "url") return FieldKind::Url;
        if (key == "email") return FieldKind::Email;
        if (key == "singleOption") return FieldKind::SingleOption;
        return FieldKind::Text;
    }

    bool fieldKindIsText(FieldKind kind)
    {
        return kind != FieldKind::Integer && kind != FieldKind::Float && kind != FieldKind::Boolean;
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
        case FieldKind::DateTime:
        {
            int year = 0;
            int month = 0;
            int day = 0;
            int hour = 0;
            int minute = 0;
            int second = 0;
            return parseDateParts(value, year, month, day, hour, minute, second);
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
        default:
            return true;
        }
    }

    String canonicalText(FieldKind kind, const String& text)
    {
        String value = trimCopy(text);
        if (kind != FieldKind::DateTime)
        {
            return value;
        }
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        if (!parseDateParts(value, year, month, day, hour, minute, second))
        {
            return value;
        }
        return String::format("%04d-%02d-%02dT%02d:%02d:%02d", year, month, day, hour, minute, second);
    }

    String displayDateTime(const String& stored)
    {
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        if (!parseDateParts(stored, year, month, day, hour, minute, second))
        {
            return stored;
        }
        return String::format("%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    }

}
