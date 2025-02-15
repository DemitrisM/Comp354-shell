#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
using namespace std;

class Shell {
public:

    void getUserInput();
    vector<string> TokenizeInput(string &input);

private:
    string input;
};

void Shell::getUserInput(){
    while(input!="exit"){
        cout << "Shell:~$ ";
        getline(cin, input);
        cout<<input<<endl;
    }
}
/*Returns a vector containing the strings as tokens*/
vector<string> Shell::TokenizeInput(string &input){
    string token;
    vector<string> tokens;
    istringstream Stream(input);

    while(Stream>>token){
        tokens.push_back(token);
    }
    return tokens;
}

int main(){
    Shell shell;      
    shell.getUserInput();  
    return 0;
}
