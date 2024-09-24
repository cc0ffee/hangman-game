#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 8080                   // selected port to use
#define BUFFER_SIZE 1024            // buffer size is 1KB
#define WORDLIST_SIZE 67600         // amount of words in list, use this for lookup on word        
#define WORD_BUFFER 32              // longest word is 29 letters long, set to 32 just to be safe     

char words[WORDLIST_SIZE][WORD_BUFFER]; // 2D array with size of wordlist with size 32 for each word
int word_count;                         

// function will load all words from text file into words array
void load_word_list() {
    // open text file with words on read mode
    FILE *file = fopen("words.txt", "r");

    // buffer for each word
    char buffer[WORD_BUFFER];
    word_count = 0;

    // scan through each line until end of file, added each word to the word array
    while(fscanf(file, "%s", buffer) != EOF) {
        snprintf(words[word_count], WORD_BUFFER, "%s", buffer);
        word_count++;
    }
    
    // close file and print out how many words are loaded
    fclose(file);
    printf("%d words loaded\n", word_count);
}

// handles the client and runs hangman game
void *handle_client(void *conn_ptr) {
    // takes connection as argument to assign it so we know where to communicate
    int conn = *(int *)conn_ptr;
    free(conn_ptr); // can't forget to free the memory!

    // set a buffer for our any messages incoming/outgoing
    char buffer[BUFFER_SIZE]= { 0 };   
    memset(buffer, 0, sizeof(buffer));

    // game variables
    int lives = 10;                                 // amount of lives
    char guessed_letters[26] = { 0 };               // all guessed characters go to this array (size 26, same length as alphabet)
    char* word = words[rand() % word_count];        // pick random number, use as index to get a random word
    char guess_word[WORD_BUFFER];                   // create the string that the user will be seeing and guessing
    memset(guess_word, '-', strlen(word));          // set all characters in word to "-" of length word so the user can guess
    guess_word[strlen(word)] = '\0';                // set end of string as null-terminate incase it didn't add it in the first place

    // use flag to detect when an error has occured (ie input is greater than a character, non-alpha character, duplicate guess)
    // this will be used when the loop returns to the start, adding the error message along with the regular message to guess again
    int flag = 0;                                 
    const char *err_messages[] = {"", "Input more than 1 character!\n", "Invalid character!\n", "You already asked that letter!\n"};
    
    while((strcmp(word, guess_word) != 0) && (lives > 0)) {
        
        // clear buffer to eliminate anything left there
        memset(buffer, 0, sizeof(buffer));
        
        // send the game data message consisting of: error (if applicable), guess_word, and amount of lives
        snprintf(buffer, BUFFER_SIZE, "%s%s\nYou have %d lives left\nEnter a letter: ", err_messages[flag], guess_word, lives);
        send(conn, buffer, strlen(buffer), 0);

        flag = 0;                                                               // set flag back to 0 as we announced the error to user
        memset(buffer, 0, sizeof(buffer));                                      // clear buffer of previous message
        int conn_data = recv(conn, buffer, sizeof(buffer), 0);                  // get input from user

        // if connection has disconnected or timeout, ungracefully exit
        if(conn_data <= 0) {
            close(conn);
            printf("Client timeout/disconnect, ungracefully exiting!\n");
            fflush(stdout);
            return 0;
        }

        // if input is more than a single letter, push error and continue
        if (strlen(buffer) > 2) {
            flag = 1;
            continue;
        }

        // set input to lowercase
        char guess = tolower(buffer[0]);    
        
        // if input is not an alpha character, push error and continue
        if (!isalpha(guess)) {
            flag = 2;
            continue;
        }

        // if input has been guessed already, push error and continue
        if (strchr(guessed_letters, guess)) {
            flag = 3;
            continue;
        }

        // add new letter to array of already guessed letters
        strncat(guessed_letters, &guess, 1); 
        
        // iterate through word, if letter is present then add that letter to same position in guess_word
        // to then dislay for the user the next round, letter_found tells if a letter is found in word or not
        int letter_found = 0;
        for (int i = 0; i < strlen(word); i++) {
            if (word[i] == guess) {
                guess_word[i] = guess;
                letter_found = 1;
            }
        }

        // if letter is not in word, remove 1 live
        if (!letter_found) {
            lives--;
        }
        
    }

    // depending on how many lives left, output the win or lose
    if (lives > 0) {
        snprintf(buffer, BUFFER_SIZE, "Congratulations! The correct word was indeed '%s'\n", word);
    } else {
        snprintf(buffer, BUFFER_SIZE, "Sorry, you have used up all 10 of your lives. The correct word was '%s'\n", word);
    }

    // send message and close the connection, as the game is finished
    send(conn, buffer, strlen(buffer), 0);
    close(conn);
    printf("Client finished game, gracefully exiting!\n");
    fflush(stdout);
    return 0;
}

int main(int argc, char const* argv[]) {
    // define necessary variables to establish server
    int socket_fd;
    struct sockaddr_in server_addr;

    // set server to IPv4, accept all connection, and set PORT
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // create socket for TCP, bind to address, and listen for connections (limit to 64 connections before refusal)
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int bind_addr = bind(socket_fd,(struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(socket_fd, 64);

    // put all the words into memory
    load_word_list();

    while (1) {
        // random seed to create randomness for each client
        srand(time(NULL));
        // memory alloc needed to pass conn argument to handle_client
        int *conn = malloc(sizeof(int));
        // accept connection from a client
        *conn = accept(socket_fd, (struct sockaddr*) NULL, NULL);

        // create a new thread per client
        pthread_t t_id;
        pthread_create(&t_id, NULL, handle_client, conn);
        printf("A new game has been created!\n");
        fflush(stdout);
        // frees thread when client is terminated
        pthread_detach(t_id);
    }
    // close the socket when server shutsdown
    close(socket_fd);
}