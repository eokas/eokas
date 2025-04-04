
#include "./json.h"
#include "./ascil.h"

namespace eokas {
    
    struct JsonParser {
        String mSource;
        size_t mPosition;
        
        explicit JsonParser(const String& source)
            : mSource(source), mPosition(0) {
        }
        
        String nextName() {
            char c = this->nextCleanChar();
            switch (c) {
                case '\0': // eof
                    return "";
                
                case '\'':
                case '"': {
                    auto str = this->nextString(c);
                    return str.asString();
                }
                default:
                    if (_ascil_is_alpha_(c)) {
                        auto identifier = this->nextIdentifier(c);
                        return identifier;
                    }
            }
            
            return "";
        }
        
        HomNode nextValue() {
            char c = this->nextChar();
            switch (c) {
                case '[':
                    return this->nextArray();
                case '{':
                    return this->nextObject();
                case '\'':
                case '\"':
                    return this->nextString(c);
                default:
                    if (_ascil_is_number(c) || c == '+' || c == '-') {
                        return this->nextNumber(c);
                    } else if (_ascil_is_alpha_(c)) {
                        size_t start = mPosition;
                        auto identifier = this->nextIdentifier(c);
                        if (identifier == "true")
                            return HomNode{true};
                        else if (identifier == "false")
                            return HomNode{false};
                        else if (identifier == "null")
                            return HomNode{HomType::Null};
                        mPosition = start;
                    }
                    return HomNode{HomType::Null};
            }
        }
        
        HomNode nextArray() {
            HomNode list(HomType::Array);
            
            int count = 0;
            
            char first = this->nextCleanChar();
            if (first == ']') {
                return list;
            } else if (first == '\0') {
                mPosition -= 1;
            }
            
            while (true) {
                HomNode value = this->nextValue();
                list.add(value);
                
                char c = this->nextCleanChar();
                switch (c) {
                    case ']':
                        return list;
                    case ',':
                        continue;
                    default:
                        return HomNode{};
                }
            }
        }
        
        HomNode nextObject() {
            HomNode object(HomType::Object);
            
            /* Peek to see if this is the empty object. */
            char first = this->nextCleanChar();
            if (first == '}') {
                return object;
            } else if (first != '\0') {
                mPosition--;
            }
            
            while (true) {
                auto name = this->nextName();
                
                /*
                 * Expect the name/value separator to be either a colon ':', an
                 * equals sign '=', or an arrow "=>". The last two are bogus but we
                 * include them because that's what the original implementation did.
                 */
                char separator = this->nextCleanChar();
                if (separator != ':' && separator != '=') {
                    // error: "Expected ':' after " + name
                    return HomNode{};
                }
                if (mPosition < mSource.length() && mSource.substr(mPosition, 1).at(0) == '>') {
                    mPosition++;
                }
                HomNode val = this->nextValue();
                object.set(name, val);
                
                switch (this->nextCleanChar()) {
                    case '}':
                        return object;
                    case ';':
                    case ',':
                        continue;
                    default: // error: Unterminated object
                        return HomNode{};
                }
            }
        }
        
        HomNode nextNumber(char first) {
            String str(first);
            if (first != '+' && first != '-') {
                char c = this->nextChar();
                if (c == '.') {
                    str += c;
                    c = this->nextChar();
                    while (_ascil_is_number(c)) {
                        str += c;
                        c = this->nextChar();
                    }
                } else {
                    c = this->nextChar();
                    while (_ascil_is_number(c)) {
                        str += c;
                        c = this->nextChar();
                    }
                    if (c == '.') {
                        str += c;
                        c = this->nextChar();
                    }
                    while (_ascil_is_number(c)) {
                        str += c;
                        c = this->nextChar();
                    }
                }
            } else {
                char c = this->nextChar();
                while (_ascil_is_number(c)) {
                    str += c;
                    c = this->nextChar();
                }
                if (c == '.') {
                    str += c;
                    c = this->nextChar();
                }
                while (_ascil_is_number(c)) {
                    str += c;
                    c = this->nextChar();
                }
            }
            
            mPosition -= 1;
            
            auto value = String::stringToValue<f64_t>(str);
            return HomNode{value};
        }
        
