#include "root.h"

static vector<string> console_output;
static char command_input[512] = "";
static bool auto_scroll = true;
static bool show_timestamps = true;
static string connection_status = "Disconnected";

static vector<string> command_history;
static int history_pos = -1;

struct MemoryStats {
    size_t total_allocations = 0;
    size_t total_deallocations = 0;
    size_t active_allocations = 0;
    size_t bytes_allocated = 0;
    size_t bytes_leaked = 0;
    int leak_count = 0;
};

static MemoryStats current_stats;

static void add_console_line(const string& line) {
    string timestamped_line;
    if (show_timestamps) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", std::localtime(&time_t));
        timestamped_line = "[" + std::string(timestamp) + "." +
                           std::to_string(ms.count()).substr(0, 3) + "] " + line;
    } else {
        timestamped_line = line;
    }

    console_output.push_back(timestamped_line);

    if (console_output.size() > 1000) {
        console_output.erase(console_output.begin());
    }
}

static void send_command(const std::string& cmd) {
    if (!cmd.empty()) {
        if (command_history.empty() || command_history.back() != cmd) {
            command_history.push_back(cmd);
            if (command_history.size() > 50) {
                command_history.erase(command_history.begin());
            }
        }
        history_pos = -1;

        if (ipc_client_send(&g_ipc, cmd)) {
            add_console_line("> " + cmd);
        } else {
            add_console_line("ERROR: Failed to send command");
        }
    }
}

static void parse_stats_response(const std::string& response) {
    if (response.find("STATS:") == 0) {
        std::istringstream iss(response.substr(6));
        std::string token;
        while (std::getline(iss, token, ' ')) {
            if (token.find("allocs=") == 0) {
                current_stats.total_allocations = std::stoull(token.substr(7));
            } else if (token.find("deallocs=") == 0) {
                current_stats.total_deallocations = std::stoull(token.substr(9));
            } else if (token.find("active=") == 0) {
                current_stats.active_allocations = std::stoull(token.substr(7));
            } else if (token.find("bytes=") == 0) {
                current_stats.bytes_allocated = std::stoull(token.substr(6));
            } else if (token.find("leaked=") == 0) {
                current_stats.bytes_leaked = std::stoull(token.substr(7));
            } else if (token.find("leaks=") == 0) {
                current_stats.leak_count = std::stoi(token.substr(6));
            }
        }
    }
}

