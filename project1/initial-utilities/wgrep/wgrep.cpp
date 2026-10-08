#include <fcntl.h>
#include <iostream>
#include <unistd.h>

using namespace std;

int checkLine(const string& searchTerm, string& line)
{
    if (!line.empty() && line.find(searchTerm) != std::string::npos){
        write(STDOUT_FILENO, line.data(), line.size());
    }

    return 0;
}

int handleGrep(const string& searchTerm,
                ssize_t bytesRead,
                char* buffer,
                string& line)
{
    if (bytesRead < 0) {
        return -1;
    }

    if (bytesRead == 0){
        checkLine(searchTerm, line);
        line.clear();
        return 0;
    }

    for (ssize_t i=0; i<bytesRead; i++){
        // line not accomplished
        line.push_back(buffer[i]);

        // line finished
        // try finding matching strings in the line
        if (buffer[i] == '\n'){
            checkLine(searchTerm, line);
            line.clear(); 
        }
    }

    return 1;
}

int main(int argc, char* argv[])
{
    if (argc < 2){
        const char message[] = "wgrep: searchterm [file ...]\n";
            write(STDOUT_FILENO, message, sizeof(message)-1);
            exit(1);
    }

    char buffer[4096];

    if (argc == 2){
        ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
        
        std::string line;

        int result = handleGrep(argv[1], bytesRead, buffer, line);
        if (result < 0)
            return 1;

        if (result == 0)
            return 0;
        
    }

    for (int i=2; i<argc; i++)
    {
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor < 0) 
        {
            const char message[] = "wgrep: cannot open file\n";
            write(STDOUT_FILENO, message, sizeof(message)-1);
            exit(1);
        }
        
        std::string line;

        while (true) {
            ssize_t bytesRead = read(fileDescriptor, buffer, sizeof(buffer));

            int result = handleGrep(argv[1], bytesRead, buffer, line);
            if (result < 0){
                close(fileDescriptor);
                exit(1);
            }

            if (result == 0)
                break;
        }

        close(fileDescriptor);
    }

    return 0;
}