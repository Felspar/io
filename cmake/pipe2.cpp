#include <fcntl.h>
#include <unistd.h>


int detect_pipe2(int fds[2]) { return ::pipe2(fds, O_NONBLOCK); }
