// Coordinator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Coordinator.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerConnectionPool.h"

// Core
#include "Core/Profile/Profile.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
Coordinator::Coordinator( const AString & args )
    : m_BaseArgs( args )
    , m_ConnectionPool( nullptr )
{
    m_ConnectionPool = FNEW( WorkerConnectionPool );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Coordinator::~Coordinator()
{
    FDELETE m_ConnectionPool;
}

// Start
//------------------------------------------------------------------------------
uint32_t Coordinator::Start()
{
    // spawn work thread
    m_WorkThread.Start(&WorkThreadWrapper, "CoordinatorThread", this);

    // Join work thread and get exit code
    return m_WorkThread.Join();
}

// WorkThreadWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t Coordinator::WorkThreadWrapper( void * userData )
{
    Coordinator * coordinator = reinterpret_cast<Coordinator *>( userData );
    return coordinator->WorkThread();
}

// Start
//------------------------------------------------------------------------------
uint32_t Coordinator::WorkThread()
{
    OUTPUT( "FBuildCoordinator - " FBUILD_VERSION_STRING "\n" );

    // start listening
    OUTPUT( "Listening on port %u\n", Protocol::COORDINATOR_PORT );
    if ( m_ConnectionPool->Listen( Protocol::COORDINATOR_PORT ) == false )
    {
        OUTPUT( "Failed to listen on port %u.  Check port is not in use.\n", Protocol::COORDINATOR_PORT );
        return (uint32_t)-3;
    }

    for(;;)
    {
        PROFILE_SYNCHRONIZE

        Thread::Sleep( 500 );
    }

    // return 0;
}

//------------------------------------------------------------------------------
