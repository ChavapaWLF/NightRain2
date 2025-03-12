// 通过这个头文件引入替代filesystem的函数

#ifndef FILESYSTEM_COMPAT_H
#define FILESYSTEM_COMPAT_H

#include <string>
#include <fstream>
#include <direct.h> // 对于Windows的_mkdir
#include <sys/stat.h>

// 替代std::filesystem::exists
inline bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 替代std::filesystem::create_directory
inline bool create_directory(const std::string& path) {
    return (_mkdir(path.c_str()) == 0);
}

// 替代std::filesystem::create_directories
inline bool create_directories(const std::string& path) {
    // 创建每一级目录
    std::string current_path;
    for (char c : path) {
        current_path += c;
        if (c == '/' || c == '\\') {
            // 尝试创建目录（如果已存在则忽略错误）
            if (!file_exists(current_path) && !create_directory(current_path)) {
                return false;
            }
        }
    }
    
    // 创建最终目录（如果路径不以斜杠结尾）
    if (!path.empty() && path.back() != '/' && path.back() != '\\') {
        if (!file_exists(path) && !create_directory(path)) {
            return false;
        }
    }
    
    return true;
}

// 替代std::filesystem::path的parent_path()
inline std::string parent_path(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return "";
}

#endif // FILESYSTEM_COMPAT_H