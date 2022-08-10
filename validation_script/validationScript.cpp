#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <memory>
#include <array>
#include <regex>

using namespace std;

// global vars
const string oggInfoPath = "~/Desktop/Thesis/validationTools/vorbis-tools/ogginfo/ogginfo ";

/* DOCUMENTATION
* There are two variables that have to be provided:
* numberOfGeneratedFiles    {int}       - The number of files that should be generated for each format.
* pathToFuzzers             {string}    - The path to the parent directory of both fuzzers.
* onlyMode                  {string}    - Whether only one format should be tested and if so, which.
*
* Notes:
* - numberOfGeneratedFiles  - Range: 1 - 10^6.
* - pathToFuzzers           - The fuzzers should be called: "ogg-fuzzer" and "flac-fuzzer".
* - onlyMode                - The file format in lowercase or nothing.
*/

string exec(const char* cmd) {
    char buffer[128];
    string result = "";
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

int main(int argc, char** argv) {
    // argument validation
    if(argc < 3){
        cout << "not enough arguments provided. Check the documentation at the top of the file."<<endl;
        abort();
    }

    int numberOfGeneratedFiles = stoi(argv[1]) ? stoi(argv[1]) : -1;
    string pathToFuzzers = argv[2] ? argv[2] : "";
    string onlyMode = argc == 4 && argv[3] ? argv[3] : "";

    if (numberOfGeneratedFiles < 0 || numberOfGeneratedFiles >= 1000000 || pathToFuzzers.length() == 0 )
    {
        cout << "at least one argument is missing or invalid" << endl;
        abort();
    }

    cout << "============================== starting validation script ==============================" << endl; 
    printf("\nnumberOfGeneratedFiles: %d\npathToFuzzers: %s\nonlyMode: %s\n\n", numberOfGeneratedFiles, pathToFuzzers.c_str(), onlyMode.length() > 0 ? onlyMode.c_str() : "not set");
    cout << "========================================================================================" << endl;

    // file generation 
    int valid_ogg = 0;
    int valid_flac = 0;

    string ogg_res ="";
    string flac_res = "";

    // commands
    string ogg_create_cmd = pathToFuzzers + "ogg-fuzzer fuzz out.ogg";
    string ogg_test_cmd = oggInfoPath + "out.ogg";
    string flac_create_cmd = pathToFuzzers + "flac-fuzzer fuzz out.flac";
    string flac_test_cmd = "metaflac --list out.flac";

    
    for(int i = 0; i < numberOfGeneratedFiles;i++){
       if(onlyMode == "ogg" || onlyMode.length() == 0) {
           // ----------- OGG --------- //
           // > create ogg file
           exec(ogg_create_cmd.c_str());

           // > evaluate ogg file
           ogg_res = exec(ogg_test_cmd.c_str());

           // > check if it was correct
           if (ogg_res.find("Vorbis stream 1") != string::npos && ogg_res.find("ERROR: No Ogg data found in the file") == string::npos) {
               valid_ogg++;
           }
       }

       if (onlyMode == "flac" || onlyMode.length() == 0) {
           // --------- flac --------- //
           // > create flac file
           exec(flac_create_cmd.c_str());

           // > evaluate the file
           flac_res = exec(flac_test_cmd.c_str());

           // > check if it was correct
           if (flac_res.find("There was an error while reading the FLAC file.") != string::npos) {
               valid_flac++;
           }
       }
    }

    // calc and  print evaluation
    float percentage_ogg =  ((float)valid_ogg / (float)numberOfGeneratedFiles);
    float percentage_flac = (float)valid_flac / (float)numberOfGeneratedFiles;
    
    // display as int
    percentage_ogg *= percentage_ogg < 1 ?   100 : 1;
    percentage_flac *= percentage_flac < 1 ? 100 : 1;

    cout << "#=========================================== results ===========================================#" << endl; 
    if(onlyMode == "ogg" || onlyMode.length() == 0) {
       printf("\t\tOGG:  %d / %d (%3.0f%% ) files are valid\n", valid_ogg, numberOfGeneratedFiles, percentage_ogg);
    } if (onlyMode == "flac" || onlyMode.length() == 0) {
       printf("\t\tfLaC: %d / %d (%3.0f%% ) files are valid\n", valid_flac, numberOfGeneratedFiles, percentage_flac);
    }
    cout << "#===============================================================================================#" << endl;
}

