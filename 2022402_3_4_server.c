// 2022402_3_4_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <ctype.h>
#define PORT 8083
#define MAX_CLIENTS 10
#define BUFFER_SIZE 16384
#define MAX_FILE_SIZE 1000000
#define MAX_WORD_LENGTH 100
#define MAX_WORD_COUNT 10000
#define TOP_N 10
//2022402
// Structure for word frequency
typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordFreq;

// Global word frequency dictionary
WordFreq global_word_freq[MAX_WORD_COUNT];
int global_word_count = 0;
pthread_mutex_t word_freq_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to read text file
char* read_file(const char* filename, long* file_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for file content
    char* content = (char*)malloc(*file_size + 1);
    if (!content) {
        perror("Memory allocation failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    // Read file content
    size_t read_size = fread(content, 1, *file_size, file);
    if (read_size != *file_size) {
        perror("Failed to read file");
        free(content);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    content[*file_size] = '\0';
    fclose(file);
    
    return content;
}

//2022402
// Function to merge word frequencies from a client into the global dictionary
void merge_word_frequencies(char* buffer) {
    char* token = strtok(buffer, " ");
    int pair_count = atoi(token);
    
    pthread_mutex_lock(&word_freq_mutex);
    
    for (int i = 0; i < pair_count; i++) {
        char* word = strtok(NULL, " ");
        char* count_str = strtok(NULL, " ");
        
        if (!word || !count_str) break;
        
        int count = atoi(count_str);
        
        // Check if word already exists in global dictionary
        int found = 0;
        for (int j = 0; j < global_word_count; j++) {
            if (strcmp(global_word_freq[j].word, word) == 0) {
                global_word_freq[j].count += count;
                found = 1;
                break;
            }
        }
        
        // Add new word to dictionary
        if (!found && global_word_count < MAX_WORD_COUNT) {
            strcpy(global_word_freq[global_word_count].word, word);
            global_word_freq[global_word_count].count = count;
            global_word_count++;
        }
    }
    
    pthread_mutex_unlock(&word_freq_mutex);
}

// Function to compare word frequencies for sorting
int compare_word_freq(const void* a, const void* b) {
    return ((WordFreq*)b)->count - ((WordFreq*)a)->count;
}

// Thread function to handle client connections
void* client_handler(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    static int next_chunk = 0;
    static int chunk_size = 0;
    static char* file_content = NULL;
    static long file_size = 0;
    static pthread_mutex_t chunk_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Load file if not loaded yet
    pthread_mutex_lock(&chunk_mutex);
    if (file_content == NULL) {
        file_content = read_file("text_file.txt", &file_size);
        chunk_size = file_size / MAX_CLIENTS;
        if (chunk_size < 100) chunk_size = file_size; // For very small files
    }
    
    // Determine which chunk to send to this client
    int start_pos = next_chunk;
    int end_pos = start_pos + chunk_size;
    
    // Make sure we don't go beyond file size
    if (end_pos > file_size) {
        end_pos = file_size;
    }
    
    //2022402
    // Move to next chunk for future clients
    next_chunk = end_pos;
    
    // Find word boundaries to avoid cutting words
    if (end_pos < file_size) {
        while (end_pos < file_size && !isspace(file_content[end_pos])) {
            end_pos++;
        }
    }
    
    int chunk_length = end_pos - start_pos;
    pthread_mutex_unlock(&chunk_mutex);
    
    if (chunk_length <= 0) {
        printf("No more content to process for this client\n");
        close(client_socket);
        return NULL;
    }
    
    // Prepare chunk to send
    char* chunk = (char*)malloc(chunk_length + 1);
    if (!chunk) {
        perror("Memory allocation failed");
        close(client_socket);
        return NULL;
    }
    
    memcpy(chunk, file_content + start_pos, chunk_length);
    chunk[chunk_length] = '\0';
    
    printf("Sending chunk of %d bytes to client (positions %d to %d)\n", 
           chunk_length, start_pos, end_pos);
    
    // Send chunk to client
    char buffer[BUFFER_SIZE];
    sprintf(buffer, "%d", chunk_length);
    send(client_socket, buffer, strlen(buffer), 0);
    

    //2022402
    // Wait for acknowledgment
    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE);
    
    // Send the actual chunk
    send(client_socket, chunk, chunk_length, 0);
    free(chunk);
    
    // Receive word frequencies from client
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    
    if (bytes_read > 0) {
        printf("Received word frequencies from client\n");
        merge_word_frequencies(buffer);
    }
    
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    // Create a sample text file if it doesn't exist
    FILE* test_file = fopen("text_file.txt", "w");
    if (test_file) {
        fprintf(test_file, "This is a sample text file for testing the distributed word count system. "
                          "It contains multiple words, some of which are repeated. "
                          "The system should count the frequency of each word and return the top N most frequent words. "
                          "Words like 'the', 'a', 'an', 'and', 'of', 'in', 'is', 'it' are common in English text. "
                          "This example also includes some less common words like distributed, frequency, and system. "
                          "The system should be case-insensitive, so 'The' and 'the' should be counted as the same word. "
                          "Let's see how well the distributed system works for counting word frequencies!");
        fclose(test_file);
    }
    
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
    printf("Waiting for clients to help with word counting...\n");
    
    // Accept connections and create threads to handle clients
    pthread_t threads[MAX_CLIENTS];
    int thread_count = 0;
    
    while (thread_count < MAX_CLIENTS) {
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
        
        // Check if we want to wait for more clients
        char input[10];
        printf("Press Enter to continue with %d clients or type 'more' to wait for more clients: ", thread_count);
        fgets(input, sizeof(input), stdin);
        if (strncmp(input, "more", 4) != 0) {
            break;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Sort the word frequencies
    qsort(global_word_freq, global_word_count, sizeof(WordFreq), compare_word_freq);
    
    // Print top N most frequent words
    printf("\nTop %d most frequent words:\n", TOP_N);
    for (int i = 0; i < TOP_N && i < global_word_count; i++) {
        printf("%d. \"%s\": %d occurrences\n", i+1, global_word_freq[i].word, global_word_freq[i].count);
    }
    //2022402
    // Cleanup
    close(server_fd);
    
    return 0;
}
