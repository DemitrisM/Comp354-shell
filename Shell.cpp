#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <bits/stdc++.h>
#include <sys/wait.h>
#include <limits.h>
using namespace std;

class Shell {
public:
    void getUserInput();
    void ProcessBatchFile(const string &file);

private:
    string input;
    vector<string> TokenizeInput(const string &input);
    void ProcessCommand(const vector<string>& tokens);
    void ProcessLS(const vector<string>& tokens);
    void ProcessCD(const vector<string>& tokens);
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

void Shell::ProcessCommand(const vector<string>& tokens){
    //Nothing entered
    if (tokens.empty()){
        return;
    }

    if (tokens[0] == "ls") {
        ProcessLS(tokens);
    }
    
    else if(tokens[0] == "cd"){
        ProcessCD(tokens);   
    }

    else if(tokens[0] == "pwd"){
        if (tokens.size() != 1){
            cerr<<"Error"<<endl;
            return;
        }
        cout<<GetCurrentDirectory()<<endl;
    }
    else if(tokens[0] == "bash"){
        if(tokens.size() != 2){
            cerr<<"Error"<<endl;
            return;
        }
        ProcessBatchFile(tokens[1]);
    }
    else if(tokens[0] == "exit"){
        if (tokens.size() != 1){
            cerr<<"Error"<<endl;
            return;
        }
        exit(0);
    }

    else{
        cout <<tokens[0]<< ": Command not found " << endl;
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

void Shell::getUserInput(){
    while(true){
        cout << GetUser();
        if(!getline(cin, input)){
            return;
        }
        vector<string> tokens = TokenizeInput(input);
        ProcessCommand(tokens);
    }
}

int main(){
    Shell shell;      
    shell.getUserInput(); 
    return 0;
}
