#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <uuid/uuid.h>

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10
#define MAX_FAQ_COUNT 100
#define MAX_QUESTION_LENGTH 256
#define MAX_ANSWER_LENGTH 512

// Structure to hold FAQ
struct FAQ {
    char question[MAX_QUESTION_LENGTH];
    char answer[MAX_ANSWER_LENGTH];
};

// Structure to hold client information
struct Client {
    int socket;
    int chatbot_active;
    char uuid[50];
};

// Function prototypes
void send_message_to_client(int sock, const char *message);
void handle_client_command(int sender_socket, struct Client *clients, int client_count, const char *command, struct FAQ *faqs, int faq_count);
void logout_client(int socket, int *client_count, struct Client *clients);

// Thread function to handle each client
void *handle_client(void *arg) {
    struct Client *client = (struct Client *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        // Receive message from client
        ssize_t bytes_received = recv(client->socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            // Client disconnected
            logout_client(client->socket, &client_count, clients);
            close(client->socket);
            pthread_exit(NULL); // Exit the thread
        }
        buffer[bytes_received] = '\0';

        // Handle client command
        handle_client_command(client->socket, clients, client_count, buffer, faqs, faq_count);
    }

    return NULL;
}

int main() {
    int server_socket, new_socket, client_count = 0;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    struct Client clients[MAX_CLIENTS];

    int server_socket, new_socket, client_sockets[MAX_CLIENTS], client_count = 0;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    struct Client clients[MAX_CLIENTS];
    
    // Read FAQs from file
    struct FAQ faqs[MAX_FAQ_COUNT];
    int faq_count = 0; // Declaration of faq_count

    FILE *file = fopen("FAQs.txt", "r");
    if (file == NULL) {
        perror("Error opening FAQ.txt");
        exit(EXIT_FAILURE);
    }

    char line[MAX_QUESTION_LENGTH + MAX_ANSWER_LENGTH + 5]; // Buffer to hold each line (question, answer, and separator)
    while (fgets(line, sizeof(line), file) != NULL && faq_count < MAX_FAQ_COUNT) {
        char *separator = strstr(line, " ||| ");
        if (separator != NULL) {
            *separator = '\0'; // Null-terminate the question
            strcpy(faqs[faq_count].question, line);
            strcpy(faqs[faq_count].answer, separator + 5); // Skip the " ||| " separator
            faq_count++;
        }
    }
    fclose(file);
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5567);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 5567...\n");

    // Accept incoming connections
    addr_size = sizeof(client_addr);
    while ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size))) {
        printf("New connection accepted.\n");

        // Add new client to array
        clients[client_count].socket = new_socket;
        clients[client_count].chatbot_active = 0; // Chatbot initially inactive

        // Generate UUID for the client
        uuid_t uuid;
        uuid_generate_random(uuid);
        uuid_unparse(uuid, clients[client_count].uuid);
        printf("Generated UUID for client: %s\n", clients[client_count].uuid);

        // Send welcome message along with UUID to the client
        char welcome_message[BUFFER_SIZE];
        snprintf(welcome_message, sizeof(welcome_message), "Welcome to the server! Your UUID is: %s\n", clients[client_count].uuid);
        send_message_to_client(new_socket, welcome_message);

        // Create a new thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, &clients[client_count]) != 0) {
            perror("pthread_create");
            close(new_socket);
        } else {
            client_count++;
        }
    }

    close(server_socket);
    return 0;
}

// Send message to client
void send_message_to_client(int sock, const char *message) {
    send(sock, message, strlen(message), 0);
}

// Handle client command
void handle_client_command(int sender_socket, struct Client *clients, int client_count, const char *command, struct FAQ *faqs, int faq_count) {
    char response[BUFFER_SIZE];

    // Check if the command is for chatbot activation or deactivation
    if (strncmp(command, "/chatbotlogin", 13) == 0) {
        // Activate chatbot for the client
        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket == sender_socket) {
                clients[i].chatbot_active = 1;
                send_message_to_client(sender_socket, "stupidbot> Hi, I am stupid bot. I am able to answer a limited set of your questions.\n");
                break;
            }
        }
    } else if (strncmp(command, "/chatbotlogout", 14) == 0) {
        // Deactivate chatbot for the client
        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket == sender_socket) {
                clients[i].chatbot_active = 0;
                send_message_to_client(sender_socket, "stupidbot> Bye! Have a nice day and do not complain about me.\n");
                break;
            }
        }
    } else {
        // Check if chatbot is active for the client
        int chatbot_active = 0;
        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket == sender_socket) {
                chatbot_active = clients[i].chatbot_active;
                break;
            }
        }

        // Handle chatbot response if active
        if (chatbot_active) {
            // Check if the message matches any FAQ
                        int faq_matched = 0;
            for (int i = 0; i < faq_count; i++) {
                if (strcmp(command, faqs[i].question) == 0) {
                    // Send the corresponding answer to the client
                    send_message_to_client(sender_socket, faqs[i].answer);
                    faq_matched = 1;
                    break;
                }
            }

            // If no FAQ matched, send default response
            if (!faq_matched) {
                send_message_to_client(sender_socket, "stupidbot> System Malfunction, I couldn't understand your query.\n");
            }
        } else {
            // Chatbot is not active, handle regular chat messages
            send_message_to_client(sender_socket, "user> Invalid command. Please log in to chatbot using '/chatbotlogin'.\n");
        }
    }
}

// Logout client
void logout_client(int socket, int *client_count, struct Client *clients) {
    int i = 0;
    while (i < *client_count && clients[i].socket != socket) {
        i++;
    }
    if (i < *client_count) {
        // Remove client from array
        for (; i < *client_count - 1; i++) {
            clients[i] = clients[i + 1];
        }
        (*client_count)--;
    }
}

