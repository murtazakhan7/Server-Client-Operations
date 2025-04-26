// 2022402_3_2_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/select.h>

#define PORT 8081
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define TIMEOUT_SECONDS 30

//2022402
typedef struct {
    long number;
    int assigned;
    int result; // -1: not processed, 0: not prime, 1: prime
    int client_id;
} PrimeTask;

void handle_timeout(int signal) {
    printf("Client timeout detected!\n");
}

int main() {
    // Define large numbers to check for primality
    long numbers[] = {
        104729, 982451653, 6700417, 2147483647, 67280421310721,
        433494437, 2971215073, 11111111111111111, 9007199254740991, 
        999999999989
    };
    int num_count = sizeof(numbers) / sizeof(numbers[0]);
    
    // Initialize tasks
    PrimeTask tasks[num_count];
    for (int i = 0; i < num_count; i++) {
        tasks[i].number = numbers[i];
        tasks[i].assigned = 0;
        tasks[i].result = -1;
        tasks[i].client_id = -1;
    }
    //2022402
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    //2022402
    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT);
    printf("Will be checking these numbers for primality:\n");
    for (int i = 0; i < num_count; i++) {
        printf("%ld\n", numbers[i]);
    }
    
    // Setup for select() to handle multiple clients with timeout
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    int max_fd = server_fd;
    
    // Setup for client tracking
    int client_sockets[MAX_CLIENTS];
    int client_count = 0;
    int next_client_id = 1;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0; // Initialize all client sockets to 0
    }
    
    // Setup alarm for timeout
    signal(SIGALRM, handle_timeout);
    
    // Process clients until all tasks are completed
    int tasks_completed = 0;
    
    while (tasks_completed < num_count) {
        // Copy the master set to the working set
        working_set = master_set;
        
        struct timeval timeout;
        timeout.tv_sec = 5; // Check every 5 seconds
        timeout.tv_usec = 0;
        
        // Wait for activity on one of the sockets
        int activity = select(max_fd + 1, &working_set, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("Select error");
            break;
        }
        //2022402
        // Check if it's the server socket (new connection)
        if (FD_ISSET(server_fd, &working_set)) {
            int new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                continue;
            }
            
            // Add to client sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    client_count++;
                    printf("New client connected, assigned ID: %d\n", next_client_id);
                    
                    // Find an unassigned task
                    int assigned = 0;
                    for (int j = 0; j < num_count; j++) {
                        if (!tasks[j].assigned) {
                            tasks[j].assigned = 1;
                            tasks[j].client_id = next_client_id;
                            
                            char buffer[BUFFER_SIZE];
                            sprintf(buffer, "%ld", tasks[j].number);
                            send(new_socket, buffer, strlen(buffer), 0);
                            
                            printf("Assigned number %ld to client %d\n", tasks[j].number, next_client_id);
                            assigned = 1;
                            break;
                        }
                    }
                    
                    if (!assigned) {
                        printf("No tasks available for new client\n");
                        close(new_socket);
                        client_sockets[i] = 0;
                        client_count--;
                    } else {
                        // Add to set for select()
                        FD_SET(new_socket, &master_set);
                        if (new_socket > max_fd) {
                            max_fd = new_socket;
                        }
                        
                        // Start timeout
                        alarm(TIMEOUT_SECONDS);
                        next_client_id++;
                    }
                    break;
                }
            }
        }
        
        // Check for I/O on client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            
            if (sd > 0 && FD_ISSET(sd, &working_set)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = read(sd, buffer, BUFFER_SIZE);
                
                if (valread == 0) {
                    // Client disconnected
                    printf("Client %d disconnected\n", i+1);
                    
                    // Find this client's task and mark it as unassigned
                    for (int j = 0; j < num_count; j++) {
                        if (tasks[j].client_id == i+1 && tasks[j].result == -1) {
                            tasks[j].assigned = 0;
                            tasks[j].client_id = -1;
                            printf("Task for number %ld returned to pool\n", tasks[j].number);
                        }
                    }
                    
                    close(sd);
                    FD_CLR(sd, &master_set);
                    client_sockets[i] = 0;
                    client_count--;
                } else {
                    // Received result from client
                    int result;
                    sscanf(buffer, "%d", &result);
                    
                    // Find the corresponding task
                    for (int j = 0; j < num_count; j++) {
                        if (tasks[j].client_id == i+1) {
                            tasks[j].result = result;
                            tasks_completed++;
                            
                            printf("Client %d reported: %ld is %s\n", 
                                   i+1, tasks[j].number, 
                                   result ? "PRIME" : "NOT PRIME");
                            
                            close(sd);
                            FD_CLR(sd, &master_set);
                            client_sockets[i] = 0;
                            client_count--;
                            break;
                        }
                    }
                }
            }
        }
        //2022402
        // Check if any remaining tasks need to be assigned to connected clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                for (int j = 0; j < num_count; j++) {
                    if (!tasks[j].assigned) {
                        tasks[j].assigned = 1;
                        tasks[j].client_id = i+1;
                        
                        char buffer[BUFFER_SIZE];
                        sprintf(buffer, "%ld", tasks[j].number);
                        send(client_sockets[i], buffer, strlen(buffer), 0);
                        
                        printf("Assigned number %ld to existing client %d\n", 
                               tasks[j].number, i+1);
                        break;
                    }
                }
            }
        }
    }
    
    // Print final results
    printf("\nFinal Results:\n");
    for (int i = 0; i < num_count; i++) {
        if (tasks[i].result == -1) {
            printf("%ld: Not determined (timeout or error)\n", tasks[i].number);
        } else {
            printf("%ld: %s\n", tasks[i].number, tasks[i].result ? "PRIME" : "NOT PRIME");
        }
    }
    //2022402
    
    // Cleanup
    close(server_fd);
    return 0;
}
