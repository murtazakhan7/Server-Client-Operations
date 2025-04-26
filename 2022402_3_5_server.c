#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define MAX_BUFFER 4096
#define MAX_ARRAY_SIZE 10000



// Function to merge two sorted arrays
void merge(int arr[], int left[], int leftSize, int right[], int rightSize) {
    int i = 0, j = 0, k = 0;
    
    while (i < leftSize && j < rightSize) {
        if (left[i] <= right[j]) {
            arr[k++] = left[i++];
        } else {
            arr[k++] = right[j++];
        }
    }
    
    while (i < leftSize) {
        arr[k++] = left[i++];
    }
    
    while (j < rightSize) {
        arr[k++] = right[j++];
    }
}

// Function to serialize an array into a string
void serializeArray(int arr[], int size, char *buffer) {
    buffer[0] = '\0';
    char temp[20];
    
    for (int i = 0; i < size; i++) {
        sprintf(temp, "%d ", arr[i]);
        strcat(buffer, temp);
    }
}

// Function to deserialize a string into an array
int deserializeArray(char *buffer, int arr[]) {
    int i = 0;
    char *token = strtok(buffer, " ");
    
    while (token != NULL) {
        arr[i++] = atoi(token);
        token = strtok(NULL, " ");
    }
    
    return i;  // Return the size of the array
}



