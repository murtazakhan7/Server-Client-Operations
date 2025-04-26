// 2022402_3_1_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_NUMBERS 10000

int main() {
    int server_fd, client_sockets[MAX_CLIENTS];
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    int numbers[MAX_NUMBERS];
    int num_count = 0;
    long long total_sum = 0;
    
    // Read integers from file
    FILE *file = fopen("integersQ1.txt", "r");
    if (file == NULL) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }
    
    int temp;
    while (fscanf(file, "%d", &temp) == 1 && num_count < MAX_NUMBERS) {
        numbers[num_count++] = temp;
    }
    fclose(file);
    
    printf("Read %d numbers from file\n", num_count);
    
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
    
    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT);
    printf("Waiting for clients to connect...\n");
    
    // Accept connections and assign work
    int client_count = 0;
    while (client_count < MAX_CLIENTS) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        client_sockets[client_count++] = new_socket;
        printf("New client connected (%d/%d)\n", client_count, MAX_CLIENTS);
        
        // Check if we have enough clients or should wait for more
        char input[10];
        printf("Press Enter to continue with %d clients or type 'more' to wait for more clients: ", client_count);
        fgets(input, sizeof(input), stdin);
        if (strncmp(input, "more", 4) != 0) {
            break;
        }
    }
    
    printf("Starting computation with %d clients\n", client_count);
    
    // Divide work among clients
    int numbers_per_client = num_count / client_count;
    int extras = num_count % client_count;
    
    int start_idx = 0;
    for (int i = 0; i < client_count; i++) {
        // Calculate this client's workload
        int this_client_numbers = numbers_per_client + (i < extras ? 1 : 0);
        
        // Create the message with the numbers
        char buffer[BUFFER_SIZE] = {0};
        int pos = 0;
        pos += sprintf(buffer + pos, "%d ", this_client_numbers);
        
        for (int j = 0; j < this_client_numbers; j++) {
            pos += sprintf(buffer + pos, "%d ", numbers[start_idx + j]);
        }
        
        // Send the work to client
        send(client_sockets[i], buffer, strlen(buffer), 0);
        printf("Sent %d numbers to client %d\n", this_client_numbers, i+1);
        
        start_idx += this_client_numbers;
    }
    
    // Receive results from clients
    for (int i = 0; i < client_count; i++) {
        char buffer[BUFFER_SIZE] = {0};
        read(client_sockets[i], buffer, BUFFER_SIZE);
        
        long long partial_sum;
        sscanf(buffer, "%lld", &partial_sum);
        
        total_sum += partial_sum;
        printf("Received partial sum from client %d: %lld\n", i+1, partial_sum);
        
        close(client_sockets[i]);
    }
    
    printf("Total sum of all numbers: %lld\n", total_sum);
    
    close(server_fd);
    return 0;
}
