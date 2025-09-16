#include "root.h"

vector<string> output_log;
vector<string> command_history;
char input_buffer[256] = "";
int history_index = -1;
bool scroll_to_bottom = false;
vector<Command> commands;

void mole_ui_root() {
    string msg;
    while (ipc_client_poll(&g_ipc, msg)) {
        if (msg.find("mole commands:") == 0) {
            commands = parse_help_output(msg);
        } else {
            output_log.push_back(msg);
            scroll_to_bottom = true;
        }
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io->DisplaySize);
    ImGui::Begin("mole", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Available Commands:");
    ImGui::BeginChild(
        "Commands", ImVec2(0, ImGui::GetTextLineHeight() * (commands.size() + 2)), true
    );
    for (const auto& cmd : commands) {
        std::string aliases_str;
        for (size_t i = 0; i < cmd.aliases.size(); ++i) {
            if (i > 0) aliases_str += ", ";
            aliases_str += cmd.aliases[i];
        }
        ImGui::Text("%s: %s", aliases_str.c_str(), cmd.desc.c_str());
    }
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::Text("Output:");
    ImGui::BeginChild("Output", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2), true);
    for (const auto& line : output_log) {
        ImGui::TextUnformatted(line.c_str());
    }

    if (scroll_to_bottom) {
        ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom = false;
    }

    ImGui::EndChild();
    ImGui::Separator();

    ImGui::Text("Command:");
    if (ImGui::InputText(
            "##Input", input_buffer, sizeof(input_buffer), ImGuiInputTextFlags_EnterReturnsTrue
        )) {
        string command = input_buffer;
        if (!command.empty()) {
            command_history.push_back(command);
            history_index = command_history.size();

            output_log.push_back("> " + command);
            ipc_client_send(&g_ipc, command.c_str());
            scroll_to_bottom = true;
            std::memset(input_buffer, 0, sizeof(input_buffer));
        }

        ImGui::SetKeyboardFocusHere(-1);
    }

    if (ImGui::IsItemActive() && !command_history.empty()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && history_index > 0) {
            history_index--;
            std::strcpy(input_buffer, command_history[history_index].c_str());
        } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
                   history_index < static_cast<int>(command_history.size()) - 1) {
            history_index++;
            std::strcpy(input_buffer, command_history[history_index].c_str());
        }
    }

    ImGui::End();
}