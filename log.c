#include "log.h"

// Function to log a command in the specified format
void log_command(const char *ip, int client_port, int storage_server_port, const char *command, const char *status) {
    FILE *log_file;
    const char *file_path = "logfile.txt"; // Change this to your desired log file path

    // Open the log file in append mode
    log_file = fopen(file_path, "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    time_t current_time;
    char timestamp[20];

    // Get current timestamp
    time(&current_time);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&current_time));

    // Write log entry to the file
    fprintf(log_file, "\n%s %s %d %d %s log.txt %s\n", timestamp, ip, client_port, storage_server_port, command, status);

    // Close the log file
    fclose(log_file);
}
