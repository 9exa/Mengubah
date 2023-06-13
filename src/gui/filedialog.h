/**
 * @file filedialog.h
 * @author 9exa
 * @brief Opens the OS's file Dialog (A wrapper around nanogui's file_dialog  because it is broken for linux zenety)
 * @version 0.1
 * @date 2023-06-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MENGU_FILE_DIALOG
#define MENGU_FILE_DIALOG

#include <cstdint>
#include <vector>
#include <utility>
#include <string>

namespace Mengu {

// opens a file will specified extensions
std::string open_file_dialog(
    const std::vector<std::pair<std::string, std::string>> &filetypes
);

}

#endif