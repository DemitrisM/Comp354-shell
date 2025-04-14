#include "Shell.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <algorithm>
using namespace std;

string Shell::Trim(const string &str) {
    //Trims the whitespaces from the input
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

void Shell::ProcessParallelCommands(const string &input) {
    //Split the input on '&' to get separate commands
    vector<string> commands;
    stringstream ss(input);
    string command;
    while(getline(ss, command, '&')) {
        //Remove extra whitespace
        command = Trim(command); 
        if (!command.empty())
            commands.push_back(command);
    }

    // Launch each command in its own child process.
    vector<pid_t> pids;
    for (const auto &cmd : commands) {
        vector<string> tokens = TokenizeInput(cmd);
        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Fork failed" << endl;
        } else if (pid == 0) {
            ProcessCommand(tokens);
            exit(0);
        } else {
            pids.push_back(pid);
        }
    }

    //Wait for all child processes to complete
    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, 0);
    }
}

vector<string> Shell::TokenizeInput(const string &input){
    vector<string> tokens;
    string current;
    bool inSingleQuotes = false;
    bool inDoubleQuotes = false;
    
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        // Toggle single-quote mode if we're not in double-quote mode.
        if (c == '\'' && !inDoubleQuotes) {
            inSingleQuotes = !inSingleQuotes;
            continue;
        }
        // Toggle double-quote mode if we're not in single-quote mode.
        else if (c == '\"' && !inSingleQuotes) {
            inDoubleQuotes = !inDoubleQuotes;
            continue;
        }
        // If we're not in quotes and see whitespace, that ends a token.
        if (!inSingleQuotes && !inDoubleQuotes && isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } 
        else {
            // Add the character to the current token.
            current.push_back(c);
        }
    }
    // If there's any leftover token after the loop, add it.
    if (!current.empty()) {
        tokens.push_back(current);
    }
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
            cerr << args[0] << ": command not found" << endl;
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
        if (line.empty()) continue;
        //Push to history
        history.push_back(line);
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
    cerr << "DEBUG: tokens[0] is ->" << tokens[0] << "<-\n";
for (unsigned char c : tokens[0]) {
    cerr << (int)c << " ";
}
cerr << endl;

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

    } else if (tokens[0] == "echo"){
        ProcessEcho(tokens);
        return;

    } else if (tokens[0] == "history"){
        HandleHistory(tokens);
        return;

    } else {
        //For any command that isn't built-in, process it as an external command
        ProcessExternalCommand(tokens);
    }
}

void Shell::ProcessCD(const vector<string>& tokens){
    if (tokens.size() == 1) {
        // No argument -> go HOME
        const char* home = getenv("HOME");
        if (!home || chdir(home) != 0) {
            cerr << "cd: Failed to change directory to HOME.\n";
        }
    } else if (tokens.size() == 2) {
        // 1 argument
        if (chdir(tokens[1].c_str()) != 0) {
            cerr << "cd: Failed to change directory to " << tokens[1] << endl;
        }
    } else {
        // Too many arguments
        cerr << "Usage: cd [dir]\n";
    }
}

void ProcessEcho(const vector<string> &tokens) {
    if (tokens.size() == 1) {
        // echo with no arguments -> just print a blank line
        cout << endl;
        return;
    }
    
    // Print tokens[1..end]
    for (size_t i = 1; i < tokens.size(); ++i) {
        cout << tokens[i];
        if (i + 1 < tokens.size()) {
            cout << " ";
        }
    }
    cout << endl;
}

void Shell::HandleHistory(const vector<string> &tokens) {
    if (tokens.size() != 1) {
        cerr << "Usage: history" << endl;
        return;
    }
    // Print all stored commands
    for (size_t i = 0; i < history.size(); ++i) {
        cout << i + 1 << "  " << history[i] << endl;
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
        // Remove all '\r' characters from the end-user input
        input.erase(remove(input.begin(), input.end(), '\r'), input.end());
        // Push to history
        if(!input.empty()) {
            history.push_back(input);
        }
        if(input.find('&') != string::npos) {
            ProcessParallelCommands(input);
        } else {
            vector<string> tokens = TokenizeInput(input);
            ProcessCommand(tokens);
        }
    }
}
