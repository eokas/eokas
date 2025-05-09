#include "scanner.h"

namespace eokas
{
    token_t::token_t()
        : type(UNKNOWN), value()
    {
    }
    
    const char* const token_t::name() const
    {
        if (type < COUNT)
            return names[type];
        return nullptr;
    }
    
    bool token_t::infer(token_type defaultType)
    {
        bool found = false;
        int count = (int) (token_t::COUNT);
        for (int i = 0; i < count; i++)
        {
            if (this->value == token_t::names[i])
            {
                this->type = (token_t::token_type) i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            this->type = defaultType;
        }
        return found;
    }
    
    void token_t::clear()
    {
        this->type = token_t::UNKNOWN;
        this->value = "";
    }
    
    scanner_t::scanner_t()
        : m_source(nullptr), m_position(nullptr), m_current(0), m_token(), m_look_ahead_token(), m_line(0), m_column(0)
    {
    }
    
    scanner_t::~scanner_t()
    {
        this->clear();
    }
    
    void scanner_t::ready(const char* source)
    {
        this->clear();
        m_source = source;
        m_position = source;
        this->read_char();
    }
    
    void scanner_t::clear()
    {
        m_source = nullptr;
        m_position = nullptr;
        m_current = 0;
        m_token.clear();
        m_look_ahead_token.clear();
        m_line = 1;
        m_column = 0;
    }
    
    const char* scanner_t::source()
    {
        return m_source;
    }
    
    void scanner_t::next_token()
    {
        if (m_look_ahead_token.type != token_t::UNKNOWN)
        {
            m_token = m_look_ahead_token;
            m_look_ahead_token.clear();
        }
        else
        {
            this->scan();
        }
    }
    
    token_t& scanner_t::token()
    {
        return m_token;
    }
    
    token_t& scanner_t::look_ahead_token()
    {
        if (m_look_ahead_token.type == token_t::UNKNOWN)
        {
            token_t tmp = m_token;
            this->scan();
            m_look_ahead_token = m_token;
            m_token = tmp;
        }
        return m_look_ahead_token;
    }
    
    int scanner_t::line()
    {
        return m_line;
    }
    
    int scanner_t::column()
    {
        return m_column;
    }
    
    void scanner_t::scan()
    {
        m_token.clear();
        
        for (;;)
        {
            switch (m_current)
            {
                case '\0':
                    m_token.type = token_t::EOS;
                    return;
                
                case '\n':
                case '\r':
                    this->new_line();
                    break;
                
                case ' ':
                case '\f':
                case '\t':
                case '\v': // spaces
                    this->read_char();
                    break;
                
                case '/':    // '//' '/*' '/'
                {
                    char div = m_current;
                    this->read_char();
                    if (m_current == '/') // line comment
                    {
                        this->scan_line_comment();
                        break;
                    }
                    else if (m_current == '*') // section comment
                    {
                        this->scan_section_comment();
                        break;
                    }
                    
                    this->save_char(div);
                    m_token.type = token_t::DIV;
                    return;
                }
                
                case '&': // && or &
                    this->save_and_read_char();
                    if (m_current == '&')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::AND2;
                        return;
                    }
                    else
                    {
                        m_token.type = token_t::AND;
                        return;
                    }
                
                case '|': // ||, |< or |
                    this->save_and_read_char();
                    if (m_current == '|')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::OR2;
                        return;
                    }
                    else if (m_current == '<')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::SHIFT_L;
                        return;
                    }
                    else
                    {
                        m_token.type = token_t::OR;
                        return;
                    }
                
                case '=': // == or =
                    this->save_and_read_char();
                    if (m_current == '=')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::EQ;
                        return;
                    }
                    else
                    {
                        m_token.type = token_t::ASSIGN;
                        return;
                    }
                
