
#include "./cli.h"

namespace eokas::cli {
    
    String Option::toString() const {
        return String::format("\t%s\t\t\t\t%s (default:%s)\n", name.cstr(), info.cstr(), value.string().cstr());
    }
    
    Command::Command()
        : name(), info(), options(), func(), subCommands() {
    }
    
    Command::Command(const String& name, const String& info)
        : name(name), info(info), options(), func(), subCommands() {
    }
    
    Command& Command::option(const String& name, const String& info, const StringValue& defaultValue) {
        Option& opt = this->options[name];
        opt.name = name;
        opt.info = info;
        opt.value = defaultValue;
        
        return *this;
    }
    
    Command& Command::action(const Func& func) {
        this->func = func;
        return *this;
    }
    
    Command& Command::subCommand(const String& name, const String& info) {
        Command& cmd = this->subCommands[name];
        cmd.name = name;
        cmd.info = info;
        
        return cmd;
    }
    
    StringValue Command::fetchValue(const String& shortName) const {
        auto opt = this->fetchOption(shortName);
        if (opt == std::nullopt)
            return StringValue();
        return opt->value;
    }
    
    std::optional<Option> Command::fetchOption(const String& shortName) const {
        for (auto& iter: this->options) {
            auto fragments = iter.first.split(",");
            if (std::find(fragments.begin(), fragments.end(), shortName) != fragments.end())
                return iter.second;
        }
        return std::nullopt;
    }
    
    std::optional<Command> Command::fetchCommand(const String& shortName) const {
        for (auto& iter: this->subCommands) {
            auto fragments = iter.first.split(",");
            if (std::find(fragments.begin(), fragments.end(), shortName) != fragments.end())
                return iter.second;
        }
        return std::nullopt;
    }
    
    String Command::toString() const {
        String str = String::format("%s\t\t\t\t%s\n", name.cstr(), info.cstr());
        for (auto& opt: this->options) {
            str += opt.second.toString();
        }
        for (auto& cmd: this->subCommands) {
            str += cmd.second.toString();
        }
        return str;
    }
    
    void Command::exec(size_t argc, char const* const* argv) {
        if (argc <= 0 || this->name != String(argv[0]))
            throw std::invalid_argument("Invalid arguments");
        
        std::vector<StringValue> args;
        for (size_t i = 0; i < argc; i++) {
            String arg = argv[i];
            args.emplace_back(arg);
        }
        
        bool isArgumentsConsumedByCommands = false;
        bool isArgumentsConsumedByOptions = false;
        
        // process sub-commands.
        if (args.size() > 1 && this->subCommands.size() > 0) {
            String cmdName = args[1];
            for (auto& cmd: this->subCommands) {
                // compatible with "-v,--version"
                auto fragments = cmd.first.split(",");
                if (std::find(fragments.begin(), fragments.end(), cmdName) == fragments.end())
                    continue;
                
                isArgumentsConsumedByCommands = true;
                return cmd.second.exec(argc - 1, argv + 1);
            }
            
        }
        
        // process options.
        if (args.size() > 1 && this->options.size() > 0) {
            for (auto& opt: this->options) {
                // compatible with "-v,--version"
                auto fragments = opt.first.split(",");
                for (const auto& frag: fragments) {
                    auto argIter = std::find(args.begin(), args.end(), frag);
                    if (argIter == args.end())
                        continue;
                    
                    ++argIter;
                    
                    // --option0 --option1
                    opt.second.value = argIter == args.end() || argIter->string().startsWith("-") ? "true" : *argIter;
                    
                    isArgumentsConsumedByOptions = true;
                }
            }
        }
        
        // Only has commands but arguments were not consumed by commands.
        if (this->subCommands.size() > 0 && this->options.size() <= 0 && !isArgumentsConsumedByCommands)
            throw std::invalid_argument("Invalid arguments.");
        
        // Has options but arguments didn't consumed by options.
        if (this->options.size() > 0 && !isArgumentsConsumedByOptions)
            throw std::invalid_argument("Invalid arguments");
        
        return this->func(*this);
    }
    
    void Command::exec(const std::vector<const char*>& args) {
        this->exec(args.size(), args.data());
    }
    
}
