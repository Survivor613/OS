#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Directories to search for executables, changed only by the `path` built-in
vector<string> paths = {"/bin"};

void printError()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Split a line on whitespace (spaces, tabs, ...) into tokens
vector<string> tokenize(const string& line)
{
    vector<string> tokens;
    istringstream stream(line);
    string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Run args as a built-in command if it is one.
// Returns true if it was a built-in (whether or not it succeeded).
bool runBuiltin(const vector<string>& args)
{
    const string& cmd = args[0];

    if (cmd == "exit") {
        if (args.size() != 1) {
            printError();
            return true;
        }
        exit(0);
    }

    if (cmd == "cd") {
        if (args.size() != 2 || chdir(args[1].c_str()) != 0) {
            printError();
        }
        return true;
    }

    if (cmd == "path") {
        // overwrite (not append); `path` with no args leaves it empty
        paths.assign(args.begin() + 1, args.end());
        return true;
    }

    return false;
}

// Look for cmd in each search directory, in order.
// Returns the full path, or "" if it is not found anywhere.
string findExecutable(const string& cmd)
{
    for (const string& dir : paths) {
        string fullPath = dir + "/" + cmd;
        if (access(fullPath.c_str(), X_OK) == 0) {
            return fullPath;
        }
    }
    return "";
}

// Fork a child process that runs args.
// Returns the child's pid, or -1 if the command could not be started.
pid_t launchCommand(const vector<string>& args)
{
    string fullPath = findExecutable(args[0]);
    if (fullPath.empty()) {
        printError();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printError();
        return -1;
    }

    if (pid == 0) {
        // child: build a NULL-terminated argv for execv
        vector<char*> argv;
        for (const string& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execv(fullPath.c_str(), argv.data());

        // execv only returns if it failed. Use _exit so the child never
        // falls back into the shell loop or flushes the parent's buffers.
        printError();
        _exit(1);
    }

    return pid;
}

void processLine(const string& line)
{
    vector<string> args = tokenize(line);
    if (args.empty()) {
        return;
    }

    if (runBuiltin(args)) {
        return;
    }

    pid_t pid = launchCommand(args);
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

int main(int argc, char* argv[])
{
    if (argc > 2) {
        printError();
        exit(1);
    }

    bool interactive = (argc == 1);
    ifstream batchFile;
    if (!interactive) {
        batchFile.open(argv[1]);
        if (!batchFile.is_open()) {
            printError();
            exit(1);
        }
    }
    istream& input = interactive ? cin : batchFile;

    string line;
    while (true) {
        if (interactive) {
            cout << "wish> " << flush;
        }

        if (!getline(input, line)) {
            exit(0);
        }

        processLine(line);
    }
}
