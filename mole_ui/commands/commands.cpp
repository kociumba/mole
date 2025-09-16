#include "commands.h"

vector<Command> parse_help_output(const string& help_text) {
    vector<Command> commands;
    std::stringstream ss(help_text);
    string line;

    std::getline(ss, line);

    while (std::getline(ss, line)) {
        if (line.size() < 4) continue;
        line = line.substr(4);

        size_t colon_pos = line.find(": ");
        if (colon_pos == string::npos) continue;

        string desc = line.substr(colon_pos + 2);
        desc.erase(desc.find_last_not_of(" \t") + 1);

        string cmd_part = line.substr(0, colon_pos);
        vector<string> aliases;
        size_t paren_pos = cmd_part.find('(');
        string primary = cmd_part.substr(0, paren_pos);
        primary.erase(primary.find_last_not_of(" \t") + 1);
        aliases.push_back(primary);

        if (paren_pos != string::npos) {
            size_t paren_end = cmd_part.find(')', paren_pos);
            if (paren_end != string::npos) {
                string aliases_str = cmd_part.substr(paren_pos + 1, paren_end - paren_pos - 1);
                std::stringstream aliases_ss(aliases_str);
                string alias;

                while (std::getline(aliases_ss, alias, ',')) {
                    alias.erase(0, alias.find_first_not_of(" \t"));
                    alias.erase(alias.find_last_not_of(" \t") + 1);
                    if (!alias.empty()) {
                        aliases.push_back(alias);
                    }
                }
            }
        }

        if (!primary.empty()) {
            commands.push_back({desc, aliases});
        }
    }
    return commands;
}