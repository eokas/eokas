
#include "./logger.h"
#include "./string.h"
#include <stack>

namespace eokas {
    
    struct LoggerManager {
        static LoggerManager& instance() {
            static LoggerManager sInstance;
            return sInstance;
        }
        
        std::stack<String> names;
        std::map<String, Logger*> loggers;
        
        ~LoggerManager() {
            if (!loggers.empty()) {
                
                auto iter = loggers.begin();
                while (iter != loggers.end()) {
                    Logger* logger = iter->second;
                    if (logger != nullptr) {
                        delete logger;
                    }
                    ++iter;
                }
                loggers.clear();
            }
        }
    };
    
    void Logger::push(const String& name) {
        LoggerManager::instance().names.push(name);
    }
    
    void Logger::pop() {
        LoggerManager::instance().names.pop();
    }
    
    Logger* Logger::log() {
        const String& name = LoggerManager::instance().names.top();
        return Logger::log(name);
    }
    
    Logger* Logger::log(const String& name) {
        LoggerManager& manager = LoggerManager::instance();
        auto iter = manager.loggers.find(name);
        if (iter != manager.loggers.end())
            return iter->second;
        Logger* logger = new Logger();
        if (!logger->open(name)) {
            delete logger;
            return nullptr;
        }
        manager.loggers[name] = logger;
        return logger;
    }
    
    Logger::Logger()
        : mFile() {
    }
    
    Logger::~Logger() {
        this->close();
    }
    
    bool Logger::open(const String& name) {
        this->close();
        mFile.open(name.cstr());
        if (mFile.is_open()) {
            mFile << "<style>\n";
            mFile << ".Verbose{color:#000000;}\n";
            mFile << ".Notice{color:#00FF00;}\n";
            mFile << ".Warning{color:#FFFF00;}\n";
            mFile << ".Error{color:#FF0000;}\n";
            mFile << "</style>\n";
        }
        return mFile.is_open();
    }
    
    void Logger::close() {
        if (mFile.is_open()) {
            mFile.close();
        }
    }
    
    void Logger::info(const char* fmt, ...) {
        String message;
        _FormatVA(message, fmt);
        this->message(LogLevel::Verbose, message);
    }
    
    void Logger::warning(const char* fmt, ...) {
        String message;
        _FormatVA(message, fmt);
        this->message(LogLevel::Warning, message);
    }
    
    void Logger::error(const char* fmt, ...) {
        String message;
        _FormatVA(message, fmt);
        this->message(LogLevel::Error, message);
    }
    
    void Logger::message(LogLevel level, const String& message) {
        if (!mFile.is_open())
            return;
        if (this->callback.hasHandler()) {
            LogSignalMessage log;
            log.level = level;
            log.message = message;
            this->callback(log);
        }
        const char* head = nullptr;
        if (level == LogLevel::Verbose) head = "<span class='Verbose'>";
        else if (level == LogLevel::Notice) head = "<span class='Notice'>";
        else if (level == LogLevel::Warning) head = "<span class='Warning'>";
        else if (level == LogLevel::Error) head = "<span class='Error'>";
        const char* tail = "</span></br>";
        mFile << head << message.cstr() << tail << std::endl;
    }
}
