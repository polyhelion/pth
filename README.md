# pth - A lightweight Pthread RAII wrapper.

For most purposes the standard multithreading model from C++11 onwards 
is the right way to go.
This wrapper is focused on fine grained control over thread properties 
and thread lifecycles via the POSIX thread (pthread) API.  
 
## Usage:

```c++

#include "pth.hxx"

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

    (...)

} 

```
