#include "SimArgs.h"

SimArgs::SimArgs(){
    this->endCondition = EndCondition::Uniques;
    this->iterations = 0;
    this->threads = 0;
    this->rarityN = 0;
    this->rarityD = 0;
    this->uniques = 0;
    this->count = 0;
    this->numRollsPerAttempt = 0;
    this->weightFactor = 0;
    this->verboseLogging = false;
    this->useTargetItems = false;
}