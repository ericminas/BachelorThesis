#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <memory>
#include <array>
#include <regex>

/* complie command:
gcc testingScript.cpp -o testingscript -lstdc++
*/

using namespace std;

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}


int main(int argc, char **argv)
{
    // check that a file path has been provided
    if (argc < 2)
    {
        printf("ERROR: did not provide a file path as the first argument!\n");
        return -1;
    }

  /*   for(int i = 0; i < argc;i++){
        cout<<argv[i]<<endl;
    } */

    // call the program with the file and check the console output
    string cmd = "mp3splt -P " + string(argv[1]) + " 0.0 EOF";
    const char* command = cmd.c_str();


//    system(command);
string console_out = exec(command);

// validate file
// -> if not ogg / stream information could not be decoded
if (console_out.find("file matches the plugin 'ogg vorbis (libvorbis)'") == string::npos || console_out.find("Ogg Vorbis Stream -") == string::npos || console_out.length() == 0)
{
    cout << "Aborted";
    abort();
    return -1;
}
// decoded properly
cout << "OK\n";
return 0;
}