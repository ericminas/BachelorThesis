/* Documentation
 * This tests the provided subjects with files generated by Format Fuzzer.
 * Required Arguments:
 * test_runs     {int}       - The number of files that should be generated
 *                             for each subject.
 * pathToFuzzers {string}    - The path to the parent directory of the fuzzers.
 *
 * Optional Args
 * -c            {void}      - Whether the files in the findings folder should
 *                             be deleted before the program starts.
 * -h            {void}      - Prints a help message.
 *
 * Notes:
 * - numberOfGeneratedFiles  - Range: [1 , 10^6].
 * - pathToFuzzers           - The fuzzers should be called:
 *                             "ogg-fuzzer", "evil-ogg-fuzzer",
 *                             "flac-fuzzer" and "evil-flac-fuzzer".
 */

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

//////////////////// constants & shared vars ////////////////////
const string script_dir = "/subject_scripts/";
const string findings_dir = "/findings";
const string result_file = "results.txt";

const string valid_ogg =
    "~/Desktop/Thesis/validationTools/vorbis-tools/ogginfo/ogginfo "
    "./generated/out.ogg";
const string valid_flac = "metaflac --list ./generated/out.flac";

string subjects[5] = {"dr_libs", "miniaudio", /* "mp3splt", */ "xiph_flac"/* , "libogg" */};
string fileType[5] = {"flac", "flac", /* "ogg", */ "flac"/* , "ogg" */};

int TEST_RUNS = 0;
int SAVE_INTERVAL = 1000;
int PRINT_LENGTH = 160;
string pathToFuzzers = "";

string ogg_command = "";
string evil_ogg_command = "";
string flac_command = "";
string evil_flac_command = "";

int last_RC = 0;
/////////////////////////////////////////////////////////////////

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

string RpadString(string msg, int LENGTH) {
    int length = LENGTH - msg.length();
    int padding = length > 0 ? (length - 1) : 0;

    string pad = "";
    pad.append(padding, ' ');

    return (" " + msg + pad);
}

void writeResult(vector<int> result, string subject) {
    std::ofstream file;
    file.open(result_file, std::ios::out | std::ios::app);

    printMessage("Saving results", '#');

    // create string representation of result
    string msg = "";

    for (int num : result) {
        msg += to_string(num) + ";";
    }

    file << subject << ";" << msg << endl;
}

vector<int> testSubject(string subjectName, string fileType) {
    if(subjectName == "" || fileType == "") {
         vector<int> results = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
         return results;
    }
    
    printMessageBar("Testing: " + subjectName);
    string test_command =
        "." + script_dir + subjectName + "_TS " + "./generated/out." + fileType;
    string move_command =
        "mv ./generated/out." + fileType + " ./findings/" + subjectName + "/";
    string output = "";
    bool evil = false, valid = false;
    // eval vars
    int crashes = 0;

    // 0: valid
    // 1: evil
    // 2: valid + valid
    // 3: valid + invalid
    // 4: evil  + valid
    // 5: evil  + invalid
    // 6: valid + valid   + #crashes
    // 7: valid + invalid + #crashes
    // 8: evil  + valid   + #crashes
    // 9: evil  + invalid + #crashes
    vector<int> results = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for (int i = 0; i < TEST_RUNS; i++) {
        // print current test
        if (TEST_RUNS > 1) {
            printMessage(subjectName + " test #" + to_string(i), '.');
        }

        ///////////////////// auto save /////////////////////////////////
        if (i % SAVE_INTERVAL == 0 && i != 0) {
            writeResult(results, subjectName);
        }

        ///////////////////// handle file generation ////////////////////
        if (fileType == "ogg") {
            // select valid vs evil fuzzer
            evil = i % 2 == 0;

            exec(evil ? evil_ogg_command : ogg_command);
            results.at(evil ? 1 : 0)++;

            // check validity
            output = exec(valid_ogg);
            valid = (output.find("Vorbis stream 1") != string::npos &&
                     output.find("ERROR: No Ogg data found in the file") ==
                         string::npos);

            results.at(valid ? (evil ? 4 : 2) : (evil ? 5 : 3))++;

        } else if (fileType == "flac") {
            // select valid vs evil fuzzer
            evil = i % 2 == 0;

            exec(evil ? evil_flac_command : flac_command);
            results.at(evil ? 1 : 0)++;

            // check validity
            output = exec(valid_flac);
            valid = (output.find(
                         "There was an error while reading the FLAC file.") ==
                     string::npos);

            results.at(valid ? (evil ? 4 : 2) : (evil ? 5 : 3))++;
        }

        //////////////////// check the subjet ////////////////////
        output = exec(test_command);

        // check the console output / whether a error was thrown
        if (output.find("aborted") != string::npos ||
            output.find("Segmentation fault (core dumped)") != string::npos ||
            last_RC != 0) {
            // move most recent file to findigs
            string cmd =
                move_command + "crash_" + to_string(crashes) + "." + fileType;
 
            if (crashes <= 1000) {
                exec(cmd);
            }
 
            // notify
            printMessage("found crash " + to_string(crashes) +
                            ". Moved the file to findigs/" + subjectName,
                         '/');

            results.at(valid ? (evil ? 8 : 6) : (evil ? 9 : 7))++;
            crashes++;
        }
    }

    // save and return the number of crashes
    writeResult(results, subjectName);
    return results;
}

