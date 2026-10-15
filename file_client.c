// file_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024


/* Send exactly n bytes: send() may transmit only part of the buffer (short write) */
static int send_all(int fd, const void* buf, size_t n)
{
    const char* p = (const char*)buf;
    size_t sent = 0;

    while (sent < n) {
        ssize_t k = send(fd, p + sent, n - sent, 0);
        if (k < 0) {
            return -1;
        }
        sent += (size_t)k;
    }
    return 0;
}

/* Read exactly n bytes: read() may return fewer bytes than requested (short read) */
static int read_full(int fd, void* buf, size_t n)
{
    char* p = (char*)buf;
    size_t got = 0;

    while (got < n) {
        ssize_t k = read(fd, p + got, n - got);
        //0 = peer closed, -1 = error 
        if (k <= 0) {          
            return (int)k;
        }
        got += (size_t)k;
    }
    return (int)got;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in serv_addr;
    int sock = 0;
    char buffer[BUFFER_SIZE] = {0};

    /* TODO: socket()
    * Create a socket using socket(AF_INET, SOCK_STREAM, 0)
    * Store the result in 'sock' variable
    * Print an error message and exit if it fails
    */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    /* TODO: inet_pton()
    * Convert the IP address "127.0.0.1" from text to binary form
    * Use inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
    * Print an error message and exit if conversion fails (returns <= 0)
    */
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton: invalid address / not supported\n");
        close(sock);
        return EXIT_FAILURE;
    }

    

    /* TODO: connect()
    * Connect to the server.
    */
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return EXIT_FAILURE;
    }

    /* IMPLEMENTING SIMPLE FILE TRANSFER */

    // Variables you may need to declare:
    // FILE *file;
    // char *base_filename;
    FILE* file = NULL;
    char* base_filename = NULL;
    char chunk[BUFFER_SIZE];


    /* TODO: Open file for reading in binary mode
    * Use fopen() with "rb" mode
    */
    file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        close(sock);
        return EXIT_FAILURE;
    }

    /* TODO: Extract filename from path (argv[1])
    * Use strrchr() to find the last '/' in the path
    * Example: "../file1.zip" -> "file1.zip"
    * If no '/' found, use the entire path as filename
    * Store result in base_filename
    */
    base_filename = strrchr(argv[1], '/');
    if (base_filename) {
        base_filename++;                 
    }
    else {
        base_filename = argv[1];         
    }

    long long fsize = 0;
    if (fseek(file, 0L, SEEK_END) == 0) {
        fsize = (long long)ftell(file);
        fseek(file, 0L, SEEK_SET);
        

    }

    /* TODO: Send filename to server
    * Send the filename (padded to BUFFER_SIZE) so server knows what to name the file
    */

    memset(buffer, 0, sizeof(buffer));
    // Get the file size and store it in buffer[0..7] in big-endian order
    {
        uint64_t v = (uint64_t)fsize;
        for (int i = 7; i >= 0; i--) {
            buffer[i] = (char)(v & 0xFFu);
            v >>= 8;
        }
    }

    /*Store the filename in buffer as well; the + 8 offset places it right after the
    8-byte size field. Layout: [8-byte size][filename] */ 
    strncpy(buffer + 8, base_filename, BUFFER_SIZE - 8 - 1);

    if (send_all(sock, buffer, BUFFER_SIZE) < 0) {
        fprintf(stderr, "send filename failed\n");
        fclose(file);
        close(sock);
        return EXIT_FAILURE;
    }

    // You can add a small delay to avoid data mix-up

    /* TODO: Send file content
    * Read file in chunks of BUFFER_SIZE and send each chunk
    */
    for (;;) {
        size_t nread = fread(chunk, 1, sizeof(chunk), file);   /* use the real byte count */
        if (nread > 0) {
            if (send_all(sock, chunk, nread) < 0) {
                fprintf(stderr, "send file content failed\n");
                fclose(file);
                close(sock);
                return EXIT_FAILURE;
            }
        }
        if (nread < sizeof(chunk)) {       /* end of file (also covers an exactly-full read) */
            break;
        }
    }

    // printf("File '%s' uploaded successfully.\n", base_filename);
    {
        char reply[BUFFER_SIZE] = { 0 };
        int n = read_full(sock, reply, 2);   /* wait for the server's "OK" receipt */
        if (n == 2) {
            reply[2] = '\0';
            printf("File '%s' uploaded successfully (%lld bytes), server replied: %s\n",
                base_filename, fsize, reply);
        }
        else {
            printf("File '%s' sent (%lld bytes), but no server confirmation\n",
                base_filename, fsize);
        }
    }

    /* TODO: Close file and socket */
    fclose(file);
    close(sock);

    return 0;
}