// Simple merge sorting approach
void simpleSort(int arr[], int size, int numClients) {
    struct timeval start, end;
    long seconds, micros;
    gettimeofday(&start, NULL);
    
    printf("\n[Simple Merge Sorting]\n");
    
    // Create server socket
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, numClients) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);
    
    // Calculate chunk size for each client
    int chunkSize = size / numClients;
    int remainder = size % numClients;
    
    // Arrays to store client connections and their sorted chunks
    int clientSockets[MAX_CLIENTS];
    int sortedChunks[MAX_CLIENTS][MAX_ARRAY_SIZE];
    int chunkSizes[MAX_CLIENTS];
    
    // Connect to all clients and send data
    for (int i = 0; i < numClients; i++) {
        if ((clientSockets[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        
        printf("Client %d connected\n", i + 1);
        
        // Calculate this client's chunk size (accounting for remainder)
        int thisChunkSize = chunkSize;
        if (i < remainder) {
            thisChunkSize++;
        }
        
        // Calculate start index for this client's chunk
        int startIdx = i * chunkSize;
        if (i < remainder) {
            startIdx += i;
        } else {
            startIdx += remainder;
        }
        
        // Prepare and send chunk size
        char sizeBuffer[20];
        sprintf(sizeBuffer, "%d", thisChunkSize);
        send(clientSockets[i], sizeBuffer, strlen(sizeBuffer), 0);
        
        // Wait for acknowledgment
        char ack[10];
        read(clientSockets[i], ack, 10);
        
        // Prepare and send chunk data
        char dataBuffer[MAX_BUFFER];
        serializeArray(&arr[startIdx], thisChunkSize, dataBuffer);
        send(clientSockets[i], dataBuffer, strlen(dataBuffer), 0);
        
        printf("Sent %d elements to client %d\n", thisChunkSize, i + 1);
    }
    
 
    
    // Receive sorted chunks from all clients
    for (int i = 0; i < numClients; i++) {
        char buffer[MAX_BUFFER] = {0};
        read(clientSockets[i], buffer, MAX_BUFFER);
        
        chunkSizes[i] = deserializeArray(buffer, sortedChunks[i]);
        printf("Received sorted chunk from client %d with %d elements\n", i + 1, chunkSizes[i]);
        
        // Close the client socket
        close(clientSockets[i]);
    }
    
    // Close the server socket
    close(server_fd);
    
    // Merge all sorted chunks
    int finalArray[MAX_ARRAY_SIZE];
    int finalSize = 0;
    
    // Copy first chunk to finalArray
    memcpy(finalArray, sortedChunks[0], chunkSizes[0] * sizeof(int));
    finalSize = chunkSizes[0];
    
    // Merge each subsequent chunk
    for (int i = 1; i < numClients; i++) {
        int tempArray[MAX_ARRAY_SIZE];
        memcpy(tempArray, finalArray, finalSize * sizeof(int));
        
        merge(finalArray, tempArray, finalSize, sortedChunks[i], chunkSizes[i]);
        finalSize += chunkSizes[i];
    }
    
    // Copy merged result back to original array
    memcpy(arr, finalArray, size * sizeof(int));
    
    gettimeofday(&end, NULL);
    seconds = end.tv_sec - start.tv_sec;
    micros = end.tv_usec - start.tv_usec;
    if (micros < 0) {
        seconds--;
        micros += 1000000;
    }
    printf("Simple merge sorting completed in: %ld.%06ld seconds\n", seconds, micros);
}



// Parallel merge sorting approach
void parallelSort(int arr[], int size, int numClients) {
    struct timeval start, end;
    long seconds, micros;
    gettimeofday(&start, NULL);
    
    printf("\n[Parallel Merge Sorting]\n");
    
    // Create server socket
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT + 1);  // Use a different port for parallel sort
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, numClients * 2) < 0) {  // Need more connections for parallel merging
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT + 1);
    
    // Calculate chunk size for each client
    int chunkSize = size / numClients;
    int remainder = size % numClients;
    
    // Arrays to store client connections and their sorted chunks
    int clientSockets[MAX_CLIENTS];
    int sortedChunks[MAX_CLIENTS][MAX_ARRAY_SIZE];
    int chunkSizes[MAX_CLIENTS];
    
    // Connect to all clients and send data (same as simple sort)
    for (int i = 0; i < numClients; i++) {
        if ((clientSockets[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        
        printf("Client %d connected\n", i + 1);
        
        // Calculate this client's chunk size (accounting for remainder)
        int thisChunkSize = chunkSize;
        if (i < remainder) {
            thisChunkSize++;
        }
        
        // Calculate start index for this client's chunk
        int startIdx = i * chunkSize;
        if (i < remainder) {
            startIdx += i;
        } else {
            startIdx += remainder;
        }
        
        // Prepare and send chunk size
        char sizeBuffer[20];
        sprintf(sizeBuffer, "%d", thisChunkSize);
        send(clientSockets[i], sizeBuffer, strlen(sizeBuffer), 0);
        
        // Wait for acknowledgment
        char ack[10];
        read(clientSockets[i], ack, 10);
        
        // Prepare and send chunk data
        char dataBuffer[MAX_BUFFER];
        serializeArray(&arr[startIdx], thisChunkSize, dataBuffer);
        send(clientSockets[i], dataBuffer, strlen(dataBuffer), 0);
        
        printf("Sent %d elements to client %d\n", thisChunkSize, i + 1);
    }
    

    
    // Receive sorted chunks from all clients
    for (int i = 0; i < numClients; i++) {
        char buffer[MAX_BUFFER] = {0};
        read(clientSockets[i], buffer, MAX_BUFFER);
        
        chunkSizes[i] = deserializeArray(buffer, sortedChunks[i]);
        printf("Received sorted chunk from client %d with %d elements\n", i + 1, chunkSizes[i]);
        
        // Keep the socket open for the parallel merge phase
    }
    
    // Parallel merge phase
    int levels = 0;
    int temp = numClients;
    while (temp > 1) {
        temp = (temp + 1) / 2;
        levels++;
    }
    
    // Arrays to keep track of current chunks for merging
    int currentChunks[MAX_CLIENTS][MAX_ARRAY_SIZE];
    int currentSizes[MAX_CLIENTS];
    
    // Initialize with sorted chunks from clients
    for (int i = 0; i < numClients; i++) {
        memcpy(currentChunks[i], sortedChunks[i], chunkSizes[i] * sizeof(int));
        currentSizes[i] = chunkSizes[i];
    }
    
    int currentNumChunks = numClients;
    
    // Perform merge levels
    for (int level = 0; level < levels; level++) {
        printf("Merge level %d, working with %d chunks\n", level + 1, currentNumChunks);
        
        int newNumChunks = (currentNumChunks + 1) / 2;
        int newChunks[MAX_CLIENTS][MAX_ARRAY_SIZE];
        int newSizes[MAX_CLIENTS];
        
        // Pairs to merge
        for (int i = 0; i < newNumChunks; i++) {
            int leftIdx = i * 2;
            int rightIdx = leftIdx + 1;
            
            if (rightIdx < currentNumChunks) {
                // Send merge pair to client
                char leftBuffer[MAX_BUFFER], rightBuffer[MAX_BUFFER];
                serializeArray(currentChunks[leftIdx], currentSizes[leftIdx], leftBuffer);
                serializeArray(currentChunks[rightIdx], currentSizes[rightIdx], rightBuffer);
                
                // Create a new merge client pair message
                char mergeMsg[MAX_BUFFER * 2 + 50];
                sprintf(mergeMsg, "%d %d ", currentSizes[leftIdx], currentSizes[rightIdx]);
                strcat(mergeMsg, leftBuffer);
                strcat(mergeMsg, "| ");
                strcat(mergeMsg, rightBuffer);
                
                // Send to client
                send(clientSockets[i % numClients], mergeMsg, strlen(mergeMsg), 0);
                
                // Receive merged result
                char resultBuffer[MAX_BUFFER * 2] = {0};
                read(clientSockets[i % numClients], resultBuffer, MAX_BUFFER * 2);
                
                newSizes[i] = deserializeArray(resultBuffer, newChunks[i]);
                printf("Received merged chunk with %d elements\n", newSizes[i]);
            } else {
                // Only one chunk left, just copy it
                memcpy(newChunks[i], currentChunks[leftIdx], currentSizes[leftIdx] * sizeof(int));
                newSizes[i] = currentSizes[leftIdx];
            }
        }
        
        // Update current chunks for next level
        currentNumChunks = newNumChunks;
        for (int i = 0; i < currentNumChunks; i++) {
            memcpy(currentChunks[i], newChunks[i], newSizes[i] * sizeof(int));
            currentSizes[i] = newSizes[i];
        }
    }
    

    
    // Copy final merged result to original array
    memcpy(arr, currentChunks[0], size * sizeof(int));
    
    // Close all client connections
    for (int i = 0; i < numClients; i++) {
        // Send termination message
        send(clientSockets[i], "DONE", 4, 0);
        close(clientSockets[i]);
    }
    
    // Close the server socket
    close(server_fd);
    
    gettimeofday(&end, NULL);
    seconds = end.tv_sec - start.tv_sec;
    micros = end.tv_usec - start.tv_usec;
    if (micros < 0) {
        seconds--;
        micros += 1000000;
    }
    printf("Parallel merge sorting completed in: %ld.%06ld seconds\n", seconds, micros);
}


// Function to print an array
void printArray(int arr[], int size) {
    printf("[ ");
    for (int i = 0; i < size && i < 20; i++) {
        printf("%d ", arr[i]);
    }
    if (size > 20) {
        printf("... ");
    }
    printf("]\n");
}

// Function to verify if an array is sorted
int isSorted(int arr[], int size) {
    for (int i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <array_size> <num_clients>\n", argv[0]);
        return 1;
    }
    
    int size = atoi(argv[1]);
    int numClients = atoi(argv[2]);
    
    if (size <= 0 || size > MAX_ARRAY_SIZE) {
        printf("Array size must be between 1 and %d\n", MAX_ARRAY_SIZE);
        return 1;
    }
    
    if (numClients <= 0 || numClients > MAX_CLIENTS) {
        printf("Number of clients must be between 1 and %d\n", MAX_CLIENTS);
        return 1;
    }
    
    // Create and fill array with random numbers
    int *arr = (int *)malloc(size * sizeof(int));
    int *arrCopy = (int *)malloc(size * sizeof(int));
    
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 10000;
    }
    
    printf("Generated array of %d random integers\n", size);
    printf("Original array (first few elements): ");
    printArray(arr, size);
    
    // Make a copy for the parallel sort
    memcpy(arrCopy, arr, size * sizeof(int));
    
    // Run simple merge sort
    simpleSort(arr, size, numClients);
    
    printf("Sorted array (first few elements): ");
    printArray(arr, size);
    
    if (isSorted(arr, size)) {
        printf("Simple sort verification: SORTED CORRECTLY\n");
    } else {
        printf("Simple sort verification: SORT FAILED!\n");
    }
    
    // Run parallel merge sort
    parallelSort(arrCopy, size, numClients);
    
    printf("Sorted array (first few elements): ");
    printArray(arrCopy, size);
    
    if (isSorted(arrCopy, size)) {
        printf("Parallel sort verification: SORTED CORRECTLY\n");
    } else {
        printf("Parallel sort verification: SORT FAILED!\n");
    }
    
    free(arr);
    free(arrCopy);
    
    return 0;
}
