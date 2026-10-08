#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

static_assert(sizeof(int) == 4, "The assignment requires a 4-byte int.");

bool writeAll(const void* data, size_t size)
{
    const char* bytes = static_cast<const char*>(data);
    size_t offset = 0;
    while (offset < size) {
        ssize_t n = write(STDOUT_FILENO, bytes + offset, size - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        offset += static_cast<size_t>(n);
    }
    return true;
}

bool emitRepeated(int count, unsigned char character,
                  char* output, size_t& used)
{
    while (count > 0) {
        size_t amount = static_cast<size_t>(count);
        if (amount > 4096 - used) amount = 4096 - used;
        std::memset(output + used, character, amount);
        used += amount;
        count -= static_cast<int>(amount);
        if (used == 4096) {
            if (!writeAll(output, used)) return false;
            used = 0;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        const char message[] = "wunzip: file1 [file2 ...]\n";
        writeAll(message, sizeof(message) - 1);
        exit(1);
    }

    char buffer[4096];
    char output[4096];
    size_t outputUsed = 0;
    unsigned char record[5];
    size_t used = 0;

    // A record may cross a read() boundary or an input-file boundary.
    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) return 1;

        while (true) {
            ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
            if (bytesRead < 0 && errno == EINTR) continue;
            if (bytesRead < 0) {
                close(fd);
                return 1;
            }
            if (bytesRead == 0) break;

            for (ssize_t j = 0; j < bytesRead; ++j) {
                record[used++] = static_cast<unsigned char>(buffer[j]);
                if (used == sizeof(record)) {
                    // get the number (can't use read() func because the pointer position of read() has already changed!!!)
                    int count;
                    std::memcpy(&count, record, sizeof(count));
                    if (count <= 0 ||
                        // record in output buffer
                        !emitRepeated(count, record[4], output, outputUsed)) {
                        close(fd);
                        return 1;
                    }
                    used = 0;
                }
            }
        }
        if (close(fd) < 0) return 1;
    }

    // A leftover partial record is an invalid compressed stream.
    if (used != 0) return 1;

    // flush out left output buffer
    return writeAll(output, outputUsed) ? 0 : 1;
}
