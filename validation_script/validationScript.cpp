#include <stdio.h>
#include <stdlib.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

///////////////////////// constants & shared vars /////////////////////////
const string oggInfoPath =
    "~/Desktop/Thesis/validationTools/vorbis-tools/ogginfo/ogginfo ";
const int PRINT_LENGTH = 120;
const int SAVE_INTERVAL = 1000;
string save_file_name = "results";
int last_RC;
///////////////////////////////////////////////////////////////////////////

/* DOCUMENTATION
 * There are two variables that have to be provided:
 * numberOfGeneratedFiles    {int}       - The number of files that should be generated for each format.
 * pathToFuzzers             {string}    - The path to the parent directory of both fuzzers.
 * saveFileName              {string}    - The name of the file where the results are saved in.
 * onlyMode                  {string}    - Whether only one format should be tested and if so, which.
 *
 * Notes:
 * - numberOfGeneratedFiles  - Range: 1 - 10^6.
 * - pathToFuzzers           - The fuzzers should be called: "ogg-fuzzer" and
 * "flac-fuzzer".
 * - onlyMode                - The file format in lowercase or nothing.
 */

string exec(string cmd) {
    char buffer[128];
    string result = "";
    char *msg;

    // string command = (cmd + " 2>&1");
    string command = "{ " + cmd + "; } 2>&1";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }

    } catch (...) {
        last_RC = pclose(pipe);
        throw;
    }
    last_RC = pclose(pipe);
    return result;
}

void printMessageBar(string msg) {
    // handle padding
    int padding = PRINT_LENGTH - (msg.length() + 2);
    padding = padding > 0 ? padding : 0;

    if (padding % 2 != 0) {
        padding--;
        msg += " ";
    }

    // top / bottom bar
    string bar = "#";
    bar.append(PRINT_LENGTH - 2, '=');
    bar += "#";

    // format message
    string pad = "";
    pad.append(padding > 0 ? (padding / 2) : 0, ' ');
    string content = "#" + pad + msg + pad + "#";

    // print
    cout << bar << endl;
    cout << content << endl;
    cout << bar << endl;
}

void printMessage(string msg, char paddingChar) {
    // handle padding
    int length = PRINT_LENGTH - msg.length();
    int padding = length > 0 ? (length - 2) / 2 : 0;

    string pad = "";
    pad.append(padding, paddingChar);
    if (msg.length() > 0) {
        msg = (" " + msg + " ");
    } else {
        msg = "";
        msg.append(2, paddingChar);
    }
    cout << pad << msg << pad << (length % 2 == 0 ? ' ' : paddingChar) << endl;
}

void writeResult(int num, int ogg, int flac) {
    printMessage("saving to file " + save_file_name, '.');
    std::ofstream file;
    file.open(save_file_name, std::ios::out | std::ios::app);

    // save current result
    file << num << ";" << ogg << ";" << flac << ";\n";
}

int main(int argc, char **argv) {
    // argument validation
    if (argc < 4) {
        cout << "not enough arguments provided. Check the documentation at the "
                "top of the file."
             << endl;
        abort();
    }

    int numberOfGeneratedFiles = stoi(argv[1]) ? stoi(argv[1]) : -1;
    string pathToFuzzers = argv[2] ? argv[2] : "";
    save_file_name = argc >= 4 && argv[3] ? argv[3] : save_file_name;
    string onlyMode = argc >= 5 && argv[4] ? argv[4] : "";

    if (numberOfGeneratedFiles < 0 || numberOfGeneratedFiles >= 1000000 ||
        pathToFuzzers.length() == 0) {
        cout << "at least one argument is missing or invalid" << endl;
        abort();
    }

    // print welcome message
    printMessageBar("starting validation script");
    cout << "- number of generated files: " << numberOfGeneratedFiles << endl;
    cout << "- path to fuzzers: " << pathToFuzzers << endl;
    cout << "- save file name: " << save_file_name << endl;
    cout << "- only mode: "
         << (onlyMode.length() > 0 ? onlyMode.c_str() : "not set") << endl;
    printMessage("", '=');
    cout << "\n";

    // file generation
    int valid_ogg = 0;
    int valid_flac = 0;

    string ogg_res = "";
    string flac_res = "";

    // commands
    string ogg_create_cmd = pathToFuzzers + "ogg-fuzzer fuzz out.ogg";
    string ogg_test_cmd = oggInfoPath + "out.ogg";
    string flac_create_cmd = pathToFuzzers + "flac-fuzzer fuzz out.flac";
    string flac_test_cmd = "metaflac --list out.flac";

    for (int i = 0; i < numberOfGeneratedFiles; i++) {
        // save the results
        if(i % SAVE_INTERVAL == 0 && i != 0){
            writeResult(i, valid_ogg, valid_flac);
        }

        if (onlyMode == "ogg" || onlyMode.length() == 0) {
            // ----------- OGG --------- //
            // > create ogg file
            exec(ogg_create_cmd.c_str());

            // > evaluate ogg file
            ogg_res = exec(ogg_test_cmd.c_str());

            // > check if it was correct
            if ((ogg_res.find("Vorbis stream 1") != string::npos &&
                 ogg_res.find("ERROR: No Ogg data found in the file") ==
                     string::npos)) {
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
            if (flac_res.find(
                    "There was an error while reading the FLAC file.") ==
                    string::npos &&
                last_RC == 0) {
                valid_flac++;
            }
        }
    }

     writeResult(numberOfGeneratedFiles, valid_ogg, valid_flac);

    // calc and  print evaluation and display it as a int
    float percentage_ogg =
        ((float)valid_ogg / (float)numberOfGeneratedFiles) * 100;
    float percentage_flac =
        ((float)valid_flac / (float)numberOfGeneratedFiles) * 100;

    cout << endl;
    printMessageBar("results");

    if (onlyMode == "ogg" || onlyMode.length() == 0) {
        printf("\t\tOGG:  %d / %d (%3.0f%% ) files are valid\n", valid_ogg,
               numberOfGeneratedFiles, percentage_ogg);
    }
    if (onlyMode == "flac" || onlyMode.length() == 0) {
        printf("\t\tfLaC: %d / %d (%3.0f%% ) files are valid\n", valid_flac,
               numberOfGeneratedFiles, percentage_flac);
    }

    printMessage("", '=');
}