        HomNode nextString(char quote) {
            String str = "";
            
            size_t start = mPosition;
            for (char c = this->nextChar(); c != '\0'; c = this->nextChar()) {
                if (c == quote) {
                    str += mSource.substr(start, mPosition - start - 1);
                    return HomNode{str};
                }
                
                if (c == '\\') {
                    str += mSource.substr(start, mPosition - start - 1);
                    str += this->escapeChar();
                    start = mPosition;
                }
            }
            
            // error : Unterminated string
            return HomNode{};
        }
        
        String nextIdentifier(char first) {
            String str(first);
            char c = this->nextChar();
            while (_ascil_is_alpha_number(c)) {
                str += c;
                c = this->nextChar();
            }
            
            mPosition -= 1;
            
            return str;
        }
        
        char escapeChar() {
            char c = this->nextChar();
            switch (c) {
                case 'u': {
                    if (mPosition + 4 >= mSource.length()) {
                        // error: Unterminated escape sequence
                        return ' ';
                    }
                    
                    String hex = mSource.substr(mPosition, 4);
                    mPosition += 4;
                    
                    // TODO: Int.parse(hex, 16) as char
                    return ' ';
                }
                
                case 'a':
                    return '\a';
                case 'b':
                    return '\b';
                case 'f':
                    return '\f';
                case 't':
                    return '\t';
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 'v':
                    return '\v';
                
                case '\'':
                case '"':
                case '\\':
                default:
                    return c;
            }
        }
        
        char nextCleanChar() {
            for (char c = this->nextChar(); c != '\0'; c = this->nextChar()) {
                switch (c) {
                    case ':':
                    case '\t':
                    case '\n':
                    case '\r':
                        continue;
                    case '#':  // php # comment
                        this->skipToEndOfLine();
                        continue;
                    case '/': {
                        char next = this->nextChar();
                        if (next == '*')  // c-style /**/ comment
                        {
                            size_t commentEnd = mSource.find("*/", mPosition);
                            if (commentEnd == -1) // error
                                return '\0';
                            mPosition = commentEnd + 2;
                            continue;
                        } else if (next == '/') // cpp style // comment
                        {
                            this->skipToEndOfLine();
                            continue;
                        } else {
                            return c;
                        }
                    }
                    default:
                        return c;
                }
            }
            
            return '\0';
        }
        
        void skipToEndOfLine() {
            char c = this->nextChar();
            while (c != '\0') {
                if (c == '\r' || c == '\n')
                    break;
                c = this->nextChar();
            }
        }
        
        char nextChar() {
            if (mPosition >= mSource.length())
                return '\0';
            char c = mSource.at(mPosition);
            mPosition += 1;
            return c;
        }
    };
    
    String JSON::stringify(const HomNode& json) {
        switch (json.type()) {
            case HomType::Null: {
                return "null";
            }
            case HomType::Number: {
                return String::valueToString(json.asNumber());
            }
            case HomType::Boolean: {
                return String::valueToString(json.asBoolean());
            }
            case HomType::String: {
                return String::format("\"%s\"", json.asString().cstr());
            }
            case HomType::Array: {
                String str = "[";
                bool first = true;
                json.foreach([&](const HomNode& val)->void {
                    if (first) {
                        str += ", ";
                        first = false;
                    }
                    str += stringify(val);
                });
                str += "]";
                return str;
            }
            case HomType::Object: {
                String str = "{";
                bool first = true;
                json.foreach([&](const String& key, const HomNode& val)->void {
                    if (first) {
                        str += ", ";
                        first = false;
                    }
                    str += String::format("\"%s\":", key.cstr());
                    str += stringify(val);
                });
                str += "}";
                return str;
            }
        }
        return "";
    }
    
    HomNode JSON::parse(const String& source) {
        JsonParser parser{source};
        return parser.nextValue();
    }
    
}
