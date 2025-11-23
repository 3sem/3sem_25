#include "Check_r_w_flags.h"

void init_file_types(file_types* files) {
    if (!files) return;
    
    memset(files, 0, sizeof(file_types));
    
    files->read_files = calloc(MAX_READ_FILES, sizeof(char*));
    files->write_files = calloc(MAX_WRITE_FILES, sizeof(char*));
    
    files->read_files_count = 0;
    files->write_files_count = 0;
    files->check_status = CHECK_NONE;
    files->has_errors = false;
    files->error_message[0] = '\0';
}

void cleanup_file_types(file_types* files) {
    if (!files) return;
    
    for (int i = 0; i < files->read_files_count; i++) {
        if (files->read_files[i]) {
            free(files->read_files[i]);
        }
    }
    
    for (int i = 0; i < files->write_files_count; i++) {
        if (files->write_files[i]) {
            free(files->write_files[i]);
        }
    }
    
    if (files->read_files) {
        free(files->read_files);
        files->read_files = NULL;
    }
    
    if (files->write_files) {
        free(files->write_files);
        files->write_files = NULL;
    }
    
    files->read_files_count = 0;
    files->write_files_count = 0;
    files->check_status = CHECK_NONE;
}

bool validate_filename(const char* filename) {
    if (!filename || strlen(filename) == 0) {
        return false;
    }
    
    if (strlen(filename) >= MAX_FILENAME_LENGTH) {
        return false;
    }
    
    const char* forbidden_chars = "\"<>|*?\\";
    for (const char* c = forbidden_chars; *c; c++) {
        if (strchr(filename, *c) != NULL) {
            return false;
        }
    }
    
    return true;
}

bool add_read_file(file_types* files, const char* filename) {
    if (!files || !filename || files->read_files_count >= MAX_READ_FILES) {
        return false;
    }
    
    if (!validate_filename(filename)) {
        snprintf(files->error_message, sizeof(files->error_message),
                 "Invalid read filename: %s", filename);
        files->has_errors = true;
        return false;
    }
    
    files->read_files[files->read_files_count] = strdup(filename);
    if (!files->read_files[files->read_files_count]) {
        snprintf(files->error_message, sizeof(files->error_message),
                 "Memory allocation failed for: %s", filename);
        files->has_errors = true;
        return false;
    }
    
    files->read_files_count++;
    files->check_status |= CHECK_R;
    return true;
}

bool add_write_file(file_types* files, const char* filename) {
    if (!files || !filename || files->write_files_count >= MAX_WRITE_FILES) {
        return false;
    }
    
    if (!validate_filename(filename)) {
        snprintf(files->error_message, sizeof(files->error_message),
                 "Invalid write filename: %s", filename);
        files->has_errors = true;
        return false;
    }
    
    files->write_files[files->write_files_count] = strdup(filename);
    if (!files->write_files[files->write_files_count]) {
        snprintf(files->error_message, sizeof(files->error_message),
                 "Memory allocation failed for: %s", filename);
        files->has_errors = true;
        return false;
    }
    
    files->write_files_count++;
    files->check_status |= CHECK_W;
    return true;
}

bool check_r_w_flags(int check_option, char** argv, int argc, file_types* files) {
    if (!files || !argv || argc <= 0) return false;
    
    files->has_errors = false;
    files->error_message[0] = '\0';
    
    bool rf_flag_found = false;
    bool wf_flag_found = false;
    
    for (int i = 0; i < argc; i++) {
        if (!argv[i]) continue;
        
        if (strncmp(argv[i], "-rf", 3) == 0 || strcmp(argv[i], "-r") == 0) {
            if (i + 1 >= argc || !argv[i + 1]) {
                snprintf(files->error_message, sizeof(files->error_message),
                         "Missing filename after flag: %s", argv[i]);
                files->has_errors = true;
                continue;
            }
            
            if (add_read_file(files, argv[i + 1])) {
                rf_flag_found = true;
                i++;
            }
        }
        
        else if (strncmp(argv[i], "-wf", 3) == 0 || strcmp(argv[i], "-w") == 0) {
            if (i + 1 >= argc || !argv[i + 1]) {
                snprintf(files->error_message, sizeof(files->error_message),
                         "Missing filename after flag: %s", argv[i]);
                files->has_errors = true;
                continue;
            }
            
            if (add_write_file(files, argv[i + 1])) {
                wf_flag_found = true;
                i++;
            }
        }
    }
    
    bool flags_check_success = true;
    
    if (check_option & CHECK_R) {
        if (!rf_flag_found) {
            flags_check_success = false;
            if (!files->has_errors) {
                snprintf(files->error_message, sizeof(files->error_message),
                         "Required read flag (-r/-rf) not found");
            }
        }
    }
    
    if (check_option & CHECK_W) {
        if (!wf_flag_found) {
            flags_check_success = false;
            if (!files->has_errors) {
                snprintf(files->error_message, sizeof(files->error_message),
                         "Required write flag (-w/-wf) not found");
            }
        }
    }
    
    if ((check_option & OPTIONAL_CHECK) && !files->has_errors) {
        flags_check_success = true;
    }
    
    return flags_check_success && !files->has_errors;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -r <file>      Request file from server and print to stdout\n");
    printf("  -rf<index> <file> Request file with index (e.g., -rf0 file1.txt)\n");
    printf("  -w <file>      Redirect stdout to file\n");
    printf("  -wf<index> <file> Redirect stdout with index\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -r file1.txt                    # Request one file\n", program_name);
    printf("  %s -rf0 file1.txt -rf1 file2.txt   # Request multiple files\n", program_name);
    printf("  %s -r input.txt -w output.txt      # Request file and save to output\n", program_name);
    printf("  %s -w log.txt                      # Just redirect stdout\n", program_name);
    printf("\n");
    printf("Note: Server must be running before starting client.\n");
}
