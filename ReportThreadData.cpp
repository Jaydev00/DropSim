#include "ReportThreadData.h"

ReporterThreadData::ReporterThreadData(const std::shared_ptr<std::atomic_ullong>& nglobalprogressCounter,
        const unsigned long long& niterations, 
        const std::shared_ptr<std::chrono::high_resolution_clock::time_point>& nstartTimePoint){
    this->globalprogressCounter = nglobalprogressCounter;
    this->iterations = niterations;
    this->startTimePoint = nstartTimePoint;
}