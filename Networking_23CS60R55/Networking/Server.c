#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_MSG_LEN 10000
#define CLIENT_DATA 10
#define MAX_VALUE 1024
#define TEMPDATA 1024


void genUUID() {
    char new_uuid[50];
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse(uuid, new_uuid);
    // printf("Generated UUID: %s\n", new_uuid);
}

char *ip = "127.0.0.1";
int port = 5567;
int client_sockets[MAX_CLIENTS];
char logout_msg[50] = "Bye!! Have a nice day";
char chatbot_login_msg[2048] = "Hi, I am a stupid bot. I am able to answer a limited set of your questions";
char chatbot_logout_msg[2048] = "Bye! Have a nice day and do not complain about me";
char not_found[102] = "System Malfunction, I couldn't understand your query.";

 struct Client{
    int id, uuid;
    int sockId;
    int chatboat_login;
    struct sockaddr_in addr;
};

struct Chat{
    int receiver_id, sender_id;
    char type[10];
    char message[1024];
};

struct Client active_clients[MAX_CLIENTS];
struct Chat chat_history[1024];
int num_clients = 0;
int uuid = 2340;
int clients_index = 0;

void writeIntoLogFile(char *, int , int);

void writeIntoLogFile( char* message, int sener_id, int receiver_id) {
    FILE *fp = fopen("log.txt","a");
    char out[1024];
    fprintf(fp, "%d - %d - send -%s\n", sener_id, receiver_id, message);
    fprintf(fp, "%d - %d - receive - %s\n", sener_id, receiver_id, message);
    fclose(fp);
}


void send_message(int sender_uuid, int sender_sock_id, int dest_uuid, char *message) {
    int dest_sock_id = -1;
    int socket_id = sender_sock_id;
    char temp[MAX_MSG_LEN];
    for (int i = 0; i < 10; i++) {
        if (active_clients[i].id == dest_uuid) {
            dest_sock_id = active_clients[i].sockId;
            sprintf(temp, "%d: %s", sender_uuid, message );
            socket_id = dest_sock_id;
            break;
        }
    }

    for (int i = 0; i < clients_index; i++) {
        if (active_clients[i].id == dest_uuid) {
            dest_sock_id = 0;
            sprintf(temp, "Client %d is offline. Please try again later.", dest_uuid);
            socket_id = sender_sock_id;
            break;
        }
    }

    if (dest_sock_id == -1) 
        strcpy(temp, "Invalid Destination ID");
    send(socket_id, temp, strlen(temp), 0);
    if (dest_sock_id > 0) 
        writeIntoLogFile(message, sender_uuid, dest_uuid);
}

void send_active_clients(int);
void remove_client(int);
void checkIO( char *, int);
void checkFAQ(char *query, int id, int socket_id) {

    printf("FAQ query: %s\n", query);
    FILE *file = fopen("FAQs.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        
        char* question = strtok(buffer, "|||");
        char* answer = strtok(NULL, "|||");
        if (answer != NULL) {
            answer[strcspn(answer, "\n")] = '\0';
        }

        if (strncmp(query, question, strlen(question)-1) == 0) {
            fclose(file); 
            send(socket_id, answer, strlen(answer), 0);
            return; 
        }
    }
    
    char no_query[] = "System Malfunction, I couldn't understand your query.";
    send(socket_id, no_query, strlen(no_query), 0);
    
    fclose(file);
}



void get_history(int sender_id, int socket_id, int dest_id) {
    FILE *file = fopen("log.txt", "r");
    char out[1024] = "No history found";
    int found = 0;
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        char *id1 = strtok(buffer, "|");
        char *id2 = strtok(NULL, "|");
        char *type = strtok(NULL, "|");
        char *message = strtok(NULL, "|");
        if (atoi(id1) == sender_id && atoi(id2) == dest_id && strcmp(type, "s") == 0) {
            sprintf(out, "you: %s", message);
            send(socket_id, out, strlen(out), 0);
            found = 1;
        } 
        if (atoi(id1) == dest_id && atoi(id2) == sender_id && strcmp(type, "r") == 0) {
            sprintf(out, "%d: %s", dest_id, message);
            send(socket_id, out, strlen(out), 0);
            found = 1;
        }
    }
    if (found == 0) {
        send(socket_id, out, strlen(out), 0);
    }
    fclose(file);
}

// void delete_history(int, int, int);

void delete_history(int sender_id, int socket_id, int dest_id) {
    int ind = 0;
    FILE *file = fopen("log.txt", "r");
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        char *id1 = strtok(buffer, "|");
        char *id2 = strtok(NULL, "|");
        char *type = strtok(NULL, "|");
        char *message = strtok(NULL, "");
        if (dest_id == -1) {
            if (atoi(id1) == sender_id  && strcmp(type, "s") == 0) 
                continue;
            else if (atoi(id2) == sender_id && strcmp(type, "r") == 0) 
                continue;
            else {
                strcpy(chat_history[ind].message, message);
                strcpy(chat_history[ind].type, type);
                chat_history[ind].sender_id = atoi(id1);
                chat_history[ind].receiver_id = atoi(id2);
                ind++;
            }
        } else if (atoi(id1) == sender_id && atoi(id2) == dest_id && strcmp(type, "s") == 0) {
            continue;
        } else if (atoi(id2) == sender_id && atoi(id1) == dest_id && strcmp(type, "r") == 0) {
            continue;
        } else {
            strcpy(chat_history[ind].message, message);
            strcpy(chat_history[ind].type, type);
            chat_history[ind].sender_id = atoi(id1);
            chat_history[ind].receiver_id = atoi(id2);
            ind++;
        }
    }
    fclose(file);
    FILE *fp = fopen("log.txt", "w");
    for(int i = 0; i < ind; i++) {
        fprintf(fp, "%d|%d|%s|%s", chat_history[i].sender_id, chat_history[i].receiver_id, chat_history[i].type, chat_history[i].message);
    }
    fclose(fp);
    sprintf(buffer, "Client %d history deleted successfully", dest_id);
    send(socket_id, buffer, strlen(buffer), 0);
}

