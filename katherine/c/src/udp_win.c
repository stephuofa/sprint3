/* Katherine Control Library
 *
 * This file was created on 31.8.18 by Felix Lehner.
 * This file was modified on 13.2.19 by Petr Manek.
 *
 * Contents of this file are copyrighted and subject to license
 * conditions specified in the LICENSE file located in the top
 * directory.
 */

#include <katherine/global.h>

void empty_method() {}

#ifdef KATHERINE_WIN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <katherine/udp.h>

#ifdef KATHERINE_DEBUG_UDP
static inline void
dump_buffer(const char *msg, const unsigned char *buf, size_t count)
{
    printf("%-10s ", msg);

#if KATHERINE_DEBUG_UDP >= 2
    if (count < 60000) {
        for (size_t i = 0; i < count; i++) {
            printf("%02X ", buf[i]);
            if (count != 8 && i % 6 == 5) {
                printf("\n");
            }
        }
        printf("\n");
    }
#endif

    printf("(%ld bytes)\n", count);
}
#endif /* KATHERINE_DEBUG_UDP */

/**
 * Initialize new UDP session.
 * @param u UDP session to initialize
 * @param local_port Local port number
 * @param remote_addr Remote IP address
 * @param remote_port Remote port number
 * @param timeout_ms Communication timeout in milliseconds (zero if disabled)
 * @return Error code.
 */
int
katherine_udp_init(katherine_udp_t *u, uint16_t local_port, const char *remote_addr, uint16_t remote_port, uint32_t timeout_ms)
{
    int res = 0;

    // Create communication buffer.
    if ((res = WSAStartup(MAKEWORD(2, 2), &u->wsa_data)) != 0) {
        printf("Cant startup\n");
        res = WSAGetLastError();
        goto err_wsa_data;
    }

    // Create socket.
    if ((u->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        res = WSAGetLastError();
          printf("Cant create sock\n");
        goto err_socket;
    }

    // Setup and bind the socket address.
    u->addr_local.sin_family = AF_INET;
    u->addr_local.sin_port = htons(local_port);
    u->addr_local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(u->sock, (const struct sockaddr*) &u->addr_local, sizeof(u->addr_local)) == SOCKET_ERROR) {
        res = WSAGetLastError();
          printf("Cant bind: %i\n",res);
        goto err_bind;
    }

    if (timeout_ms > 0) {
        // Set socket timeout.
        DWORD timeout = timeout_ms;
        if (setsockopt(u->sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout)) == SOCKET_ERROR) {
            res = WSAGetLastError();
              printf("Cant set sock opt\n");
            goto err_timeout;
        }
    }

    int requested_recv_buf_size = 1024 * 1024 * 2; // Try 2 MB
    if (setsockopt(u->sock, SOL_SOCKET, SO_RCVBUF, (char*)&requested_recv_buf_size, sizeof(requested_recv_buf_size)) == SOCKET_ERROR) {
        res = WSAGetLastError();
        printf("Cant set sock opt SO_RCVBUF: %i\n", res);
    }

    int actual_recv_buf_size;
    int optlen_check = sizeof(actual_recv_buf_size);
    if (getsockopt(u->sock, SOL_SOCKET, SO_RCVBUF, (char*)&actual_recv_buf_size, &optlen_check) == SOCKET_ERROR) {
        res = WSAGetLastError();
        printf("Cant get sock opt SO_RCVBUF: %i\n", res);
    } else {
        printf("Requested recv buffer size: %u bytes\n", requested_recv_buf_size);
        printf("Actual recv buffer size: %u bytes\n", actual_recv_buf_size);
    }
// --- END ADDED SECTION ---

