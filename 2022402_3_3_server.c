// 2022402_3_3_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>


//2022402
#define PORT 8082
#define MAX_CLIENTS 10
#define BUFFER_SIZE 8192
#define MAX_SIZE 100

// Matrix structures
typedef struct {
    int rows;
    int cols;
    int data[MAX_SIZE][MAX_SIZE];
} Matrix;

// Global matrices
Matrix A, B, C;

// Function to read a matrix from a file
void read_matrix(Matrix *mat, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open matrix file");
        exit(EXIT_FAILURE);
    }
    
    // Read dimensions
    fscanf(file, "%d %d", &mat->rows, &mat->cols);
    
    // Read matrix elements
    for (int i = 0; i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++) {
            fscanf(file, "%d", &mat->data[i][j]);
        }
    }
    
    fclose(file);
}

// Function to print a matrix
void print_matrix(Matrix *mat) {
    printf("Matrix %dx%d:\n", mat->rows, mat->cols);
    for (int i = 0; i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++) {
            printf("%d ", mat->data[i][j]);
        }
        printf("\n");
    }
}
// 2022402
// Thread function to handle client connections
void *client_handler(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);
    
    // Determine which rows to assign to this client
    static int next_row = 0;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&mutex);
    int start_row = next_row;
    int rows_to_process = (A.rows - next_row) > 0 ? 1 : 0;
    next_row += rows_to_process;
    pthread_mutex_unlock(&mutex);
    
    if (rows_to_process == 0) {
        printf("No work for this client\n");
        close(client_socket);
        return NULL;
    }
    
    printf("Assigning row %d to client\n", start_row);
    
    // Send matrices A and B to client
    char buffer[BUFFER_SIZE];
    int pos = 0;
    
    // Format: rows_to_process, A.rows, A.cols, B.rows, B.cols, [A data], [B data]
    pos += sprintf(buffer + pos, "%d %d %d %d %d ", 
                  rows_to_process, A.rows, A.cols, B.rows, B.cols);
    
    // Send only the rows needed from A
    for (int i = start_row; i < start_row + rows_to_process; i++) {
        for (int j = 0; j < A.cols; j++) {
            pos += sprintf(buffer + pos, "%d ", A.data[i][j]);
        }
    }
    
    // Send entire matrix B
    for (int i = 0; i < B.rows; i++) {
        for (int j = 0; j < B.cols; j++) {
            pos += sprintf(buffer + pos, "%d ", B.data[i][j]);
        }
    }
    
    send(client_socket, buffer, strlen(buffer), 0);
    
    // Receive result from client
    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE);
    
    // Parse the result and update C matrix
    char *token = strtok(buffer, " ");
    int row_count = atoi(token);
    
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < row_count; i++) {
        for (int j = 0; j < B.cols; j++) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                C.data[start_row + i][j] = atoi(token);
            }
        }
    }
    //2022402
    pthread_mutex_unlock(&mutex);
    
    printf("Received result for row %d\n", start_row);
    
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    // Read matrices from files
    // Example hardcoded matrices for testing
    // Matrix A: 3x3
    A.rows = 3;
    A.cols = 3;
    for (int i = 0; i < A.rows; i++) {
        for (int j = 0; j < A.cols; j++) {
            A.data[i][j] = i + j + 1;
        }
    }
    
    // Matrix B: 3x3
    B.rows = 3;
    B.cols = 3;
    for (int i = 0; i < B.rows; i++) {
        for (int j = 0; j < B.cols; j++) {
            B.data[i][j] = i * j + 1;
        }
    }
    

    //2022402
    // Check if dimensions are compatible for multiplication
    if (A.cols != B.rows) {
        fprintf(stderr, "Error: Incompatible matrix dimensions for multiplication\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize result matrix C
    C.rows = A.rows;
    C.cols = B.cols;
    memset(C.data, 0, sizeof(C.data));
    
    // Print input matrices
    printf("Matrix A:\n");
    print_matrix(&A);
    printf("Matrix B:\n");
    print_matrix(&B);
    
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
    //2022402
    
    printf("Server listening on port %d\n", PORT);
    printf("Waiting for clients to help with matrix multiplication...\n");
    
    // Accept connections and create threads to handle clients
    pthread_t threads[MAX_CLIENTS];
    int thread_count = 0;
    
    while (thread_count < A.rows) {
        int *client_sock = malloc(sizeof(int));
        if ((*client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            free(client_sock);
            continue;
        }
        
        printf("New client connected\n");
        
        if (pthread_create(&threads[thread_count], NULL, client_handler, client_sock) != 0) {
            perror("Thread creation failed");
            free(client_sock);
            continue;
        }
        
        thread_count++;
    
        
        // Check if we've processed all rows
        if (thread_count >= A.rows) {
            printf("All rows have been assigned, no more clients needed\n");
            break;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print result matrix
    printf("\nResult Matrix C = A * B:\n");
    print_matrix(&C);
            // Cleanup
            close(server_fd);
            
            return 0;
    //2022402
        }