void merge(int arr[], int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    // Create temporary arrays
    int L[n1], R[n2];

    // Copy data to temporary arrays L[] and R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    // Merge the temporary arrays back into arr[left..right]
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = left; // Initial index of merged subarray
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[], if there are any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[], if there are any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(int arr[], int left, int right) {
    if (left < right) {
        // Same as (left+right)/2, but avoids overflow for large left and right
        int mid = left + (right - left) / 2;

        // Sort first and second halves
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);

        // Merge the sorted halves
        merge(arr, left, mid, right);
    }
}

void printArray(int arr[], int size) {
    int i;
    for (i = 0; i < size; i++)
        printf("%d ", arr[i]);
    printf("\n");
}

int main1() {
    int arr[] = {12, 11, 13, 5, 6, 7};
    int arr_size = sizeof(arr) / sizeof(arr[0]);

    // printf("Given array is \n");
    // printArray(arr, arr_size);

    mergeSort(arr, 0, arr_size - 1);

    // printf("\nSorted array is \n");
    // printArray(arr, arr_size);
    // return 0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    main1();
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    for (int i = 0; i < MAX_CLIENTS; ++i) { 
        client_sockets[i] = 0; 
    }

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
                if (sd > max_sd) 
                    max_sd = sd;
            }
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            if (num_clients >= MAX_CLIENTS) {
                printf("Maximum clients reached. Connection rejected.\n");
                close(client_fd);
                continue;
            }

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_fd;
                    active_clients[i].id = uuid++;
                    genUUID();
                    active_clients[i].chatboat_login = 0;
                    active_clients[i].sockId = client_fd;
                    active_clients[i].addr = client_addr;

                    num_clients++;

                    char welcome_msg[BUFFER_SIZE];
                    sprintf(welcome_msg, "Welcome! Your ID is %d", active_clients[i].id);
                    send(client_fd, welcome_msg, strlen(welcome_msg), 0);
                    printf("Client %d connected with socket id %d\n", active_clients[i].id, client_fd); 
                    break;
                }
            }
        } else {
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                int sd = client_sockets[i];
                if (FD_ISSET(sd, &readfds)) {
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, sizeof(buffer));
                    if (recv(sd, buffer, BUFFER_SIZE, 0) <= 0) {
                        remove_client(i);
                    } else {
                        checkIO( buffer, i);
                    }
                }
            }
        }
    }
    
    close(server_fd);
    return 0;
}

void checkIO(char* buffer,int socket_index) {
    char query[1024];
    strcpy(query, buffer);
    char *command = strtok(buffer, " ");

    if (strcmp(command, "/logout") == 0){ 
        remove_client(socket_index);
    } else if (strcmp(command, "/active") == 0) { 
        send_active_clients(client_sockets[socket_index]);
    }
    else if (strcmp(command, "/send")==0) {
        char *dest_id_str = strtok(NULL, " ");
        char *message = strtok(NULL, "");
        if (dest_id_str != NULL && message != NULL) {
            int dest_id = atoi(dest_id_str);
            send_message(active_clients[socket_index].id, client_sockets[socket_index],  dest_id, message);
        }
    }
    else if (strcmp(command, "/delete_all")==0) { 
        delete_history(active_clients[socket_index].id, client_sockets[socket_index], -1);
    }
     else if (strcmp(command, "/history")==0) { 
        get_history(active_clients[socket_index].id, client_sockets[socket_index], atoi(strtok(NULL,"")));
    } else if (strcmp(command, "/history_delete")==0) { 
        delete_history(active_clients[socket_index].id, client_sockets[socket_index], atoi(strtok(NULL,"")));
    }  else if (strcmp(command, "/chatbot")==0) {
        char *token = strtok(NULL," ");
        if(strcmp(token,"login")==0) {
             active_clients[socket_index].chatboat_login = 1;
             send(client_sockets[socket_index], chatbot_login_msg, strlen(chatbot_login_msg), 0);
        } else if (strcmp(token,"logout")==0) {
            active_clients[socket_index].chatboat_login=0;
            send(client_sockets[socket_index], chatbot_logout_msg, strlen(chatbot_logout_msg), 0);
        }
    } else {
        if (active_clients[socket_index].chatboat_login) {
            checkFAQ(query, active_clients[socket_index].id, client_sockets[socket_index]);
        }
    }
}



void send_active_clients(int socket_id) {
    char temp[MAX_MSG_LEN] = "Active Clients: ";
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) { 
            sprintf(temp, "%s %d", temp, active_clients[i].id);
        }
    }
    printf("%s\n", temp);
    send(socket_id, temp, strlen(temp), 0);
}


void remove_client(int socket_index) {
    printf("Host %d disconnected, socket fd is %d\n", active_clients[socket_index].id, client_sockets[socket_index]);
    send(client_sockets[socket_index], logout_msg, strlen(logout_msg), 0);
    close(client_sockets[socket_index]);
    client_sockets[socket_index] = 0;
    num_clients--;
}