// Setup and bind the socket address.
u->addr_local.sin_family = AF_INET;
// ... rest of your code

    // Set remote socket address.
    u->addr_remote.sin_family = AF_INET;
    u->addr_remote.sin_port = htons(remote_port);
    if (inet_pton(AF_INET, remote_addr, &u->addr_remote.sin_addr) <= 0) {
        res = WSAGetLastError();
          printf("Cant set remote\n");
        goto err_remote;
    }

    if ((u->mutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
        res = GetLastError();
          printf("Cant create mutex\n");
        goto err_mutex;
    }

    return res;

err_mutex:
err_remote:
err_timeout:
err_bind:
    (void) closesocket(u->sock);
err_socket:
    (void) WSACleanup();
err_wsa_data:
    return res;
}

/**
 * Finalize UDP session.
 * @param u UDP session to finalize
 */
void
katherine_udp_fini(katherine_udp_t *u)
{
    // Ignoring return codes below.
    int res =  closesocket(u->sock);
    if (res!=0){
        printf("Error closing socket %#x\n",WSAGetLastError());
    }

    boolean x = CloseHandle(u->mutex);
    if (x==0){
        printf("Error closing handle: %#x\n",GetLastError());
    }

    int res2 = WSACleanup();
    if (res2!=0){
        printf("Error during WSA Cleanup: %#x\n",WSAGetLastError());
    }
}

/**
 * Send a message (unreliable).
 * @param u UDP session
 * @param data Message start
 * @param count Message length in bytes
 * @return Error code.
 */
int
katherine_udp_send_exact(katherine_udp_t* u, const void* data, size_t count)
{
    size_t sent;
    size_t total = 0;
    const char *cdata = (const char*) data;

    do {
        sent = sendto(u->sock, cdata + total, count - total, 0, (struct sockaddr*) &u->addr_remote, sizeof(u->addr_remote));
        if (sent == SOCKET_ERROR) {
            return WSAGetLastError();
        }

        total += sent;
    } while (total < count);

#ifdef KATHERINE_DEBUG_UDP
    dump_buffer("Sent:", data, count);
#endif /* KATHERINE_DEBUG_UDP */

    return 0;
}

/**
 * Receive a message (unreliable).
 * @param u UDP session
 * @param data Inbound buffer start
 * @param count Inbound buffer size in bytes
 * @return Error code.
 */
int
katherine_udp_recv_exact(katherine_udp_t* u, void* data, size_t count)
{
    size_t received;
    size_t total = 0;
    socklen_t addr_len = sizeof(u->addr_remote);
    char *cdata = (char*) data;

    while (total < count) {
        received = recvfrom(u->sock, cdata + total, count - total, 0, (struct sockaddr *) &u->addr_remote, &addr_len);
        if (received == SOCKET_ERROR) {
            return WSAGetLastError();
        }

        total += received;
    }

#ifdef KATHERINE_DEBUG_UDP
    dump_buffer("Received:", data, received);
#endif /* KATHERINE_DEBUG_UDP */

    return 0;
}

/**
 * Receive a portion of a message (unreliable).
 * @param u UDP session
 * @param data Inbound buffer start
 * @param count Inbound buffer size in bytes
 * @return Error code.
 */
int
katherine_udp_recv(katherine_udp_t* u, void* data, size_t* count)
{
    socklen_t addr_len = sizeof(u->addr_remote);
    char *cdata = (char*) data;
    size_t received = recvfrom(u->sock, cdata, *count, 0, (struct sockaddr *) &u->addr_remote, &addr_len);

    if (received == SOCKET_ERROR) {
        return WSAGetLastError();
    }

#ifdef KATHERINE_DEBUG_UDP
    dump_buffer("Received:", data, received);
#endif /* KATHERINE_DEBUG_UDP */

    *count = (size_t) received;
    return 0;
}

/**
 * Lock mutual exclusion synchronization primitive.
 * @param u UDP session
 * @return Error code.
 */
int
katherine_udp_mutex_lock(katherine_udp_t *u)
{
    return WaitForSingleObject(u->mutex, INFINITE);
}

/**
 * Unlock mutual exclusion synchronization primitive.
 * @param u UDP session
 * @return Error code.
 */
int
katherine_udp_mutex_unlock(katherine_udp_t *u)
{
    return ReleaseMutex(u->mutex);
}

#endif /* KATHERINE_WIN */
