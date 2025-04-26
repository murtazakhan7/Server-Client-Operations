// 2022532_3_4_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8083
#define BUFFER_SIZE 16384
#define MAX_WORD_LENGTH 100
#define MAX_WORD_COUNT 5000

// Structure for word frequency
typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordFreq;

// Function to convert a word to lowercase
void to_lowercase(char* word) {
    for (int i = 0; word[i]; i++) {
        word[i] = tolower(word[i]);
    }
}

// Function to remove punctuation from a word
void remove_punctuation(char* word) {
    int i, j = 0;
    for (i = 0; word[i]; i++) {
        if (isalnum(word[i]) || word[i] == '-' || word[i] == '_') {
            word[j++] = word[i];
        }
    }
    word[j] = '\0';
}

// Function to count word frequencies in a text chunk
int count_word_frequencies(char* text, WordFreq* word_freq) {
    int word_count = 0;
    char* token = strtok(text, " \t\n\r\f\v.,;:!?\"'()[]{}-");
    
    while (token != NULL && word_count < MAX_WORD_COUNT) {
        // Skip empty tokens
        if (strlen(token) > 0) {
            // Convert to lowercase and remove punctuation
            to_lowercase(token);
            remove_punctuation(token);
            
            // Skip if word became empty after processing
            if (strlen(token) > 0) {
                // Check if word already exists in dictionary
                int found = 0;
                for (int i = 0; i < word_count; i++) {
                    if (strcmp(word_freq[i].word, token) == 0) {
                        word_freq[i].count++;
                        found = 1;
                        break;
                    }
                }
                
                // Add new word to dictionary
                if (!found) {
                    strcpy(word_freq[word_count].word, token);
                    word_freq[word_count].count = 1;
                    word_count++;
                }
            }
        }
        
        token = strtok(NULL, " \t\n\r\f\v.,;:!?\"'()[]{}-");
    }
    
    return word_count;
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
    
    // Receive chunk size from server
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        perror("No data received from server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    int chunk_size = atoi(buffer);
    printf("Expected chunk size: %d bytes\n", chunk_size);
    
    // Send acknowledgment
    strcpy(buffer, "ACK");
    send(sock, buffer, strlen(buffer), 0);
    
    // Receive text chunk
    char* chunk = (char*)malloc(chunk_size + 1);
    if (!chunk) {
        perror("Memory allocation failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    memset(chunk, 0, chunk_size + 1);
    valread = read(sock, chunk, chunk_size);
    
    if (valread <= 0) {
        perror("Failed to receive chunk");
        free(chunk);
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    chunk[valread] = '\0';
    printf("Received %d bytes of text\n", valread);
    
    // Count word frequencies
    WordFreq word_freq[MAX_WORD_COUNT];
    int word_count = count_word_frequencies(chunk, word_freq);
    
    printf("Found %d unique words in chunk\n", word_count);
    
    // Send word frequencies back to server
    memset(buffer, 0, BUFFER_SIZE);
    int pos = 0;
    
    // Format: word_count, [word count_1], [word count_2], ...
    pos += sprintf(buffer + pos, "%d ", word_count);
    
    for (int i = 0; i < word_count; i++) {
        pos += sprintf(buffer + pos, "%s %d ", word_freq[i].word, word_freq[i].count);
        // Reg number: 2022532
    }
    
    send(sock, buffer, strlen(buffer), 0);
    
    printf("Sent word frequencies to server\n");
    
    // Cleanup
    free(chunk);
    close(sock);
    
    return 0;
}