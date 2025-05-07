//------------------------------------------------------------------------------
// Shell.cpp  -- full implementation with Doxygen comments
//------------------------------------------------------------------------------

#include "Shell.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

//------------------------------------------------------------------------------
/**
 * @brief Trim leading and trailing whitespace from a string.
 *
 * Whitespace characters considered: space, tab, carriage‑return and newline.
 *
 * @param str String to be trimmed.
 * @return A copy of @p str with surrounding whitespace removed.
 */
string Shell::Trim(const string &str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}
/**
 * @brief Execute several sub‑commands in parallel.
 *
 * Splits the raw @p input on the ampersand (‘&’) symbol, tokenises each part,
 * forks once per sub‑command, and waits for all children to finish before
 * returning.
 *
 * @param input Full command line that may contain one or more ‘&’.
 */
void Shell::ProcessParallelCommands(const string &input) {
    vector<string> commands;
    stringstream ss(input);
    string command;
    while(getline(ss, command, '&')) {
        command = Trim(command); 
        if (!command.empty())
            commands.push_back(command);
    }

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

    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, 0);
    }
}

/**
 * @brief Tokenise a command line while respecting single and double quotes.
 *
 * Rules  
 *  • Outside quotes, any run of whitespace ends the current token.  
 *  • Text inside single quotes (') is taken literally.  
 *  • Text inside double quotes (") is taken literally and may coexist with
 *    single‑quote sections.
 *
 * @param input Raw command string.
 * @return Vector of parsed tokens.
 */
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

/**
 * @brief Fork and execute an external program, supporting one < and one > / >>.
 *
 * Recognised redirection operators:  
 *  • `<  file`  – redirect stdin from @p file.  
 *  • `>  file`  – redirect stdout to @p file (truncate).  
 *  • `>> file`  – redirect stdout to @p file (append).  
 *
 * If more than one of the same operator is detected, the shell prints an
 * error and returns.
 *
 * @param tokens Token vector containing command, args, and redirection ops.
 */
