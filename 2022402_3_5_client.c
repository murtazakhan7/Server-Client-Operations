#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_BUFFER 4096
#define MAX_ARRAY_SIZE 10000

// Function to merge two sorted arrays
void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
    
    // Create temp arrays
    int L[n1], R[n2];
    
    // Copy data to temp arrays
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
    
    // Merge the temp arrays back into arr
    i = 0;
    j = 0;
    k = l;
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
    //2022402
    // Copy the remaining elements of L[], if any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    
    // Copy the remaining elements of R[], if any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}


// Merge Sort implementation
void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        
        merge(arr, l, m, r);
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
//2022402

// Function to deserialize a string into an array
int deserializeArray(char *buffer, int arr[]) {
    int i = 0;
    char *token = strtok(buffer, " ");
    
    while (token != NULL && strcmp(token, "|") != 0) {
        arr[i++] = atoi(token);
        token = strtok(NULL, " ");
    }
    
    return i;  // Return the size of the array
}

// Function to deserialize a merge pair
void deserializeMergePair(char *buffer, int left[], int right[], int *leftSize, int *rightSize) {
    // First parse sizes
    *leftSize = atoi(strtok(buffer, " "));
    *rightSize = atoi(strtok(NULL, " "));
    
    // Parse left array
    int i = 0;
    char *token = strtok(NULL, " ");
    while (token != NULL && strcmp(token, "|") != 0) {
        left[i++] = atoi(token);
        token = strtok(NULL, " ");
    }
    
    // Parse right array
    i = 0;
    token = strtok(NULL, " ");
    while (token != NULL) {
        right[i++] = atoi(token);
        token = strtok(NULL, " ");
    }
}

//2022402

// Function to merge two already sorted arrays
void mergeSortedArrays(int left[], int leftSize, int right[], int rightSize, int result[]) {
    int i = 0, j = 0, k = 0;
    
    while (i < leftSize && j < rightSize) {
        if (left[i] <= right[j]) {
            result[k++] = left[i++];
        } else {
            result[k++] = right[j++];
        }
    }
    
    while (i < leftSize) {
        result[k++] = left[i++];
    }
    
    while (j < rightSize) {
        result[k++] = right[j++];
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }
    
    char *server_ip = argv[1];
    
    // Create socket for simple sort
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to server for simple sort\n");
    
    // Receive chunk size
    char sizeBuffer[20] = {0};
    read(sock, sizeBuffer, 20);
    int chunkSize = atoi(sizeBuffer);
    
    // Send acknowledgment
    send(sock, "ACK", 3, 0);
    
    // Receive chunk data
    char buffer[MAX_BUFFER] = {0};
    read(sock, buffer, MAX_BUFFER);
    
    // Deserialize data
    int chunk[MAX_ARRAY_SIZE];
    int size = deserializeArray(buffer, chunk);
    
    printf("Received %d integers to sort\n", size);
    
    // Sort the chunk
    mergeSort(chunk, 0, size - 1);
    
    printf("Chunk sorted\n");
    
    // Send sorted chunk back
    char sortedBuffer[MAX_BUFFER];
    serializeArray(chunk, size, sortedBuffer);
    send(sock, sortedBuffer, strlen(sortedBuffer), 0);
    

    
    // Now connect for parallel sort tasks
    int parallelSock = 0;
    
    if ((parallelSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error for parallel sort\n");
        return -1;
    }
    
    serv_addr.sin_port = htons(PORT + 1);  // Use the parallel sort port
    
    if (connect(parallelSock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed for parallel sort\n");
        return -1;
    }
    
    printf("Connected to server for parallel sort\n");
    
    // Receive chunk size for parallel sort
    read(parallelSock, sizeBuffer, 20);
    chunkSize = atoi(sizeBuffer);
    
    // Send acknowledgment
    send(parallelSock, "ACK", 3, 0);
    
    // Receive chunk data
    memset(buffer, 0, MAX_BUFFER);
    read(parallelSock, buffer, MAX_BUFFER);
    
    // Deserialize data
    size = deserializeArray(buffer, chunk);
    
    printf("Received %d integers to sort for parallel sort\n", size);
    
    // Sort the chunk
    mergeSort(chunk, 0, size - 1);
    
    printf("Chunk sorted for parallel sort\n");
    
    // Send sorted chunk back
    serializeArray(chunk, size, sortedBuffer);
    send(parallelSock, sortedBuffer, strlen(sortedBuffer), 0);
    
    // Wait for merge tasks or termination
    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        int bytes_read = read(parallelSock, buffer, MAX_BUFFER * 2);
        
        // Check if we received termination signal
        if (bytes_read <= 0 || strcmp(buffer, "DONE") == 0) {
            printf("Received termination signal, exiting...\n");
            break;
        }
        
        // Process merge task
        int left[MAX_ARRAY_SIZE], right[MAX_ARRAY_SIZE], result[MAX_ARRAY_SIZE * 2];
        int leftSize, rightSize;
        
        // Deserialize merge pair
        deserializeMergePair(buffer, left, right, &leftSize, &rightSize);
        
        printf("Received merge task: %d and %d elements\n", leftSize, rightSize);
        
        // Merge the arrays
        mergeSortedArrays(left, leftSize, right, rightSize, result);
        
        // Send merged result back
        char resultBuffer[MAX_BUFFER * 2];
        serializeArray(result, leftSize + rightSize, resultBuffer);
        send(parallelSock, resultBuffer, strlen(resultBuffer), 0);
        
        printf("Sent merged result of %d elements\n", leftSize + rightSize);
    }
    //2022402
    
    // Close the sockets
    close(sock);
    close(parallelSock);
    
    return 0;
}
