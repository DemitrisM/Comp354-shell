#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <bits/stdc++.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h> 
using namespace std;

class Shell {
public:
    void GetUserInput();
    void ProcessBatchFile(const string &file);

private:
    string input;
    vector<string> TokenizeInput(const string &input);
    void ProcessCommand(const vector<string>& tokens);
    void ProcessLS(const vector<string>& tokens);
    void ProcessCD(const vector<string>& tokens);
    void ProcessExternalCommand(const vector<string>& tokens);
    string GetCurrentDirectory();
    string GetUser();
};

vector<string> Shell::TokenizeInput(const string &input){
    string token;
    vector<string> tokens;
    istringstream Stream(input);

    while(Stream>>token){
        tokens.push_back(token);
    }
    //returns a vector containing the strings as tokens
    return tokens;
}

void Shell::ProcessExternalCommand(const vector<string>& tokens) {
    // Find the position of the redirection operator ">" if it exists.
    size_t redirPos = -1;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == ">") {
            redirPos = i;
            break;
        }
    }
    // Prepare a vector for the actual command arguments (excluding redirection parts).
    vector<string> cmdTokens;
    string outputFile;
    
    if (redirPos != (size_t)-1) {
        // Ensure that there is exactly one token after ">"
        if (redirPos + 1 >= tokens.size() || redirPos + 2 < tokens.size()) {
            cerr << "Error: invalid redirection" << endl;
            return;
        }
        // Command tokens are all tokens before the redirection operator.
        for (size_t i = 0; i < redirPos; i++) {
            cmdTokens.push_back(tokens[i]);
        }
        // The token immediately after ">" is the output filename.
        outputFile = tokens[redirPos + 1];
    } else {
        // No redirection; use all tokens.
        cmdTokens = tokens;
    }
    
    // Convert cmdTokens to a C-style array for execvp.
    vector<char*> args;
    for (const auto &token : cmdTokens) {
        args.push_back(const_cast<char*>(token.c_str()));
    }
    args.push_back(nullptr);
    
    // Fork a new process.
    pid_t pid = fork();
    if (pid < 0) {
        cerr << "Fork failed" << endl;
        return;
    } else if (pid == 0) {
        // In the child process, if redirection is required, set it up.
        if (!outputFile.empty()) {
            // Open the output file (create/truncate it).
            int fd = open(outputFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            // Redirect stdout (fd 1) to the file descriptor.
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout failed");
                exit(1);
            }
            // Optionally, redirect stderr (fd 2) to the file as well:
            if (dup2(fd, STDERR_FILENO) < 0) {
                perror("dup2 stderr failed");
                exit(1);
            }
            // Close the file descriptor, as it's no longer needed directly.
            close(fd);
        }
        // Execute the external command.
        if (execvp(args[0], args.data()) == -1) {
            perror("execvp failed");
            exit(1);
        }
    } else {
        // In the parent process, wait for the child to finish.
        int status;
        waitpid(pid, &status, 0);
    }
}

void Shell::ProcessBatchFile(const string &file) {
    ifstream batchFile(file);
    if (!batchFile.is_open()) {
        cerr << "Error: Cannot open batch file: " << file << endl;
        return;
    }
    
    string line;
    //If batch mode is invoked from within our shell,don't print a prompt.
    while(getline(batchFile, line)) {
        //Skip empty lines.
        if (line.empty())
            continue;
        vector<string> tokens = TokenizeInput(line);
        ProcessCommand(tokens);
    }
    batchFile.close();
}

string Shell::GetCurrentDirectory(){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return string(cwd);
    } else {
        perror("getcwd failed");
        return "unknown";
    }
}

void Shell::ProcessCommand(const vector<string>& tokens) {
    if (tokens.empty())
        return;

    if (tokens[0] == "cd") {
        ProcessCD(tokens);
    } else if (tokens[0] == "pwd") {
        if (tokens.size() != 1) {
            cerr << "Error" << endl;
            return;
        }
        cout << GetCurrentDirectory() << endl;
    } else if (tokens[0] == "bash") {
        if (tokens.size() != 2) {
            cerr << "Error" << endl;
            return;
        }
        ProcessBatchFile(tokens[1]);
    } else if (tokens[0] == "exit") {
        if (tokens.size() != 1) {
            cerr << "Error" << endl;
            return;
        }
        exit(0);
    } else {
        //For any command that isn't built-in, process it as an external command.
        ProcessExternalCommand(tokens);
    }
}

void Shell::ProcessLS(const vector<string>& tokens){
    //execv() requires a C-style array of char* so we convert the vector of string into that
    vector<char*> args;
    //converts each string in the tokens vector into a non const char
    for (const auto &token : tokens) {
        args.push_back(const_cast<char*>(token.c_str()));
    }
    //adds a null pointer at the end of the args vector
    args.push_back(nullptr);
    pid_t pid = fork();
    if (pid < 0) {
        //if fork() returns a negative value, output an error message.
        cerr << "Fork failed" << endl;
        return;
    } else if (pid == 0) {
        // we assume that ls is located at /bin/ls,if it succeeds, the current process image is replaced by the ls program,
        // and nothing below this line in the child program is executed, if it fails it returns -1 
        if (execv("/bin/ls", args.data()) == -1) {
            perror("execv failed");
            exit(1);
        }
    } else {
        //wait for the child proceess to finish executing
        int status;
        waitpid(pid, &status, 0);
    }
    // After executing ls, return from ProcessCommand.
    return;  

}

void Shell::ProcessCD(const vector<string>& tokens){
    if (tokens.size() == 1){
        //retrieves the value of the home directory
        const char* home = getenv("HOME");
        //if home is not set or we werent able to go the home directory, get an error message
        if (home == nullptr || chdir(home) != 0) {
            cerr << "Error" << endl;
        }
    }
    else if(tokens.size() == 2){
        if(chdir(tokens[1].c_str()) !=0){
            cerr<<"Error"<<endl;
        }
    }
    else{
        cerr<<"Error"<<endl;
    }
}

string Shell::GetUser(){
    string user = "Shell";
    string host = "COMP354";
    string cwd = GetCurrentDirectory();
    return user + "@" + host + ":" + cwd + "$ ";
}

void Shell::GetUserInput(){
    while(true){
        cout << GetUser();
        if(!getline(cin, input)){
            return;
        }
        vector<string> tokens = TokenizeInput(input);
        ProcessCommand(tokens);
    }
}

int main(int argc, char* argv[]) {
    Shell shell;
    if (argc == 1) {
        // Interactive mode.
        shell.GetUserInput();
    } else if (argc == 2) {
        // Batch mode.
        shell.ProcessBatchFile(argv[1]);
    } else {
        cerr << "Error: Too many arguments." << endl;
        exit(1);
    }
    return 0;
}
