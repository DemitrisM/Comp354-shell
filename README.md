# Custom UNIX shell
This project is a simple Unix shell implemented in C++. It is designed as a learning tool to understand core concepts of Unix shell functionality, such as process creation, process management, command parsing, and inter-process communication.
## Features Implemented

- Interactive Mode:
  The shell runs in an interactive loop. It displays a dynamic prompt (showing a fixed username "Shell", a fixed host "COMP354", and the current working directory), reads user input, tokenizes the command, and then processes it.
    
 - Built-in Commands:
    The shell supports several built-in commands:
    - **cd** : Changes the current working directory. If no argument or an empty argument is provided, it defaults to the home directory.
    - **pwd**: Prints the current working directory.
    - **exit**: Exits the shell.
    - **bash** [batch_file]: Executes commands from a specified batch file. (This is our custom method to trigger batch mode.)

  - External Command Execution:
    External commands (e.g., ls) are executed using fork() and execvp(). The shell tokenizes user input and passes the arguments to the external program.

  - Batch Mode:
    Batch mode is implemented as a built-in command (bash). When invoked (for example, by typing bash file.txt), the shell opens the given file, reads commands line by line, tokenizes them, and processes each command sequentially without printing a prompt.
    
```bash
bash file.txt
```
  - Parallel Command Execution:
    The shell supports executing multiple commands in parallel using the ampersand (&) operator. When an input line contains &, the shell splits it into separate commands, executes each in its own child process concurrently, and waits for all of them to complete before returning control to the user.
```bash
sleep 3 & echo "Hello" & ls
```

## How to Compile and Run
Compilation

Use a C++ compiler (e.g., g++) to compile the source code. For example:

```bash
g++ -std=c++11 -o custom_shell Shell.cpp
```


(Replace Shell.cpp with the name of your source file.)
