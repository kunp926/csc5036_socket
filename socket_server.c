#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define BACKLOG 3

//To find the 1st slot in client_socket.Return -1 if not free slot found, else return the according idx.
static int find_free_slot(int client_socket[], int max_clients)
{
    for (int i = 0; i < max_clients; i++) {
        if (client_socket[i] == 0)
            return i;
    }
    return -1;
}

int main() {
    int opt = 1;
    int master_socket, addrlen, new_socket, client_socket[MAX_CLIENTS], valread;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    struct pollfd poll_fds[MAX_CLIENTS + 1]; // +1 for the master socket
    int max_clients = MAX_CLIENTS;
    int poll_count;

    // Initialize all client_socket[] to 0 so not checked
    for (int i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }


    /* TODO: socket()
    * Create a master socket using socket(); 
    * outputs an error message if it fails.
    */
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    

    /* TODO: setsockopt()
    * Use setsockopt() to enable the SO_REUSEADDR option. 
    * The server allows multiple connections on the same local address and port.
    */
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR,
        (const char*)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    // Type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    /* TODO: bind()
    * The server should bind to the specified port and address without errors.
    */
    if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("Listener on port %d \n", PORT);
    /* TODO: listen()
    * The server listens on the specified port and can queue up to 3 pending 
    * connections for the master socket.
    */
    if (listen(master_socket,BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    // Set up the initial listening socket and event for listening to incoming connections
    poll_fds[0].fd = master_socket;
    poll_fds[0].events = POLLIN;

    while (1) {
        /* TODO: 
        * Prepare the poll_fds array for existing connections.
        * Hint: poll_fds[0] is reserved for the listening socket.
        */
        for (int i = 0; i < max_clients; i++) {
            poll_fds[i + 1].fd = client_socket[i];
            poll_fds[i + 1].events = POLLIN;
            poll_fds[i + 1].revents = 0;

        }

        // Wait for some event on one of the sockets with no timeout (NULL)
        poll_count = poll(poll_fds, max_clients + 1, -1);

        if (poll_count < 0) {
            if (errno == EINTR) continue;
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        // If something happened on the master socket, then it's an incoming connection
        if (poll_fds[0].revents & POLLIN) {
            /* TODO: accept()
            * Accept a new connection when there is incoming request on the master socket.
            */
            new_socket = accept(master_socket, (struct sockaddr*)&address,(socklen_t*) &addrlen);
            if (new_socket < 0) {
                if (errno == EINTR || errno == ECONNABORTED || errno == EAGAIN) {
                    perror("accept (recoverable)");
                }
                else {
                    perror("accept");
                }
            }
            else {
                printf("New connection, socket fd is %d, ip is: %s, port: %d\n",
                    new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                /* TODO:
                * Add the new client socket to the client_socket array.
                */
                int slot = find_free_slot(client_socket, max_clients);
                if (slot < 0) {

                    fprintf(stderr, "Max clients (%d) reached, rejecting fd %d\n",
                        max_clients, new_socket);
                    close(new_socket);
                }
                else {
                    client_socket[slot] = new_socket;
                }
            }
            
        }

        // Else it's some IO operation on some other socket
        for (int i = 0; i < max_clients; i++) {
            int sd = client_socket[i];

            // Skip if this slot is not connected
            if (sd == 0) continue;

            if (poll_fds[i + 1].revents & POLLIN) {
                // Check if it was a closing and also read the incoming message
                if ((valread = read(sd, buffer, BUFFER_SIZE - 1)) == 0) {
                    // Somebody disconnected, get details and print
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s, port %d\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                } else if (valread < 0) {
                    if (errno == EINTR)
                        continue;

                    perror("read");
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    // Echo back the message that came in
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}