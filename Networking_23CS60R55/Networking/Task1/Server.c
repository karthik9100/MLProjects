#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <sys/time.h>
#include <sys/select.h>
 
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
 
struct client_data {
    char uuid[50];
    int sock;
    int status;
};
 
void get_clients(struct client_data dic[], int client_count, char *arr) {
    strcpy(arr, ""); // Initialize arr as an empty string
    for (int i = 0; i < client_count; i++) {
        if (dic[i].status == 1) {
            strcat(arr, dic[i].uuid); // Concatenate UUID to arr
            strcat(arr, "\n");
        }
    }
}
 
void logout_client(int sock, int client_count, struct client_data dic[]) {
    printf("Logging out %d\n", sock);
    char goodbye_msg[1024];
    for (int i = 0; i < client_count; i++) {
        if (dic[i].sock == sock) {
            dic[i].status = 0;
            printf("Client %s logged out.\n", dic[i].uuid);
            snprintf(goodbye_msg, sizeof(goodbye_msg), "Bye!!! Have a nice day");
            send(sock, goodbye_msg, strlen(goodbye_msg), 0); // Send goodbye message
            break; // Break after finding the client to logout
        }
    }
}
 
void send_welcome_msg(int client_sock, const char *uuid, int client_count) {
    char welcome_msg[1024];
    snprintf(welcome_msg, sizeof(welcome_msg), "Welcome! Your UUID is: %s\n ", uuid);
    send(client_sock, welcome_msg, strlen(welcome_msg), 0);
}
 
void genUUID(char new_uuid[50]) {
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse(uuid, new_uuid);
    printf("Generated UUID: %s\n", new_uuid);
}
 
void send_message_to_client(int client_sock, const char *message) {
    printf("Sending msg to %d\n",client_sock);
    send(client_sock, message, strlen(message), 0);
}
 
void handle_chat_message(struct client_data dic[], int sender_sock, int client_count, char *buffer) {
    char dest_id[50];
    char message[BUFFER_SIZE - 7]; // Assuming maximum message length
    if (sscanf(buffer, "/send %s %[^\n]", dest_id, message) == 2) {
        int found = 0;
        for (int i = 0; i < client_count; i++) {
            if (strcmp(dic[i].uuid, dest_id) == 0 && dic[i].status == 1) {
                char full_message[4096];
                snprintf(full_message, sizeof(full_message), "Message from %s: %s\n", dic[sender_sock].uuid, message);
                send_message_to_client(dic[i].sock, full_message);
                found = 1;
                break;
            }
        }
        if (!found) {
            send_message_to_client(sender_sock, "Recipient not found or offline.\n");
        }
    } else {
        send_message_to_client(sender_sock, "Invalid format for /send command.\n");
    }
}
 
int main() {
    struct client_data dic[MAX_CLIENTS];
    int client_count = 0;
    char *ip = "127.0.0.1";
    int port = 5567;
    int server_sock, client_sock, max_sd;
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds;
    int activity, i, sd;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
 
    // Initialize client data array
    memset(dic, 0, sizeof(dic));
 
    // Create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket error");
        exit(1);
    }
 
    // Set server address
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
 
    // Bind server socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
        exit(1);
    }
 
    // Listen for incoming connections
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen error");
        exit(1);
    }
 
    printf("Listening for clients...\n");
 
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        max_sd = server_sock;
 
        // Add client sockets to the readfds set
        for (i = 0; i < client_count; i++) {
            sd = dic[i].sock;
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
 
        // Wait for activity on sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }
 
        // Handle new client connections
        if (FD_ISSET(server_sock, &readfds)) {
            addr_size = sizeof(client_addr);
            client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
            if (client_sock < 0) {
                perror("Accept error");
                continue;
            }
            printf("New client connected.\n");
            genUUID(dic[client_count].uuid);
            dic[client_count].sock = client_sock;
            send_welcome_msg(client_sock, dic[client_count].uuid, client_count);
            dic[client_count].status = 1;
            client_count++;
        }
 
        // Handle activity on client sockets
        for (i = 0; i < client_count; i++) {
                        sd = dic[i].sock;
            if (FD_ISSET(sd, &readfds)) {
                bzero(buffer, sizeof(buffer));
                ssize_t bytes_received = recv(sd, buffer, sizeof(buffer), 0);
 
                if (bytes_received <= 0) {
                    if (bytes_received == 0) {
                        printf("Client disconnected: %s\n", dic[i].uuid);
                    } else {
                        perror("Receive error");
                    }
                    close(sd);
                    // Remove the client from the array
                    for (int j = i; j < client_count - 1; j++) {
                        dic[j] = dic[j + 1];
                    }
                    client_count--;
                    continue;
                }
 
                if (strcmp(buffer, "/active") == 0) {
                    // Collect active clients' UUIDs
                    char arr[MAX_CLIENTS * 50];
                    get_clients(dic, client_count, arr);
                    send_message_to_client(sd, arr);
                } else if (strncmp(buffer, "/send", 5) == 0) {
                    handle_chat_message(dic, i, client_count, buffer);
                } else if (strncmp(buffer, "/logout", 7) == 0) {
                    logout_client(sd, client_count, dic);
                    continue;
                } else {
                    send_message_to_client(sd, "Invalid command. Available commands: /active, /send, /logout\n");
                }
            }
        }
    }
 
    close(server_sock);
    return 0;
}