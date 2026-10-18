#include <fcntl.h>

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <string>
#include <vector>
#include <sys/wait.h>


using namespace std;

vector<string> paths = {"/bin"};

struct Command {
    vector<string> args;
    string outputFile; // empty if no redirection
};

void printError()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

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

vector<string> split(const string& line, char delim)
{
    vector<string> parts;
    istringstream stream(line);
    string part;
    while (getline(stream, part, delim)) {
        parts.push_back(part);
    }
    return parts;
}

bool parseCommand(const string& line, Command& cmd)
{
    size_t pos = line.find('>');
    if (pos == string::npos) {
        cmd.args = tokenize(line);
        return true;
    }

    // exist '>' after the first '>', i.e. more than 1 '>'
    if ((line.find('>', pos + 1)) != string::npos) {
        return false;
    }

    cmd.args = tokenize(line.substr(0, pos));
    vector<string> files = tokenize(line.substr(pos + 1));

    // can only have 1 redirection file after '>'
    if (cmd.args.empty() || files.size() != 1) {
        return false;
    }

    cmd.outputFile = files[0];
    return true;
}

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

pid_t launchCommand(const Command& cmd)
{
    const vector<string>& args = cmd.args;
    string fullPath = findExecutable(args[0]);
    if (fullPath.empty()) { // cannot find
        printError();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printError();
        return -1;
    }

    if (pid == 0) {
        // child
        // redirection
        if (!cmd.outputFile.empty()) {
            int fd = open(cmd.outputFile.c_str(),
                                    O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0){
                printError();
                _exit(1);
            }
            dup2(fd, STDERR_FILENO);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        //execv
        vector<char*> argv;
        for (const string& arg : args){
            // execv needs pointers to content
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execv(fullPath.c_str(), argv.data());

        printError();
        _exit(1);
    }

    return pid;
}

void processLine(const string& line)
{
    vector<Command> commands;
    for (const string& segment : split(line, '&')) {
        Command cmd;
        if (!parseCommand(segment, cmd)) {
            printError();
            return;
        }
    
        if (!cmd.args.empty()) {
            commands.push_back(cmd);
        }
    }

    vector<pid_t> pids;
    for (const Command& cmd : commands) {
        if (runBuiltin(cmd.args)) {
            continue;
        }
        
        pid_t pid = launchCommand(cmd);
        if (pid > 0) {
            pids.push_back(pid);
        }
    }
    
    for (pid_t pid : pids)
        waitpid(pid, nullptr, 0);
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