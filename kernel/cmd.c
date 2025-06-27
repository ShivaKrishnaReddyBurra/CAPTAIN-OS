#include "utils.h"

void normalize_path(const char *path, char *result, char *current_directory, int max_input) {
    if (!path || !result) return;
    
    if (path[0] != '/') {
        // Relative path - combine with current directory
        string_copy(result, current_directory, max_input);
        if (result[string_length(result) - 1] != '/' && path[0] != '\0') {
            int len = string_length(result);
            if (len < 255) {
                result[len] = '/';
                result[len + 1] = '\0';
            }
        }
        
        // Append relative path
        int result_len = string_length(result);
        int path_len = string_length(path);
        if (result_len + path_len < 255) {
            for (int i = 0; i < path_len; i++) {
                result[result_len + i] = path[i];
            }
            result[result_len + path_len] = '\0';
        }
    } else {
        string_copy(result, path, max_input);
    }
    
    // Remove trailing slash unless it's root
    int len = string_length(result);
    if (len > 1 && result[len - 1] == '/') {
        result[len - 1] = '\0';
    }
}


