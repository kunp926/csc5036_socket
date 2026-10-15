#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#define open_file(p, fl) open((p), (fl), 0644)   /* from the skeleton: create files with mode 0644 */

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define BACKLOG     3
#define CHUNK_SIZE  65536   /* chunk size used when writing the body */

/* Send exactly n bytes: send() may transmit only part of the buffer (short write) */
static int send_all(int fd, const void *buf, size_t n)
{
    const char *p = (const char *)buf;
    size_t sent = 0;

    while (sent < n) {
        ssize_t k = send(fd, p + sent, n - sent, 0);
        if (k < 0) {
            if (errno == EINTR) {
                continue;               /* interrupted by a signal: retry */
            }
            return -1;
        }
        sent += (size_t)k;
    }
    return 0;
}

/* Read exactly n bytes: read() may return fewer bytes than requested (short read) */
static int read_full(int fd, void *buf, size_t n)
{
    char *p = (char *)buf;
    size_t got = 0;

    while (got < n) {
        ssize_t k = read(fd, p + got, n - got);
        if (k < 0) {
            if (errno == EINTR) {
                continue;               /* interrupted by a signal: retry */
            }
            return -1;
        }
        if (k == 0) {
            return (int)got;            /* peer closed: report how many bytes we got */
        }
        got += (size_t)k;
    }
    return (int)got;
}

/* Return the first free slot in client_socket[], or -1 if there is none */
static int find_free_slot(int client_socket[], int max_clients)
{
    for (int i = 0; i < max_clients; i++) {
        if (client_socket[i] == 0) {
            return i;
        }
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
                   (const char *)&opt, sizeof(opt)) < 0) {
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
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("Listener on port %d \n", PORT);
    /* TODO: listen()
    * The server listens on the specified port and can queue up to 3 pending 
    * connections for the master socket.
    */
    if (listen(master_socket, BACKLOG) < 0) {
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
            poll_fds[i + 1].fd = client_socket[i];   /* 0 means empty slot, poll ignores it */
            poll_fds[i + 1].events = POLLIN;
            poll_fds[i + 1].revents = 0;
        }

        // Wait for some event on one of the sockets with no timeout (NULL)
        poll_count = poll(poll_fds, max_clients + 1, -1);

        if (poll_count < 0) {
            if (errno == EINTR) {       /* interrupted by a signal: retry instead of exiting */
                continue;
            }
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        // If something happened on the master socket, then it's an incoming connection
        if (poll_fds[0].revents & POLLIN) {
            /* TODO: accept()
            * Accept a new connection when there is incoming request on the master socket.
            */
            new_socket = accept(master_socket, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen);

            /* Errors related to handling a client connection must not make the server exit */
            if (new_socket < 0) {
                perror("accept");
            }
            else {
                printf("New connection, socket fd is %d, ip is: %s, port: %d\n",
                       new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                /* TODO:
                * Add the new client socket to the client_socket array.
                */
                int slot = find_free_slot(client_socket, max_clients);
                if (slot < 0) {
                    /* All slots taken: refuse politely, never write out of bounds */
                    fprintf(stderr, "Max clients (%d) reached, rejecting fd %d\n",
                            max_clients, new_socket);
                    close(new_socket);
                }
                else {
                    client_socket[slot] = new_socket;
                }
            }
        }

        // Loop through all client sockets
        for (int i = 0; i < max_clients; i++) {
            int sd = client_socket[i];

            // Skip if this slot is not connected
            if (sd == 0) continue;

            // Check if the socket has incoming data using poll
            if (poll_fds[i + 1].revents & POLLIN) {
                /* IMPLEMENTING SIMPLE FILE TRANSFER */

                // Variables you may need to declare:
                // char filename[BUFFER_SIZE];
                // FILE *file;
                char filename[BUFFER_SIZE];
                FILE *file;

                /* TODO: Read the filename from the client
                * The client sends the filename first (padded to BUFFER_SIZE)
                */
                /* One message unit is exactly BUFFER_SIZE bytes:
                   first 8 bytes = file length (big-endian uint64), then the filename */
                valread = read_full(sd, buffer, BUFFER_SIZE);
                if (valread < 0) {
                    perror("read filename");
                    close(sd);
                    client_socket[i] = 0;
                    continue;
                }

                /* TODO: Handle client disconnection
                * If read returns 0, the client has disconnected
                */
                if (valread == 0) {
                    printf("Client disconnected, fd %d\n", sd);
                    close(sd);
                    client_socket[i] = 0;
                    continue;
                }
                if (valread < BUFFER_SIZE) {
                    /* Did not receive a full message unit yet: wait for the next poll event */
                    continue;
                }

                /* Decode the 8-byte big-endian length, then take out the filename */
                uint64_t filesize = 0;
                for (int k = 0; k < 8; k++) {
                    filesize = (filesize << 8) | (unsigned char)buffer[k];
                }
                memcpy(filename, buffer + 8, BUFFER_SIZE - 8);
                filename[BUFFER_SIZE - 8 - 1] = '\0';

                /* Keep the base name only: a path from the client must not write elsewhere */
                {
                    char *slash = strrchr(filename, '/');
                    if (slash) {
                        memmove(filename, slash + 1, strlen(slash + 1) + 1);
                    }
                }

                /* TODO: Open the file for writing
                * Use fopen() with "wb" mode for binary write
                */
                int fd = open_file(filename, O_WRONLY | O_CREAT | O_TRUNC);
                file = (fd < 0) ? NULL : fdopen(fd, "wb");

                // Uncomment the following after declaring 'file' and 'filename':
                if (!file) {
                    perror("Error opening file");
                    close(sd);
                    client_socket[i] = 0;
                    continue;
                }
                printf("Receiving file: %s (%llu bytes)\n",
                       filename, (unsigned long long)filesize);

                /* TODO: Receive file data from the client and write to file
                * Loop: read from socket, write to file, until no more data
                */
                /* Receive exactly the length announced by the protocol, stop when full --
                   this does not rely on the peer closing the connection, so files
                   larger than 50MB are handled correctly too */
                uint64_t remaining = filesize;
                char chunk[CHUNK_SIZE];
                int recv_err = 0;

                while (remaining > 0) {
                    size_t want = (remaining > CHUNK_SIZE)
                                  ? (size_t)CHUNK_SIZE : (size_t)remaining;
                    valread = read_full(sd, chunk, want);
                    if (valread <= 0) {
                        recv_err = 1;
                        break;
                    }
                    if (fwrite(chunk, 1, (size_t)valread, file) != (size_t)valread) {
                        perror("Error writing to file");
                        recv_err = 1;
                        break;
                    }
                    remaining -= (uint64_t)valread;
                }

                // Confirm file save and clean up resources
                // Uncomment after implementing:
                if (recv_err) {
                    fprintf(stderr, "Transfer aborted for %s\n", filename);
                } else {
                    printf("File saved as: %s (%llu bytes)\n",
                           filename, (unsigned long long)filesize);
                    if (send_all(sd, "OK", 2) < 0) {
                        perror("send ack");
                    }
                }
                fclose(file);
                close(sd);
                client_socket[i] = 0;
            }
        }
    }

    return 0;
}
