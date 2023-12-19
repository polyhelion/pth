
#include "pth.hxx"
#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstddef>
#include <chrono>
#include <ctime>



pth::mutex iomutex;


void* func(void*) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

    iomutex.lock();
    std::cout << "thread id : " << ::pthread_self() << "  |  timestamp (ns since epoch) : " << \
               nanoseconds.count() << std::endl;
    iomutex.unlock();
    return nullptr;
};


int main() {

    const int num_threads = 50;

    std::vector<pth::thread> threads;

    ::pthread_mutexattr_t m_attr{};
	::pthread_mutexattr_setprotocol( &m_attr, PTHREAD_PRIO_PROTECT );   
    pth::mutex iomutex( m_attr );

    ::pthread_attr_t th_attr{};
    ::pthread_attr_init(&th_attr);
    ::pthread_attr_setdetachstate(&th_attr,PTHREAD_CREATE_JOINABLE);

    for(auto i{0}; i < num_threads; i++) {
        threads.push_back(pth::thread(th_attr,func));
    } 
    
    for (auto& thread : threads) {
        thread.join();
    }

}
