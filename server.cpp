#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

int on_receive(char* buffer)
{
    printf("[UDP] Received: %s\n", buffer);
    return 0;
}

int on_update(int* timer_interval)
{
    printf("[TIMER] %d seconds passed. Doing periodic action...\n", *timer_interval);
    // *timer_interval += 1;
    return 0;
}

int main() {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    printf("Server listening on UDP port 8080.\n");

    fd_set readfds;
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);

    time_t last_action = time(NULL);
    int timer_interval = 5; // seconds

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(socket_fd, &readfds);
        int max_fd = socket_fd;

        // Timeout de 1 seconde pour pouvoir faire une action périodique
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(socket_fd, &readfds)) {
            ssize_t len = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
                                   (struct sockaddr*)&client_addr, &addrlen);
            if (len > 0) {
                buffer[len] = '\0';
                on_receive(buffer);
            }
        }

        time_t now = time(NULL);
        if (difftime(now, last_action) >= timer_interval) {
            on_update(&timer_interval);
            last_action = now;
        }
    }

    close(socket_fd);
    return 0;
}
