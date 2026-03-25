#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // Initialize syslog
    openlog(NULL, LOG_PID, LOG_USER);

    // check number of arguments
    if(argc != 3){
        syslog(LOG_ERR, "Error: 2 arguments required, but %d provided", argc - 1);
        closelog();

        return 1;
    }

    const char *write_file = argv[1];
    const char *write_str = argv[2];

    // Open the file for writing
    FILE *fp = fopen(write_file, "w");
    if (fp == NULL) {
        syslog(LOG_ERR, "Error: Cannot open file %s for writing", write_file);
        closelog();

        return 1;
    }

    // write the string to the file
    if (fwrite(write_str, sizeof(char), strlen(write_str), fp) < strlen(write_str)) {
        syslog(LOG_ERR, "Error: Failed to write to file %s", write_file);
        fclose(fp);
        closelog();

        return 1;
    }

    // Log success and close file
    syslog(LOG_DEBUG, "Writing %s to %s", write_str, write_file);

    fclose(fp);
    closelog();

    return 0;
}