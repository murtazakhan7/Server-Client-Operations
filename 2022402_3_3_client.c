// 2022402_3_3_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8082
#define BUFFER_SIZE 8192
#define MAX_SIZE 100

// Matrix structure
typedef struct {
    int rows;
    int cols;
    int data[MAX_SIZE][MAX_SIZE];
} Matrix;

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
    
    // Receive matrices from server
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        perror("No data received from server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Parse the received data
    Matrix A, B, C;
    int rows_to_process, A_rows, A_cols, B_rows, B_cols;
    
    char *token = strtok(buffer, " ");
    rows_to_process = atoi(token);
    
    token = strtok(NULL, " ");
    A_rows = atoi(token);
    
    token = strtok(NULL, " ");
    A_cols = atoi(token);
    
    token = strtok(NULL, " ");
    B_rows = atoi(token);
    
    token = strtok(NULL, " ");
    B_cols = atoi(token);
    
    // Initialize matrices
    A.rows = rows_to_process;
    A.cols = A_cols;
    B.rows = B_rows;
    B.cols = B_cols;
    C.rows = rows_to_process;
    C.cols = B_cols;
    
    // Read matrix A data (just the assigned rows)
    for (int i = 0; i < rows_to_process; i++) {
        for (int j = 0; j < A_cols; j++) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                A.data[i][j] = atoi(token);
            }
        }
    }
    
    // Read matrix B data (entire matrix)
    for (int i = 0; i < B_rows; i++) {
        for (int j = 0; j < B_cols; j++) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                B.data[i][j] = atoi(token);
            }
        }
    }
    
    printf("Received %d rows to process\n", rows_to_process);
    printf("A: %dx%d, B: %dx%d\n", A.rows, A.cols, B.rows, B.cols);
    
    // Perform matrix multiplication for assigned rows
    for (int i = 0; i < rows_to_process; i++) {
        for (int j = 0; j < B_cols; j++) {
            C.data[i][j] = 0;
            for (int k = 0; k < A_cols; k++) {
                C.data[i][j] += A.data[i][k] * B.data[k][j];
                
            }
        }
    }
    
    printf("Multiplication completed for assigned rows\n");
    
    // Send result back to server
    memset(buffer, 0, BUFFER_SIZE);
    int pos = 0;
    
    // Format: rows_processed, [C data]
    pos += sprintf(buffer + pos, "%d ", rows_to_process);
    
    for (int i = 0; i < rows_to_process; i++) {
        for (int j = 0; j < B_cols; j++) {
            pos += sprintf(buffer + pos, "%d ", C.data[i][j]);
        }
    }
    
    send(sock, buffer, strlen(buffer), 0);
    
    printf("Sent result to server\n");
    
    // Close socket
    close(sock);
    
    return 0;
}