void printResults(vector<vector<int>> results) {
    printMessageBar("Results");
    cout << endl;
    printMessage("Legend:", '-');
    cout << "N  = normal file (should be valid)\n"
         << "M  = mutated file (should be invalid)\n"
         << "V  = valid file\n"
         << "IV = invalid file\n"
         << "C: = crahes of file with the following properites\n";
    printMessage("table header for the results", '-');
    int padding = 14;
    string separator = "|";
    cout << RpadString("subject", padding) << separator
         << RpadString("N", padding) << separator << RpadString("M", padding)
         << separator << RpadString("N & V", padding) << separator
         << RpadString("N & IV", padding) << separator
         << RpadString("M & V", padding) << separator
         << RpadString("M & IV", padding) << separator
         << RpadString("C: N & V", padding) << separator
         << RpadString("C: N & IV", padding) << separator
         << RpadString("C: M & V", padding) << separator
         << RpadString("C: M & IV", padding) << endl;
    printMessage("", '-');

    // print the table
    int i = 0;
    for (vector<int> res : results) {
        cout << RpadString(subjects[i], padding) << separator
             << RpadString(to_string(res[0]), padding) << separator
             << RpadString(to_string(res[1]), padding) << separator
             << RpadString(to_string(res[2]), padding) << separator
             << RpadString(to_string(res[3]), padding) << separator
             << RpadString(to_string(res[4]), padding) << separator
             << RpadString(to_string(res[5]), padding) << separator
             << RpadString(to_string(res[6]), padding) << separator
             << RpadString(to_string(res[7]), padding) << separator
             << RpadString(to_string(res[8]), padding) << separator
             << RpadString(to_string(res[9]), padding) << endl;
        i++;
    }
    printMessage("", '-');
}

void validateDirs() {
    printMessage("checking dirs", '-');
    string required_dirs[7] = {"generated",          "findings",
                               "findings/dr_libs",   "findings/libogg",
                               "findings/miniaudio", "findings/mp3splt",
                               "findings/xiph_flac"};
    string required_files[4] = {
        pathToFuzzers + "ogg-fuzzer", pathToFuzzers + "evil-ogg-fuzzer",
        pathToFuzzers + "flac-fuzzer", pathToFuzzers + "evil-flac-fuzzer"};

    string cmd = "";
    string out = "";
    bool shouldStop = false;

    int size = (sizeof(required_dirs) / sizeof(required_dirs[0]));
    for (int i = 0; i < size; i++) {
        // check existance
        cmd = "[ -d \"./" + required_dirs[i] + "\" ] && echo \"exists\"";
        out = exec(cmd);

        if (out.find("exists") == string::npos) {
            cout << required_dirs[i] << " does not exist" << endl;
            shouldStop = true;
        } else {
            cout << "found: " << required_dirs[i] << endl;
        }
    }

    size = (sizeof(required_files) / sizeof(required_files[0]));
    for (int i = 0; i < size; i++) {
        cmd = "test -f " + required_files[i] + " && echo \"exists\"";
        out = exec(cmd);

        if (out.find("exists") == string::npos) {
            cout << required_files[i] << " does not exist" << endl;
            shouldStop = true;
        } else {
            cout << "found: " << required_files[i] << endl;
        }
    }

    if (shouldStop) {
        abort();
    }
}