void mole_ui_root() {
    if (g_ipc.pipe != INVALID_HANDLE_VALUE && g_ipc.running) {
        connection_status = "Connected";
    } else {
        connection_status = "Disconnected";
    }

    string msg;
    while (ipc_client_poll(&g_ipc, msg)) {
        parse_stats_response(msg);
        add_console_line(msg);
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io->DisplaySize);
    ImGuiWindowFlags main_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_MenuBar;

    ImGui::Begin("Memory Leak Detection Tool", nullptr, main_flags);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Connection")) {
            if (ImGui::MenuItem("Connect", nullptr, false, connection_status == "Disconnected")) {
                if (ipc_client_connect(&g_ipc)) {
                    add_console_line("Connected to memory leak detection tool");
                    send_command("status");
                } else {
                    add_console_line("ERROR: Failed to connect to tool");
                }
            }
            if (ImGui::MenuItem("Disconnect", nullptr, false, connection_status == "Connected")) {
                ipc_client_disconnect(&g_ipc);
                add_console_line("Disconnected from tool");
            }
            ImGui::Separator();
            ImGui::Text("Status: %s", connection_status.c_str());
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Auto-scroll", &auto_scroll);
            ImGui::Checkbox("Show timestamps", &show_timestamps);
            if (ImGui::MenuItem("Clear console")) {
                console_output.clear();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Commands")) {
            if (ImGui::MenuItem("Get Status", nullptr, false, connection_status == "Connected")) {
                send_command("status");
            }
            if (ImGui::MenuItem(
                    "Start Monitoring", nullptr, false, connection_status == "Connected"
                )) {
                send_command("start");
            }
            if (ImGui::MenuItem(
                    "Stop Monitoring", nullptr, false, connection_status == "Connected"
                )) {
                send_command("stop");
            }
            if (ImGui::MenuItem("Show Leaks", nullptr, false, connection_status == "Connected")) {
                send_command("leaks");
            }
            if (ImGui::MenuItem(
                    "Generate Report", nullptr, false, connection_status == "Connected"
                )) {
                send_command("report");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if (ImGui::CollapsingHeader("Memory Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "stats_columns", false);

        ImGui::Text("Total Allocations:");
        ImGui::NextColumn();
        ImGui::Text("%zu", current_stats.total_allocations);
        ImGui::NextColumn();

        ImGui::Text("Total Deallocations:");
        ImGui::NextColumn();
        ImGui::Text("%zu", current_stats.total_deallocations);
        ImGui::NextColumn();

        ImGui::Text("Active Allocations:");
        ImGui::NextColumn();
        ImGui::Text("%zu", current_stats.active_allocations);
        ImGui::NextColumn();

        ImGui::Text("Bytes Allocated:");
        ImGui::NextColumn();
        ImGui::Text("%.2f KB", current_stats.bytes_allocated / 1024.0);
        ImGui::NextColumn();

        ImGui::Text("Bytes Leaked:");
        ImGui::NextColumn();
        if (current_stats.bytes_leaked > 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Red
            ImGui::Text("%.2f KB", current_stats.bytes_leaked / 1024.0);
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));  // Green
            ImGui::Text("0 KB");
            ImGui::PopStyleColor();
        }
        ImGui::NextColumn();

        ImGui::Text("Leak Count:");
        ImGui::NextColumn();
        if (current_stats.leak_count > 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Red
            ImGui::Text("%d", current_stats.leak_count);
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));  // Green
            ImGui::Text("0");
            ImGui::PopStyleColor();
        }
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    ImGui::Separator();
    ImGui::Text("Console Output");
    ImGui::SameLine();
    ImGui::PushStyleColor(
        ImGuiCol_Text, connection_status == "Connected" ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
                                                        : ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
    );
    ImGui::Text("(%s)", connection_status.c_str());
    ImGui::PopStyleColor();

    ImGui::BeginChild("console", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true);

    for (const auto& line : console_output) {
        // Color code different types of messages
        if (line.find("ERROR") != std::string::npos || line.find("LEAK") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        } else if (line.find("WARNING") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
        } else if (line.find(">") == line.find_first_not_of(" \t") ||
                   (show_timestamps && line.find(">") != std::string::npos)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 1.0f, 1.0f));  // Command input
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        }

        ImGui::TextUnformatted(line.c_str());
        ImGui::PopStyleColor();
    }

    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("Command:");
    ImGui::SameLine();

    ImGui::PushItemWidth(-80);
    bool enter_pressed = ImGui::InputText(
        "##command", command_input, sizeof(command_input),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
        [](ImGuiInputTextCallbackData* data) -> int {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
                if (data->EventKey == ImGuiKey_UpArrow) {
                    if (history_pos < (int)command_history.size() - 1) {
                        history_pos++;
                        strcpy_s(
                            data->Buf, data->BufSize,
                            command_history[command_history.size() - 1 - history_pos].c_str()
                        );
                        data->BufTextLen = (int)strlen(data->Buf);
                        data->BufDirty = true;
                    }
                } else if (data->EventKey == ImGuiKey_DownArrow) {
                    if (history_pos > 0) {
                        history_pos--;
                        strcpy_s(
                            data->Buf, data->BufSize,
                            command_history[command_history.size() - 1 - history_pos].c_str()
                        );
                        data->BufTextLen = (int)strlen(data->Buf);
                        data->BufDirty = true;
                    } else if (history_pos == 0) {
                        history_pos = -1;
                        data->Buf[0] = 0;
                        data->BufTextLen = 0;
                        data->BufDirty = true;
                    }
                }
            }
            return 0;
        }
    );
    ImGui::PopItemWidth();

    ImGui::SameLine();
    bool send_clicked = ImGui::Button("Send");

    if (enter_pressed || send_clicked) {
        if (connection_status == "Connected") {
            send_command(std::string(command_input));
            command_input[0] = '\0';
        } else {
            add_console_line("ERROR: Not connected to tool");
        }
    }

    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive()) {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}