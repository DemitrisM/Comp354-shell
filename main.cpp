#include "Shell.h"
#include <cstdlib>
#include <iostream>
using namespace std;

int main(int argc, char* argv[]) {
    Shell shell;
    if (argc == 1) {
        // Interactive mode
        shell.GetUserInput();
    } else if (argc == 2) {
        // Batch mode - run commands from batch file.
        shell.ProcessBatchFile(argv[1]);
    } else {
        cerr << "Error: Too many arguments." << endl;
        exit(1);
    }
    return 0;
}
