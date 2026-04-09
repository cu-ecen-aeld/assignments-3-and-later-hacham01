#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

static volatile sig_atomic_t caught_signal = 0;

static void signal_handler(int signo)
{
    caught_signal = signo;
}


int main(int argc, char *argv[])
{
    openlog("aesdsocket", LOG_PID | LOG_PERROR, LOG_USER);

    int daemon_mode = 0;
    int opt;

    while ((opt = getopt(argc, argv, "d")) != -1) {
        if (opt == 'd') {
            daemon_mode = 1;
        } else {
            return -1;
        }
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Failed to register signal handler");
        closelog();
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
    	syslog(LOG_ERR, "Failed to create socket");
        closelog();
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        closelog();
        return -1;
    }
	
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid == -1) {
            syslog(LOG_ERR, "Failed to fork: %s (%d)", strerror(errno), errno);
            close(sockfd);
            closelog();
            return -1;
        }
	
        if (pid > 0) {
            close(sockfd);
            closelog();
            return 0;
        }

        if (setsid() < 0) {
            return -1;
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
    
    if (listen(sockfd, 10) == -1) {
    	syslog(LOG_ERR, "Failed to listed socket: listres == -1");
	closelog();
	return -1;
    }
    
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
    	    if (caught_signal) {
                break;
            }
            syslog(LOG_ERR, "Failed to recive client connection");
            close(sockfd);
	        closelog();
            return -1;
        }
    
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
    
        char *client_ip = inet_ntoa(client_addr.sin_addr);
        char *received_buffer = malloc(sizeof(char) * BUFFER_SIZE);
        char *packet = NULL;
        size_t packet_size = 0;
        int received_data_size;       
	
        while ((received_data_size = recv(client_fd, received_buffer, BUFFER_SIZE, 0)) > 0) {
            packet = realloc(packet, packet_size + received_data_size);
            if (packet == NULL) {
                syslog(LOG_ERR, "Failed to allocate memory");
                free(received_buffer);
                close(client_fd);
                close(sockfd);
                closelog();
                return -1;
            }
            memcpy(packet + packet_size, received_buffer, received_data_size);
            packet_size += received_data_size;

            char *newline;
            while ((newline = memchr(packet, '\n', packet_size)) != NULL) {
                size_t line_len = newline - packet + 1; 
                int fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd == -1) {
                    syslog(LOG_ERR, "Failed to open file '%s': %s (%d)", FILE_PATH, strerror(errno), errno);
                    free(packet);
                    free(received_buffer);
                    close(client_fd);
                    close(sockfd);
                    closelog();
                    return -1;
                }

                ssize_t wdata_len = write(fd, packet, line_len);
                if (wdata_len != (ssize_t)line_len) {
                    syslog(LOG_ERR, "Failed to write received data");
                    free(packet);
                    free(received_buffer);
                    close(fd);
                    close(client_fd);
                    close(sockfd);
                    closelog();
                    return -1;
                }

                close(fd);

                size_t remaining = packet_size - line_len;
                memmove(packet, packet + line_len, remaining);
                packet_size = remaining;

                fd = open(FILE_PATH, O_RDONLY);
                if (fd == -1) {
		            syslog(LOG_ERR, "Failed to open file for read '%s': %s (%d)", FILE_PATH, strerror(errno), errno);
		            free(received_buffer);
                    close(fd);
                    close(client_fd);
                    close(sockfd);
                    closelog();
                    return -1;
                }

                int rdata_len;
                while ((rdata_len = read(fd, received_buffer, BUFFER_SIZE)) > 0) {
                    int send_len = send(client_fd, received_buffer, rdata_len, 0);
                    if (send_len != rdata_len) {
                        syslog(LOG_ERR, "Failed to send file data '%s': %s (%d)", FILE_PATH, strerror(errno), errno);
                        free(received_buffer);
                        close(fd);
			free(packet);
                        close(client_fd);
                        close(sockfd);
                        closelog();
                        return -1;
                    }
                }
                
                close(fd);
            }
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);

        free(packet);
        free(received_buffer);
        close(client_fd);
    }

    if (caught_signal) {
        syslog(LOG_INFO, "Caught signal, exiting");
        printf("Caught signal, exiting\n");
    }

    close(sockfd);
    remove(FILE_PATH);
    closelog();
    return 0;
}
