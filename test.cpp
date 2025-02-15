#include <iostream>
#include <string>
using namespace std;

class Shell {
public:

    void getUserInput();

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

int main(){
    Shell shell;      
    shell.getUserInput();  
    return 0;
}
