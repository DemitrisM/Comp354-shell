//------------------------------------------------------------------------------
// Shell.h
//------------------------------------------------------------------------------
/**
 * @file Shell.h
 * @brief Declaration of a minimal Unix‑like shell class.
 *
 * The Shell class provides an interactive or batch‑mode command‑line interface
 * that supports built‑in commands (cd, pwd, history, echo, exit), external
 * command execution via `fork()`/`execvp()`, I/O redirection (<, >, >>),
 * and simple parallel execution with the ampersand (&) operator.
 */

 #ifndef SHELL_H
 #define SHELL_H
 
 #include <string>
 #include <vector>
 
 using namespace std; //!< project keeps std names unqualified by convention
 
 /**
  * @class Shell
  * @brief A very small Unix‑like shell implemented in C++.
  */
 class Shell {
 public:
     /** Interactive REPL that repeatedly reads a line, parses, and executes. */
     void GetUserInput();
 
     /** Execute commands from a batch file (non‑interactive mode). */
     void ProcessBatchFile(const string& file);
 
 private:
     // ---------------------------------------------------------------------
     // Core parsing helpers
     // ---------------------------------------------------------------------
     /** Tokenise @p input into whitespace‑separated fields with quote support. */
     vector<string> TokenizeInput(const string& input);
 
     /** Trim leading/trailing whitespace (space, tab, CR, LF) from @p str. */
     string Trim(const string& str);
 
     // ---------------------------------------------------------------------
     // Command dispatchers
     // ---------------------------------------------------------------------
     /** Top‑level dispatcher: decide whether @p tokens form a built‑in or exec. */
     void ProcessCommand(const vector<string>& tokens);
 
     /** Handle built‑in history command. */
     void HandleHistory(const vector<string>& tokens);
 
     /** Handle built‑in cd command. */
     void ProcessCD(const vector<string>& tokens);
 
     /** Handle built‑in echo command. */
     void ProcessEcho(const vector<string>& tokens);
 
     /** Fork/exec path for external commands, with I/O redirection support. */
     void ProcessExternalCommand(const vector<string>& tokens);
 
     /** Launch multiple commands in parallel, delimited by '&'. */
     void ProcessParallelCommands(const string& input);
 
     // ---------------------------------------------------------------------
     // Utility helpers
     // ---------------------------------------------------------------------
     /** @return absolute path of current working directory or "unknown". */
     string GetCurrentDirectory();
 
     /** @return formatted prompt string (e.g., Shell@COMP354:/path$). */
     string GetUser();
 
     // ---------------------------------------------------------------------
     // Data members
     // ---------------------------------------------------------------------
     string input;                 //!< Current raw line read from stdin.
     vector<string> history;       //!< Rolling list of commands entered.
 };
 
 #endif // SHELL_H