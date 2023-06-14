#include "gui/filedialog.h"

#include <nanogui/common.h>
#include <cstring>

#ifdef __linux__
std::string Mengu::open_file_dialog(const std::vector<std::pair<std::string, std::string>> &filetypes) {
    char filename[1024] = {'\0'};
    
    std::string filedialog_open_command =  "zenity --file-selection ";
    filedialog_open_command.append(" --file-filter=\'Supported Files |");
    for (auto filetype: filetypes) {
        filedialog_open_command.append(" *.");
        filedialog_open_command.append(filetype.first);
    }
    filedialog_open_command.append("\'");

    for (auto filetype: filetypes) {
        filedialog_open_command.append(" --file-filter=\'");
        filedialog_open_command.append(filetype.second);
        filedialog_open_command.append(" | *.");
        filedialog_open_command.append(filetype.first);
        filedialog_open_command.append("\'");
    }

    FILE *f = popen(filedialog_open_command.data(), "r");
    if (fgets(filename, 1024, f) != nullptr) {
        char *newline_at = strrchr(filename, '\n');
        if (newline_at != nullptr) {
            *newline_at = '\0'; // remove the newline character at the end
        }
    }
    pclose(f);

    return std::string(filename);
}

#else
std::string Mengu::open_file_dialog(const std::vector<std::pair<std::string, std::string>> &filetypes) {
    return nanogui::file_dialog(filetypes, false);
}

#endif