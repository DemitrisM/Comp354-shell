//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------
/**
 * @file main.cpp
 * @brief Entry point that launches the custom shell in interactive or batch mode.
 */
#include "Shell.h"
#include <cstdlib>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    Shell shell;
    if (argc == 1) {
        shell.GetUserInput();              //!< Interactive REPL
    } else if (argc == 2) {
        shell.ProcessBatchFile(argv[1]);   //!< Batch mode from file
    } else {
        cerr << "Error: Too many arguments." << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
