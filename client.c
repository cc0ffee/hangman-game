#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_SIZE 1024    // buffer size is 1KB

int main(int argc, char const* argv[]) {
    // returns -1 if IP and PORT are not passed as arguments
    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return -1;
    }
    
    // set variables for IP and PORT
    const char* IP_ADDR = argv[1];
    int PORT = atoi(argv[2]);

    // necessary variables
    int socket_fd;
    char buffer[BUFFER_SIZE] = { 0 };
    struct sockaddr_in server_addr;
    
    // flag to continue game or exit program
    char in_game = 'y';

    // IPv4 and set PORT of server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP_ADDR, &server_addr.sin_addr);
    
    while (in_game == 'y') {
        // establish connection with server
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        int conn = connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

        // check if there's server connection, if not exit
        if (conn < 0) {
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        while(1) {
            // clear memory for buffer
            memset(buffer, 0, BUFFER_SIZE);
            // receive server message
            int received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
            // if nothing received, assumes server closed
            if (received <= 0) {
                printf("No connection to server, ungracefully exiting!\n");
                exit(EXIT_FAILURE);
            }
            // print server message
            printf("%s", buffer);
            // if game is completed, win or lose then exit game loop
            if (strstr(buffer, "Congratulations") != NULL || strstr(buffer, "Sorry") != NULL) {
                break;
            }
            // get input for letter and send it to server
            fgets(buffer, sizeof(buffer), stdin);
            send(socket_fd, buffer, strlen(buffer), 0);
        }

        // close connection since if user wants another game, the server will do a new thread
        close(socket_fd);

        // while loop to handle invalid input
        while(1) {
            printf("Want to play again? (y/n): ");
            fgets(buffer, sizeof(buffer), stdin);
            // y to play another game
            if(strlen(buffer) > 1 && tolower(buffer[0]) == 'y') {
                in_game = 'y';
                break;
            }
            // n to exit the client
            if(strlen(buffer) > 1 && tolower(buffer[0]) == 'n') {
                in_game = 'n';
                break;
            }
            // error handling for invalid inputs
            printf("Not a valid response!\n");
        }
    }
}