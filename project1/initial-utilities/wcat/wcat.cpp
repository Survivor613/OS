#include <fcntl.h>
#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
    char buffer[4096];

    for (int i=1; i<argc; i++)
    {
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor < 0) 
        {
            const char message[] = "wcat: cannot open file\n";
            write(STDOUT_FILENO, message, sizeof(message)-1);
            exit(1);
        }
        
        while (true) {
            ssize_t bytesRead = read(fileDescriptor, buffer, sizeof(buffer));

            if (bytesRead < 0) {
                
                close(fileDescriptor);
                exit(1);
            }

            if (bytesRead == 0){
                break;
            }

            write(STDOUT_FILENO, buffer, bytesRead);
        }
    }
}