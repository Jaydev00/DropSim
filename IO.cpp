#include "IO.h"
#include "versions.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <sstream>
#include <iomanip>

void IOUtils::printHelpMsg(char *exeName) {
    std::cout << "Usage " << exeName << "Version " << DropSimulation_VERSION_MAJOR << "." << DropSimulation_VERSION_MINOR << std::endl;
    std::cout << "-h \t\t\t\t: show help" << std::endl;
    std::cout << "-a <filename> \t\t\t: csv of target items gained, should be in same order as weight and include any tertiary drops. " << std::endl;
    std::cout << "-b <factor> \t\t\t: factor by which the weighting of items is multiplied in the weights csv" << std::endl;
    std::cout << "-c <num items>\t\t\t: only simulate until a given number of items are obtained instead of all." << std::endl;
    std::cout << "-f <fileName>\t\t\t: name of file to output simulation results." << std::endl;
    std::cout << "-g <fileName>\t\t\t: File with already obtained items include all items | in the same order as weighting if applicable" << std::endl;
    std::cout << "-l <[1-3]>\t\t\t: set the end condition of the iteration to (1) 1 to 1 weight (2) total weight (3) given attempts. " << std::endl;
    std::cout << "-p <number of rolls>\t\t: number of rolls per attempt" << std::endl;
    std::cout << "-r <rarity N>/<rarity D>\t: (Required) Rarity of items n/d" << std::endl;
    std::cout << "-s <simulations>\t\t: (Required) number of simulations to run." << std::endl;
    std::cout << "-t <threads>\t\t\t: (Required) number of threads to run the simulation on." << std::endl;
    std::cout << "-u <uniques>\t\t\t: (Required) number of unique items to collect." << std::endl;
    std::cout << "-v \t\t\t\t: Verbose output" << std::endl;
    std::cout << "-w <fileName>\t\t\t: name of the file with unique weighting for uniques with different rates" << std::endl;
    std::cout << "-3 <rarity N>/<rarity D>\t: add teritary roll. eg -3 1/100" << std::endl;
}


void IOUtils::printHelpMsg(char* exeName, const std::string &extraMsg) {
    std::cout << extraMsg << std::endl;
    printHelpMsg(exeName);
}

