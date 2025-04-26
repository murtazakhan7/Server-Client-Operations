// 2022402_3_1_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_NUMBERS 10000

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server\n");
    
    // Receive data from server
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        perror("No data received from server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Parse the received numbers
    int numbers[MAX_NUMBERS];
    int count;
    char *token = strtok(buffer, " ");
    
    // First token is the count of numbers
    count = atoi(token);
    printf("Received %d numbers to process\n", count);
    
    // Parse the rest of the numbers
    for (int i = 0; i < count; i++) {
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Error: Fewer numbers received than expected\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        numbers[i] = atoi(token);
    }
    
    // Calculate sum
    long long sum = 0;
    for (int i = 0; i < count; i++) {
        sum += numbers[i];
      
    }
    
    printf("Calculated partial sum: %lld\n", sum);
    
    // Send result back to server
    sprintf(buffer, "%lld", sum);
    send(sock, buffer, strlen(buffer), 0);
    
    printf("Sent result to server\n");
    
    // Close socket
    close(sock);
    
    return 0;
}
