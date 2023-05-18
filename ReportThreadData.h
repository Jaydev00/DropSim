#ifndef REPORT_THREAD_DATA_H
#define REPORT_THREAD_DATA_H
#include <memory>
#include <atomic>
#include <chrono>

class ReporterThreadData {
   public:
    ReporterThreadData(const std::shared_ptr<std::atomic_ullong>&,
        const unsigned long long&, 
        const std::chrono::high_resolution_clock::time_point&);
    
    std::shared_ptr<std::atomic_ullong> globalprogressCounter;
    unsigned long long iterations;
    std::chrono::high_resolution_clock::time_point startTimePoint;
};

#endif