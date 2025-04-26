// 2022402_3_2_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

#define PORT 8081
#define BUFFER_SIZE 1024

// Function to check if a number is prime
int is_prime(long number) {
    // Handle special cases
    if (number <= 1) return 0;  // Not prime
    if (number <= 3) return 1;  // 2 and 3 are prime
    if (number % 2 == 0 || number % 3 == 0) return 0;  // Divisible by 2 or 3
    
    // Use 6k +/- 1 optimization to check for primality
    long sqrt_num = sqrt(number);
    for (long i = 5; i <= sqrt_num; i += 6) {
        if (number % i == 0 || number % (i + 2) == 0)
          
            return 0;  // Found a divisor
    }
    
    return 1;  // No divisors found, number is prime
}

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
    
    // Receive number to check
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        perror("No data received from server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Convert the received string to a long integer
    long number = atol(buffer);
    printf("Received number to check: %ld\n", number);
    
    // Check if the number is prime
    printf("Checking if %ld is prime...\n", number);
    int result = is_prime(number);
    printf("Result: %ld is %s\n", number, result ? "PRIME" : "NOT PRIME");
    
    // Send result back to server
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "%d", result);
    send(sock, buffer, strlen(buffer), 0);
    
    printf("Sent result to server\n");
    
    // Close socket
    close(sock);
    
    return 0;
}
