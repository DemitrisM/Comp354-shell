#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <bits/stdc++.h>
using namespace std;

class Shell {
public:
    void getUserInput();

private:
    string input;
    vector<string> TokenizeInput(const string &input);
    void ProcessCommand(const vector<string>& tokens);
};

//Returns a vector containing the strings as tokens
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

    if(tokens[0] == "cd"){
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
}
void Shell::getUserInput(){
    while(true){
        cout << "Shell:~$ ";
        if(!getline(cin, input)){
            break;
        }
        else if(input == "exit"){
            break;
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
