#include "commands.h"

unordered_map<string, Command> commands = {
    {
        "unload",
        {"'unload': unloads mole from it's host",
         {"unload"},
         [] {
             info("unloading mole from host ...");
             unload([] { g_unload = true; });
         }},
    },
    {
        "help",
        {"'help': show this help", {"help", "h"}, print_help},
    },
    {
        "leaks",
        {"'leaks': evaluates the current malloc/free data", {"leaks", "l"}, [] { print_leaks(); }},
    },
    {
        "leaks-v",
        {"'leaks-v': evaluates the current malloc/free data and prints stack traces",
         {"leaks-v", "lv"},
         [] { print_leaks(true); }},
    },
    {
        "filter-runtime",
        {"'filter-runtime': toggles filtering of leaks originating from windows runtime "
         "libraries",
         {"filter-runtime", "fr"},
         [] {
             g_filter_runtime_leaks = !g_filter_runtime_leaks;
             info("filtering of runtime libraries is {}", g_filter_runtime_leaks ? "on" : "off");
         }},
    },
    {
        "exit-host",
        {"'exit-host': signals the host to exit, may not work",
         {"exit-host", "eh"},
         [] {
             HWND handle;
             if (has_window(GetCurrentProcessId(), &handle)) {
                 PostMessage(handle, WM_CLOSE, 0, 0);
                 PostMessage(handle, WM_QUIT, 0, 0);
             } else {
                 ExitProcess(0);
             }
         }},
    },
    {
        "terminate-host",
        {"'terminate-host': terminates the host by force",
         {"terminate-host", "th"},
         [] { TerminateProcess(GetCurrentProcess(), 42069); }},
    },
};

static string format_aliases(const vector<string>& aliases, bool skip_primary = true) {
    if (aliases.size() <= (skip_primary ? 1u : 0u)) return "";
    std::stringstream ss;
    ss << "(";
    for (size_t i = skip_primary ? 1 : 0; i < aliases.size(); ++i) {
        if (i != (skip_primary ? 1 : 0)) ss << ", ";
        ss << aliases[i];
    }

    ss << ")";
    return ss.str();
}

const Command* find_command(const string& input) {
    for (const auto& [primary, cmd] : commands) {
        for (const auto& alias : cmd.aliases) {
            if (input == alias) {
                return &cmd;
            }
        }
    }
    return nullptr;
}

static string apply_alias_token(const string& desc, const string& aliases_str) {
    string out = desc;
    const string token = "%alias";
    size_t pos = 0;
    while ((pos = out.find(token, pos)) != string::npos) {
        out.replace(pos, token.size(), aliases_str);
        pos += aliases_str.size();
    }

    return out;
}

void print_help() {
    std::stringstream help;
    const auto indent = "    ";

    help << "mole commands:\n";
    for (const auto& [primary, cmd] : commands) {
        string aliases_str = format_aliases(cmd.aliases, true);
        // string formatted_desc = apply_alias_token(cmd.desc, aliases_str);

        help << indent << primary;
        if (!aliases_str.empty()) help << " " << aliases_str;
        help << ": " << cmd.desc << "\n";
    }

    std::print("{}", help.str());
}