#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

static_assert(sizeof(int) == 4, "The assignment requires a 4-byte int.");

// Write all bytes, including when write() transfers only part of the data.
bool writeAll(const void* data, size_t size)
{
    const char* bytes = static_cast<const char*>(data);
    size_t offset = 0;
    while (offset < size) {
        ssize_t n = write(STDOUT_FILENO, bytes + offset, size - offset); // turn void* into char*
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += static_cast<size_t>(n);
    }
    return true;
}

bool emitRun(int count, char character, char* output, size_t& used)
{
    // flush out 'used' contents together
    if (used + 5 > 4096) {
        if (!writeAll(output, used)) return false;
        used = 0;
    }
    // copy the number (5 in 5b, 4bytes)
    std::memcpy(output + used, &count, sizeof(count));
    // copy the number (b in 5b, 1byte)
    output[used + sizeof(count)] = character;
    used += 5;
    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        const char message[] = "wzip: file1 [file2 ...]\n";
        write(STDOUT_FILENO, message, sizeof(message)-1);
        exit(1);
    }

    char buffer[4096];
    char output[4096];
    size_t outputUsed = 0;
    char previous = 0;
    int count = 0;

    // Keep previous and count across both read() calls and input files.
    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) return 1;

        while (true) {
            ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
            if (bytesRead < 0) {
                close(fd);
                return 1;
            }
            if (bytesRead == 0) break;

            for (ssize_t j = 0; j < bytesRead; ++j) {
                char current = buffer[j];
                if (count == 0) {
                    previous = current;
                    count = 1;
                } else if (current == previous && count < INT_MAX) {
                    ++count;
                } else {
                    // save into the output buffer
                    if (!emitRun(count, previous, output, outputUsed)) {
                        close(fd);
                        exit(1);
                    }
                    previous = current;
                    count = 1;
                }
            }
        }
        close(fd);
    }

    // save the last content into the output buffer and flush out (unprinted + last) contents immediately
    if (count > 0 && !emitRun(count, previous, output, outputUsed)) return 1;
    if (!writeAll(output, outputUsed)) return 1;


    return 0;
}
