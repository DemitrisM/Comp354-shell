#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>

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
    void ProcessParallelCommands(const string &input); 
    string Trim(const string &str);
    string GetCurrentDirectory();
    string GetUser();
};

#endif