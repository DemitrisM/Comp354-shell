#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <bits/stdc++.h>
#include <sys/wait.h>
using namespace std;

class Shell {
public:
    void getUserInput();

private:
    string input;
    vector<string> TokenizeInput(const string &input);
    void ProcessCommand(const vector<string>& tokens);
    void ProcessLS(const vector<string>& tokens);
};

//returns a vector containing the strings as tokens
vector<string> Shell::TokenizeInput(const string &input){
    string token;
    vector<string> tokens;
    istringstream Stream(input);

    while(Stream>>token){
        tokens.push_back(token);
    }
    return tokens;
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
    return;  // After executing ls, return from ProcessCommand.

}

void Shell::getUserInput(){
    while(true){
        cout << "Shell:~$ ";
        if(!getline(cin, input)){
            return;
        }
        else if(input == "exit"){
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
