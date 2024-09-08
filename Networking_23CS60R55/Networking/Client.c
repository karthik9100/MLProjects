
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include<ctype.h>

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
    printf("\nClient : ");
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
    int uuid;
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
    bzero(buffer, sizeof(buffer));
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server: %s\n", buffer);

    

    // Variables for chatbot feature
    int chatbot_active = 0; // 0 for inactive, 1 for active
    const char *chatbot_login_msg = "stupidbot> Hi, I am stupid bot. I am able to answer a limited set of your questions\n";
    const char *chatbot_prompt = "stupidbot> ";

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sock;
    fds[1].events = POLLIN;

    while (1) {
        // Wait for activity on either stdin or socket
        int ret = poll(fds, 2, -1);
        if (ret == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        
        if (fds[0].revents & POLLIN) {
            // Read input from stdin
            
            fflush(stdout);
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                // Remove trailing newline character
                // size_t len = strlen(buffer);
                // if (len > 0 && buffer[len - 1] == '\n') {
                //     buffer[len - 1] = '\0';
                // }
                 if (strncmp(buffer, "/chatbot login", 14) == 0) {
                    send_command(sock, buffer);
                    chatbot_active = 1; // Activate chatbot
                } else if (chatbot_active == 1 && strncmp(buffer, "/chatbot logout", 15) == 0) {
                    send_command(sock, buffer);
                    chatbot_active = 0; // Deactivate chatbot
                }
                // Check for chatbot activation/deactivation commands
                else if (strncmp(buffer, "/history", 8) == 0) {
                    // Request chat history
                    //  size_t len = strlen(buffer);
                    // if (len > 0 && buffer[len - 1] == '\n') {
                    //     buffer[len - 1] = '\0';
                    // }
                    //  snprintf(buffer + strlen(buffer), BUFFER_SIZE - strlen(buffer), " %d", uuid);
                    send_command(sock, buffer);
                } else if (strncmp(buffer, "/history_delete", 15) == 0) {
                    // Delete chat history of a specific recipient
                    send_command(sock, buffer);
                } else if (strncmp(buffer, "/delete_all", 11) == 0) {
                    // Delete complete chat history
                    send_command(sock, buffer);
                }
                else if (strncmp(buffer, "/logout", 7) == 0) {
                    // Send the logout command to the server
                   send_command(sock, buffer);
                    bzero(buffer, sizeof(buffer));
                    // Receive acknowledgment from server
                    recv(sock, buffer, BUFFER_SIZE, 0);
                    printf("%s ", buffer);
                    exit(1); // Exit the main loop
                }
                 else {
                     size_t len = strlen(buffer);
                    if (len > 0 && buffer[len - 1] == '\n') {
                        buffer[len - 1] = '\0';
                    }
                    // Send the command to the server
                    send_command(sock, buffer);
                }

            }
        }

        if (fds[1].revents & POLLIN) {
            // Receive and print messages from the server
            receive_and_print(sock);
            if (chatbot_active) {
                printf("\n%s", chatbot_prompt); // Display chatbot prompt
            }
          
        }
        
            
    }

    // close(sock);
    return 0;
}