void Shell::ProcessExternalCommand(const vector<string>& tokens) {
    if (tokens.empty()) return;  // No command provided
    
    // We'll look for up to ONE input redirection (<) and ONE output redirection (either > or >>).
    size_t inPos = (size_t)-1;
    size_t outPos = (size_t)-1;
    bool appendMode = false; // Distinguish between > vs >> (false for truncate, true for append)

    // 1) Parse tokens to find <, >, or >>
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "<") {
            // If we find multiple <, you might handle errors
            if (inPos != (size_t)-1) {
                cerr << "Error: multiple input redirections not supported\n";
                return;
            }
            inPos = i;
        }
        else if (tokens[i] == ">>") {
            // If we find multiple > or >>, handle error or last-one-wins
            if (outPos != (size_t)-1) {
                cerr << "Error: multiple output redirections not supported\n";
                return;
            }
            outPos = i;
            appendMode = true;
        }
        else if (tokens[i] == ">") {
            if (outPos != (size_t)-1) {
                cerr << "Error: multiple output redirections not supported\n";
                return;
            }
            outPos = i;
            appendMode = false;
        }
    }

    // 2) Extract the filenames from tokens (if any) and build the actual command tokens
    string inputFile, outputFile;
    vector<string> cmdTokens;
    cmdTokens.reserve(tokens.size()); 
    // We'll skip the redirection operators + filenames when building cmdTokens
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i == inPos || i == inPos + 1) continue;
        if (i == outPos || i == outPos + 1) continue;
        cmdTokens.push_back(tokens[i]);
    }

    // If we found <, parse input filename
    if (inPos != (size_t)-1) {
        // Need a token after '<'
        if (inPos + 1 >= tokens.size()) {
            cerr << "Error: no input file after <\n";
            return;
        }
        inputFile = tokens[inPos + 1];
    }

    // If we found > or >>
    if (outPos != (size_t)-1) {
        // Need a token after '>' or '>>'
        if (outPos + 1 >= tokens.size()) {
            cerr << "Error: no output file after > or >>\n";
            return;
        }
        outputFile = tokens[outPos + 1];
    }

    // 3) Convert cmdTokens to a C-style array for execvp
    if (cmdTokens.empty()) {
        // Means user typed just redirection with no actual command, e.g. `< file` alone
        cerr << "Error: no command specified\n";
        return;
    }

    vector<char*> args;
    args.reserve(cmdTokens.size() + 1); // optional
    for (auto &t : cmdTokens) {
        args.push_back(const_cast<char*>(t.c_str()));
    }
    args.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        cerr << "Fork failed\n";
        return;
    }
    else if (pid == 0) {
        // CHILD process: set up redirections if needed

        // a) Input redirection
        if (!inputFile.empty()) {
            int fdIn = open(inputFile.c_str(), O_RDONLY);
            if (fdIn < 0) {
                cerr << inputFile << ": " << strerror(errno) << endl;
                exit(1);
            }
            if (dup2(fdIn, STDIN_FILENO) < 0) {
                cerr << "dup2 stdin failed: " << strerror(errno) << endl;
                exit(1);
            }
            close(fdIn);
        }

        // b) Output redirection
        if (!outputFile.empty()) {
            int flags = O_CREAT | O_WRONLY;
            if (appendMode) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            int fdOut = open(outputFile.c_str(), flags, 0644);
            if (fdOut < 0) {
                cerr << outputFile << ": " << strerror(errno) << endl;
                exit(1);
            }
            if (dup2(fdOut, STDOUT_FILENO) < 0) {
                cerr << "dup2 stdout failed: " << strerror(errno) << endl;
                exit(1);
            }
            // Optional: also redirect stderr to the same file
            // if (dup2(fdOut, STDERR_FILENO) < 0) {
            //     cerr << "dup2 stderr failed: " << strerror(errno) << endl;
            //     exit(1);
            // }
            close(fdOut);
        }

        // c) Exec the command with search through PATH
        if (execvp(args[0], args.data()) == -1) {
            cerr << args[0] << ": command not found" << endl;
            exit(1);
        }
    }
    else {
        // PARENT process: wait for child
        int status;
        waitpid(pid, &status, 0);
    }
}

/**
 * @brief Execute commands from a batch file (non‑interactive mode).
 *
 * Reads @p file line‑by‑line, skips blanks, stores each line in history,
 * tokenises, and dispatches with ProcessCommand().
 *
 * @param file Path to a text file containing one command per line.
 */
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

/**
 * @brief Retrieve the absolute current working directory.
 * @return Directory path on success, or the string \"unknown\" on failure.
 */
string Shell::GetCurrentDirectory(){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return string(cwd);
    } else {
        perror("getcwd failed");
        return "unknown";
    }
}

/**
 * @brief Decide whether a token vector represents a built‑in or an external command.
 *
 * Delegates to the appropriate handler based on the first token:
 * * cd, pwd, bash, exit, echo, history – built‑ins
 * * otherwise – external via ProcessExternalCommand().
 *
 * @param tokens Parsed command tokens.
 */
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

/**
 * @brief Built‑in change‑directory command.
 *
 * * `cd` with no arguments → $HOME  
 * * `cd <dir>`             → change to <dir>  
 * * Any other arity prints a usage error.
 *
 * @param tokens Token vector beginning with \"cd\".
 */
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

/**
 * @brief Built‑in echo command that prints its arguments.
 * @param tokens Token vector beginning with \"echo\".
 */
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

/**
 * @brief Display the stored history of user commands.
 * @param tokens Should contain only the word \"history\".
 */
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

/**
 * @brief Compose the interactive prompt string:  Shell@COMP354:/cwd$
 * @return Prompt string.
 */
string Shell::GetUser(){
    string user = "Shell";
    string host = "COMP354";
    string cwd = GetCurrentDirectory();
    return user + "@" + host + ":" + cwd + "$ ";
}


/**
 * @brief Main interactive REPL loop: print prompt, read line, parse, execute.
 *
 * Stores every non‑blank line into @c history, supports parallel (‘&’) commands
 * as well as single‑command execution, and exits cleanly on EOF.
 */
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