bool IOUtils::parseArgs(const int& argc, char* argv[], SimArgs &argsStruct) {
    int opt;
    std::string temp;
    std::ifstream infile;
    while ((opt = getopt(argc, argv, "hvs:t:u:r:f:w:c:g:3:p:a:b:l:")) != -1) {
        switch (opt) {
            case 'h':
                printHelpMsg(argv[0]);
                return false;

            case 'a':  // target Items
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing target items end point input");
                    return false;
                }
                infile.open(optarg);
                for (std::string temp; std::getline(infile, temp);) {
                    argsStruct.targetItems.push_back(stoi(temp));
                }
                argsStruct.useTargetItems = true;
                infile.close();
                break;

            case 'b':  // weight factor
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Weight Factor input");
                    return false;
                }
                argsStruct.weightFactor = std::stoi(optarg);
                break;

            case 'c':  // conditional end point
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing end condition input");
                    return false;
                }
                if (optarg)
                    argsStruct.count = std::stoi(optarg);
                break;

            case 'f':  // output file
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing file output input");
                    return false;
                }
                argsStruct.resultsFileName = optarg;
                break;

            case 'g':  // items already gained/starting point csv
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing gained items starting point input");
                    return false;
                }
                infile.open(optarg);
                for (std::string temp; std::getline(infile, temp);) {
                    argsStruct.obtainedItems.push_back(stoi(temp));
                }
                infile.close();
                break;
            case 'l':  // use end condition 1 to 1 weighting
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing end condition input");
                    return false;
                }
                if (argsStruct.endCondition != EndCondition::Uniques) {
                    printHelpMsg(argv[0], "Multiple conflicting end condtitions given");
                    return false;
                } else {
                    switch (atoi(optarg)) {
                        case static_cast<int>(EndCondition::Weight1to1):
                            argsStruct.endCondition = EndCondition::Weight1to1;
                            break;
                        case static_cast<int>(EndCondition::WeightTotal):
                            argsStruct.endCondition = EndCondition::WeightTotal;
                            break;
                        case static_cast<int>(EndCondition::Attempts):
                            argsStruct.endCondition = EndCondition::Attempts;
                            break;
                        default:
                            printHelpMsg(argv[0], "Unkown end condtitions given");
                            return false;
                    }
                }
                break;
            case 'p':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Rolls per attempt input");
                    return false;
                }
                argsStruct.numRollsPerAttempt = std::stoi(optarg);
                break;

            case 'r':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing basic rarity input");
                    return false;
                }
                temp = optarg;
                argsStruct.rarityN = std::stoi(temp.substr(0, temp.find("/")));
                argsStruct.rarityD = std::stoi(temp.substr(temp.find("/") + 1, temp.length()));
                break;
            case 's':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Simulations input");
                    return false;
                }
                argsStruct.iterations = std::stoi(optarg);
                break;
            case 't':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Thread input");
                    return false;
                }
                argsStruct.threads = std::stoi(optarg);
                break;

            case 'u':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Uniques input");
                    return false;
                }
                argsStruct.uniques = std::stoi(optarg);
                break;
            case 'v':
                argsStruct.verboseLogging = true;
                break;

            case 'w':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Weight File input");
                    return false;
                }
                infile.open(optarg);
                std::cout << "optarg:" << optarg << std::endl;
                if(infile.fail()){
                    printHelpMsg(argv[0], "File does not exist");
                    return false;
                }
                for (std::string temp; std::getline(infile, temp);) {
                    argsStruct.weightings.push_back(std::pair<int, int>(
                        stoi(temp.substr(0, temp.find(","))),
                        stoi(temp.substr(temp.find(",") + 1, temp.length()))));
                }
                infile.close();
                break;

            case '3':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing tertiary input");
                    return false;
                }
                temp = optarg;
                argsStruct.tertiaryRolls.push_back(std::pair<int, int>(
                    stoi(temp.substr(0, temp.find("/"))),
                    stoi(temp.substr(temp.find("/") + 1, temp.length()))));
                break;
        }
    }
    if (argsStruct.iterations == 0 ||
        argsStruct.threads == 0 ||
        argsStruct.uniques == 0 ||
        argsStruct.rarityN == 0 ||
        argsStruct.rarityD == 0) {
        std::cout << "Missing required input" << std::endl;
        std::cout << "have"
                  << " sims " << argsStruct.iterations
                  << " threads " << argsStruct.threads
                  << " uniques " << argsStruct.uniques
                  << " rairtyN " << argsStruct.rarityN
                  << " rarityD " << argsStruct.rarityD
                  << std::endl;
        printHelpMsg(argv[0]);
        return false;
    }
    if(argsStruct.numRollsPerAttempt < 1){
        std::cout << "Detected Number of 'Rolls Per Attempt' as less than 1, setting to 1" << std::endl;
        argsStruct.numRollsPerAttempt = 1;
    }
    return true;
}

void IOUtils::printStartParameters(const SimArgs& args){
    std::cout << "Running " << FmtCmma(args.iterations) << " Simulations with " << args.uniques << " slots with " << FmtCmma(args.rarityN) << "/" << FmtCmma(args.rarityD) << " rarity on " << args.threads << " threads." << std::endl;
    if (args.count > 0)
        std::cout << "Stopping after " << args.count << " Items Obtained." << std::endl;
    if (args.numRollsPerAttempt > 1)
        std::cout << "Rolling for Loot " << args.numRollsPerAttempt << " Times per Attempt." << std::endl;
    if (args.weightings.size() > 0)
        std::cout << "Using Weight File" << std::endl;
    if (args.obtainedItems.size() > 0)
        std::cout << "Using Obtained Items File" << std::endl;
    if (args.tertiaryRolls.size() > 0)
        std::cout << "Rolling " << args.tertiaryRolls.size() << " Tertiary Roll(s) per attempt" << std::endl;
    std::cout << std::endl;
}



