//
//
//  A lightweight Pthread RAII wrapper.
//  For most purposes the standard multithreading model from C++11 onwards 
//  is the right way to go.
//  This wrapper is focused on fine grained control over thread properties 
//  and thread lifecycles via the POSIX thread (pthread) API.  
//  It has little to no safeguards against argument misuse and failed pthread 
//  calls. 
// 
//  2023 Jens Christian Keil
//
//


#ifndef PTH_HXX
#define PTH_HXX


#include <cerrno>
#include <ctime>
#include <cassert>
#include <cstddef>
#include <utility>

#include <pthread.h>


// Some instrumentation
#if defined NDEBUG
# define ASSERT_EQ0(X) void(0)
#else
# define ASSERT_EQ0(X) \
    ( (X == 0) ? void(0) : []{assert(#X " != 0");}() )
#endif


namespace pth {


class thread {
public:
    thread( const ::pthread_attr_t& attrhandle, void* (*start_routine)(void*), void* arg = nullptr) noexcept
        {            
            int detachstate;
            int retval = ::pthread_attr_getdetachstate (&attrhandle, &detachstate);
            ASSERT_EQ0( retval );
            if (detachstate == PTHREAD_CREATE_JOINABLE) { _joinable = true; }
            else { _joinable = false; }

            ASSERT_EQ0( ::pthread_create( &_handle, &attrhandle, start_routine, arg ) ); 
        
        }

    // The POSIX standard requires threads to be joinable by default. The default setting of the detach 
    // state attribute in a newly initialized thread attributes object is PTHREAD_CREATE_JOINABLE. 

    thread( void* (*start_routine)(void*), void* arg = nullptr ) noexcept
        { 
            _joinable = true;
            ASSERT_EQ0( ::pthread_create( &_handle, nullptr, start_routine, arg ) ); 
        }

    thread() noexcept : _handle(0L), _joinable(false) { }
        // Adapt default constructor behaviour to own use case 

    ~thread() { 
        if ( joinable() ) { join(); }
        // For some use cases 'detach' or 'terminate' is a better fit here
    }
    
    thread( const thread& ) = delete;
    thread& operator=( const thread& ) = delete;

    thread(thread&& other) noexcept {
        _handle = std::exchange(other._handle, 0L);
        _joinable = std::exchange(other._joinable, false);
    }

    thread& operator=(thread&& other) noexcept {
        _handle = std::exchange(other._handle, 0L);
        _joinable = std::exchange(other._joinable, false);
        return *this;
    }


    bool joinable() const noexcept { return _joinable; }

    ::pthread_t native_handle() noexcept { return _handle; }

    
    void join(void** retval = nullptr) {
        if ( joinable() ) {
            ASSERT_EQ0( ::pthread_join( _handle, retval) );
            _joinable = false; 
        }
    }
    
    void detach() {
         ASSERT_EQ0( ::pthread_detach( _handle ) );
    }

private:
  
    ::pthread_t _handle;
    bool _joinable;
};


class rwlock {
public:
    rwlock();

    rwlock( const ::pthread_rwlockattr_t& attr_handle )
       {  ASSERT_EQ0( ::pthread_rwlock_init( &_handle, &attr_handle ) ); }
    
    ~rwlock() {  ASSERT_EQ0( ::pthread_rwlock_destroy( &_handle ) ); }

    rwlock( const rwlock& other ) = delete;
    rwlock& operator=( const rwlock& other ) = delete;

    void rdlock() {  ASSERT_EQ0( ::pthread_rwlock_rdlock( &_handle ) ); }
    bool tryrdlock();
    void wrlock() {  ASSERT_EQ0( ::pthread_rwlock_wrlock( &_handle ) ); }
    bool trywrlock();
    void unlock() {  ASSERT_EQ0( ::pthread_rwlock_unlock( &_handle ) ); }

    ::pthread_rwlock_t  native_handle() { return _handle; }

private:
 
    ::pthread_rwlock_t _handle;
};

inline rwlock::rwlock() {
    ::pthread_rwlockattr_t attr;

    ASSERT_EQ0( ::pthread_rwlockattr_init( &attr ) );
    ASSERT_EQ0( ::pthread_rwlockattr_setkind_np( &attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP ) );
    ASSERT_EQ0( ::pthread_rwlock_init( &_handle, &attr ) );
    ASSERT_EQ0( ::pthread_rwlockattr_destroy( &attr ) );
    
}

inline bool rwlock::tryrdlock() {
    int retval = ::pthread_rwlock_tryrdlock( &_handle );
    if ( retval == EBUSY ) return false;
    ASSERT_EQ0( retval );
    return true;
}

inline bool rwlock::trywrlock() {
    int retval = ::pthread_rwlock_trywrlock( &_handle );
    if ( retval == EBUSY ) return false;
    ASSERT_EQ0( retval );
    return true;
}



class mutex {
public:
    mutex() {  ASSERT_EQ0( ::pthread_mutex_init( &_handle, nullptr ) ); }
    
    mutex( const ::pthread_mutexattr_t& attrhandle )
       {  ASSERT_EQ0( ::pthread_mutex_init( &_handle, &attrhandle ) ); }
    
    ~mutex() {  ASSERT_EQ0( ::pthread_mutex_destroy( &_handle ) ); }

    mutex( const mutex& other ) = delete;
    mutex& operator=( const mutex& other ) = delete;

    void lock() {  ASSERT_EQ0( ::pthread_mutex_lock( &_handle ) ); }
    void unlock() {  ASSERT_EQ0( ::pthread_mutex_unlock( &_handle ) ); }
    bool trylock();

    ::pthread_mutex_t native_handle() { return _handle; }

private:
 
    ::pthread_mutex_t _handle;
};

inline bool mutex::trylock() {
    int retval = ::pthread_mutex_trylock( &_handle );
    if ( retval == EBUSY ) return false;
    ASSERT_EQ0( retval );
    return true;
}


class spinlock {
public:
    explicit spinlock( int pshared ) {  ASSERT_EQ0( ::pthread_spin_init( &_handle, pshared ) ); }
    
    ~spinlock() {  ASSERT_EQ0( ::pthread_spin_destroy( &_handle ) ); }
    
    spinlock( const spinlock& other ) = delete;
    spinlock& operator=( const spinlock& other ) = delete;

    void lock() {  ASSERT_EQ0( ::pthread_spin_lock( &_handle ) ); }
    void unlock() {  ASSERT_EQ0( ::pthread_spin_unlock( &_handle ) ); }
    bool trylock();

    ::pthread_spinlock_t native_handle() { return _handle; }

private: 
    
    ::pthread_spinlock_t _handle;
};

inline bool spinlock::trylock() {
    int retval = ::pthread_spin_trylock( &_handle );
    if ( retval == EBUSY ) return false;
    ASSERT_EQ0( retval );
    return true;
}



class cond_var {
public:
    cond_var () {  ASSERT_EQ0( ::pthread_cond_init( &_handle, nullptr ) ); }
    
    cond_var( const ::pthread_condattr_t& attrhandle )
       {  ASSERT_EQ0( ::pthread_cond_init( &_handle, &attrhandle ) ); }
    
    ~cond_var() {  ASSERT_EQ0( ::pthread_cond_destroy( &_handle ) ); }

    cond_var( const cond_var& other ) = delete;
    cond_var& operator=( const cond_var& other ) = delete;

    void wait( mutex& mtx );
    int  timedwait( mutex& mtx, long nsec );
    void signal() {  ASSERT_EQ0( ::pthread_cond_signal( &_handle ) ); }
    void broadcast() {  ASSERT_EQ0( ::pthread_cond_broadcast( &_handle ) ); }

    ::pthread_cond_t native_handle() { return _handle; }

private:

    ::pthread_cond_t _handle;
};

inline void cond_var::wait( mutex& mtx ) {
    ::pthread_mutex_t nh = mtx.native_handle();
    ASSERT_EQ0( ::pthread_cond_wait( &_handle, &nh ) );
}

inline int cond_var::timedwait( mutex& mtx, long nsec ) {

    constexpr long NS_PER_SEC = 1000000000L;

    std::timespec now, abstime;

    ::clock_gettime( CLOCK_REALTIME, &now );  // hardware and os setup may require a
                                              // different clock like CLOCK_MONOTONIC
    
    abstime.tv_sec  =  now.tv_sec  + nsec / NS_PER_SEC;
    abstime.tv_nsec =  now.tv_nsec + nsec % NS_PER_SEC;
    long over = abstime.tv_nsec / NS_PER_SEC;
    if ( abstime.tv_nsec / NS_PER_SEC ) {
        abstime.tv_nsec -= over * NS_PER_SEC;
        abstime.tv_sec  += over;
    }

    ::pthread_mutex_t nh = mtx.native_handle();
    int retval = ::pthread_cond_timedwait( &_handle, &nh, &abstime );
    if ( retval == ETIMEDOUT ) return ETIMEDOUT;
    ASSERT_EQ0( retval ); 
    return 0;
}

} // namespace pth

#endif // PTH_HXX