void cleanUpResultFile() {
    string command = "rm " + result_file;
    exec(command);
}

void cleanUpFiles() {
    printMessageBar("clean up files in findings");

    // lopp through the dirs
    string path = "./findings/";
    string command = "";
    for (int i = 0; i < (sizeof(subjects) / sizeof(subjects[0])); i++) {
        cout << "remove files from: " << path << subjects[i] << endl;
        command = "rm " + path + subjects[i] + "/*";

        exec(command);
    }

    cleanUpResultFile();
}

void printHelp() {
    printMessageBar("Help for the script:");
    cout << "Needed arguments: \n"
         << "\t\t- {int}    number of testruns for each subject.\n"
         << "\t\t- {string} the path to the directory that contains the "
            "fuzzers.\n"
         << "\t\t- {int}     number of file generations until the current "
            "results are saved.\n";
    cout << "\nAdditional Arguments:\n"
         << "\t\t- '-c' {void}  removes the files from the /findings folder\n";
    cout << "\nNotes:\n"
         << "\t\t- number of testruns must be in interval: [1, 10^6]\n"
         << "\t\t- the fuzzers should have the names: ogg-fuzzer, "
            "evil-ogg-fuzzer, flac-fuzzer and evil-flac-fuzzer.\n";
}

int main(int argc, char **argv) {
    // get args
    if (argc < 3) {
        // check help flag
        if (argc == 2) {
            string flag = argv[1];
            if (flag == "-h") {
                printHelp();
                return 0;
            }
        }
        cout << "not enough arguments provided. Check the documentation at the "
                "top of the file. or use -h"
             << endl;
        abort();
    }

    TEST_RUNS = stoi(argv[1]);
    pathToFuzzers = argv[2];
    SAVE_INTERVAL = argc >= 4 ? stoi(argv[3]) : 1000;

    if (TEST_RUNS < 0 || pathToFuzzers.length() == 0 ||
        pathToFuzzers.back() != '/') {
        cout << "at least one argument is missing or invalid" << endl;
        abort();
    }

    // create the file generation commands
    ogg_command = pathToFuzzers + "ogg-fuzzer fuzz ./generated/out.ogg";
    evil_ogg_command =
        pathToFuzzers + "evil-ogg-fuzzer fuzz ./generated/out.ogg";
    flac_command = pathToFuzzers + "flac-fuzzer fuzz ./generated/out.flac";
    evil_flac_command =
        pathToFuzzers + "evil-flac-fuzzer fuzz ./generated/out.flac";

    if (argc >= 5) {
        string flag = argv[4];
        if (flag == "-c") {
            cleanUpFiles();
        }
    }

    // welcome message
    printMessageBar("Arguments and Metadata for the script");
    // print args
    cout << "test runs: " << TEST_RUNS << endl;
    cout << "save intervall: " << SAVE_INTERVAL << endl;
    cout << "path to fuzzers: " << pathToFuzzers << "\n\n";
    cout << "ogg  generation command: " << ogg_command << endl;
    cout << "evil ogg  generation command: " << evil_ogg_command << "\n\n";
    cout << "flac generation command: " << flac_command << endl;
    cout << "evil flac generation command: " << evil_flac_command << "\n\n";
    // check required dirs
    validateDirs();
    cleanUpResultFile();

    // test subjects
    vector<vector<int>> results;
    for (int i = 0; i < (sizeof(subjects) / sizeof(subjects[0])); i++) {
        results.push_back(testSubject(subjects[i], fileType[i]));
    }

    // evaluate
    printResults(results);

    return 0;
}
