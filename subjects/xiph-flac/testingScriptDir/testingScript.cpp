#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <memory>
#include <array>

/* compile command
 gcc testingScriptDir/testingScript.cpp -o testingscript -lstdc++
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

int main(int argc, char **argv){
    // check that a file path has been provided
    if (argc < 2) {
        printf("ERROR: did not provide a file path as the first argument!\n"); 
        abort();
        return -1 ;
    }


    // run the command
	string cmd_t = "flac -t " + string(argv[1]);
	const char* command_t = cmd_t.c_str();
	string cmd_m = "metaflac --list " + string(argv[1]);
	const char* command_m = cmd_m.c_str();

   // string res_t = exec(command_t);
    string res_m = exec(command_m);
    //cout <<"res t:\n" <<res_t<<endl;
	//cout << "res m:\n" << res_m << endl;


    // handle console capture for flac test
    /*if(res_t.length() == 0){
        abort();
	}*/

    // handle console capture for metaflac#
	if(res_m.length() == 0 || res_m.find("There was an error while reading the FLAC file.") != string::npos) {
        abort();
	}

	cout << "file rendered succesfully" << endl;

	return 0;
}