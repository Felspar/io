#include <sys/socket.h>


int detect_accept4(int fd) {
    return ::accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
}