                case '<': // <=, <
                    this->save_and_read_char();
                    if (m_current == '=')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::LE;
                        return;
                    }
                    else
                    {
                        m_token.type = token_t::LT;
                        return;
                    }
                
                case '>': // >|, >= or >
                    this->save_and_read_char();
                    if (m_current == '|')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::SHIFT_R;
                        return;
                    }
                    else if (m_current == '=')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::GE;
                        return;
                    }
                    else
                    {
                        m_token.type = token_t::GT;
                        return;
                    }
                
                case '!': // != or !
                    this->save_and_read_char();
                    if (m_current == '=')
                    {
                        this->save_and_read_char();
                        m_token.type = token_t::NE;
                        return;
                    }
                    else
                    {
                        m_token.type = token_t::NOT;
                        return;
                    }
                
                case '.':
                    this->save_and_read_char();
                    if (m_current == '.')
                    {
                        this->save_and_read_char();
                        if (m_current == '.')
                        {
                            this->save_and_read_char();
                            m_token.type = token_t::DOT3;
                            return;
                        }
                        m_token.type = token_t::DOT2;
                        return;
                    }
                    m_token.type = token_t::DOT;
                    return;
                
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    this->scan_number();
                    return;
                
                case '"':
                case '\'':
                    this->scan_string(m_current);
                    return;
                default:
                    if (_ascil_is_alpha_(m_current))
                    {
                        this->scan_identifier();
                        return;
                    }
                    else
                    {
                        // single operator + - * / etc.
                        this->save_and_read_char();
                        m_token.infer(token_t::UNKNOWN);
                        return;
                    }
            }
        }
    }
    
    void scanner_t::scan_number()
    {
        this->save_and_read_char();
        
        if (m_current == 'b' || m_current == 'B') // bit
        {
            this->save_and_read_char();
            while (m_current == '0' || m_current == '1')
            {
                this->save_and_read_char();
            }
            m_token.type = token_t::INT_B;
        }
        else if (m_current == 'x' || m_current == 'X') // hex
        {
            this->save_and_read_char();
            while (_ascil_is_hex(m_current))
            {
                this->save_and_read_char();
            }
            m_token.type = token_t::INT_X;
        }
        else // decimal
        {
            while (_ascil_is_number(m_current))
            {
                this->save_and_read_char();
            }
            m_token.type = token_t::INT_D;
            
            if (m_current == '.')
            {
                this->save_and_read_char();
                while (_ascil_is_number(m_current))
                {
                    this->save_and_read_char();
                }
                m_token.type = token_t::FLOAT;
            }
        }
    }
    
    void scanner_t::scan_string(char delimiter)
    {
        this->read_char();
        while (m_current != delimiter)
        {
            if (m_current == '\0')
            {
                m_token.type = token_t::UNKNOWN;
                return;
            }
            else if (m_current == '\n' || m_current == '\r')
            {
                m_token.type = token_t::UNKNOWN;
                return;
            }
            else if (m_current == '\\') // eseokas sequence
            {
                this->read_char();
                switch (m_current)
                {
                    case 'a':
                        this->save_char('\a');
                        this->read_char();
                        break;
                    case 'b':
                        this->save_char('\b');
                        this->read_char();
                        break;
                    case 'f':
                        this->save_char('\f');
                        this->read_char();
                        break;
                    case 'n':
                        this->save_char('\n');
                        this->read_char();
                        break;
                    case 'r':
                        this->save_char('\r');
                        this->read_char();
                        break;
                    case 't':
                        this->save_char('\t');
                        this->read_char();
                        break;
                    case 'v':
                        this->save_char('\v');
                        this->read_char();
                        break;
                    case 'x': // \xFF
                        this->read_char();
                        if (!_ascil_is_hex(m_current))
                        {
                            m_token.type = token_t::UNKNOWN;
                            return;
                        }
                        this->save_and_read_char();
                        if (!_ascil_is_hex(m_current))
                        {
                            m_token.type = token_t::UNKNOWN;
                            return;
                        }
                        this->save_and_read_char();
                        break;
                    case '\\':
                    case '\'':
                    case '"':
                        this->save_and_read_char();
                        break;
                    default:
                        m_token.type = token_t::UNKNOWN;
                        return;
                }
            }
            else
            {
                this->save_and_read_char();
            }
        }
        this->read_char();
        m_token.type = token_t::STRING;
    }
    
    void scanner_t::scan_identifier()
    {
        this->save_and_read_char();
        while (_ascil_is_alpha_number_(m_current))
        {
            this->save_and_read_char();
        }
        m_token.infer(token_t::ID);
    }
    
    void scanner_t::scan_line_comment()
    {
        this->read_char();
        while (m_current != '\n' && m_current != '\r' && m_current != '\0')
            this->read_char(); // skip to end-of-line or end-of-source
    }
    
    void scanner_t::scan_section_comment()
    {
        this->read_char();
        for (;;)
        {
            switch (m_current)
            {
                case '\0':
                    m_token.type = token_t::EOS;
                    return;
                
                case '\n':
                case '\r':
                    this->new_line();
                    break;
                
                case '*':
                    this->read_char();
                    if (m_current == '/')
                    {
                        this->read_char(); // skip /
                        return;
                    }
                    else
                    {
                        break;
                    }
                default:
                    this->read_char();
                    break;
            }
        }
    }
    
    void scanner_t::new_line()
    {
        char old = m_current;
        this->read_char();
        if (m_current == '\n' || (m_current == '\r' && m_current != old))
            this->read_char();
        m_line++;
        m_column = 0;
    }
    
    void scanner_t::read_char()
    {
        m_current = *m_position;
        m_position++;
        m_column++;
    }
    
    void scanner_t::save_char(char c)
    {
        m_token.value.append(c);
    }
    
    void scanner_t::save_and_read_char()
    {
        this->save_char(m_current);
        this->read_char();
    }
    
    bool scanner_t::check_char(const char* charset)
    {
        const char* ptr = strchr(charset, m_current);
        return ptr != nullptr;
    }
}
