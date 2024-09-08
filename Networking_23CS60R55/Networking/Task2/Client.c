#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define BUFFER_SIZE 4096

void send_command(int sock, const char *cmd) {
    send(sock, cmd, strlen(cmd), 0);
}

void receive_and_print(int sock) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    while ((bytes_received = recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT)) > 0) {
        printf("%.*s", (int)bytes_received, buffer);
    }

    if (bytes_received == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
        perror("recv error");
        exit(1);
    }
}

int main() {
    char *ip = "127.0.0.1";
    int port = 5567;
    int sock;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    int chatbot_active =0;

    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket error");
        exit(1);
    }

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect error");
        exit(1);
    }

    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server: %s\n", buffer);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sock;
    fds[1].events = POLLIN;

    while(1)
    {
        // Wait for activity on either stdin or socket
        int ret = poll(fds, 2, -1);
        if (ret == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            // Read input from stdin
            // printf("Enter command: \n");
            fflush(stdout);
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                // Remove trailing newline character
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len - 1] == '\n') {
                    buffer[len - 1] = '\0';
                }

                // Check for "/send" command
                if (strncmp(buffer, "/send", 5) == 0) {
                    // Send the message to the server
                    char id[128], msg[BUFFER_SIZE - 6]; // Assuming ID can be up to 128 characters
                    if (sscanf(buffer, "%*s %127s %[^\n]", id, msg) == 2) {
                        // Send the "/send" command with ID and message
                        send_command(sock, buffer);
                    } else {
                        printf("Invalid format for /send command\n");
                    }
                }
                // Check for "/logout" command
                else if (strncmp(buffer, "/logout", 7) == 0) {
                    // Send the logout command to the server
                    send_command(sock, buffer);
                    bzero(buffer, sizeof(buffer));
                    // Receive acknowledgment from server
                    recv(sock, buffer, BUFFER_SIZE, 0);
                    printf("%s ",buffer);
                    break; // Exit the main loop
                }
                // Check for "/active" command
                else if (strncmp(buffer, "/active", 7) == 0) {
                    // Send the active command to the server
                    send_command(sock, buffer);
                }
                // Check for chatbot login command
                else if (strncmp(buffer, "/chatbotlogin", 13) == 0) {
                    chatbot_active = 1; // Enable chatbot
                    printf("stupidbot> Hi, I am stupid bot. I am able to answer a limited set of your questions.\n");
                }
                // Check for chatbot logout command
                else if (strncmp(buffer, "/chatbotlogout", 14) == 0) {
                    chatbot_active = 0; // Disable chatbot
                    printf("stupidbot> Bye! Have a nice day and do not complain about me.\n");
                }
                // Check for regular commands
                else {
                    if (chatbot_active || strncmp(buffer, "/logout", 7) == 0 || strncmp(buffer, "/active", 7) == 0) {
                        send_command(sock, buffer); // Send the message to the server
                    } else {
                        printf("user> Invalid command. Available commands: /chatbotlogin, /chatbotlogout, /send, /logout, /active\n");
                    }
                }
                // Invalid command
                // else {
                //     printf("Invalid command. Available commands: /send, /logout, /active\n");
                // }
            }
        }

        if (fds[1].revents & POLLIN) {
            // Receive and print messages from the server
            bzero(buffer, sizeof(buffer));
            receive_and_print(sock);
        }
    }

    // close(sock);
    return 0;
}