// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerage.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerConnectionPool.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Thread.h"
#include "Core/Tracing/Tracing.h"

#if defined( __APPLE__ )

#include <sys/socket.h>
#include <ifaddrs.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static bool ConvertHostNameToLocalIP4( AString& hostName )
{
    bool result = false;

    struct ifaddrs * allIfAddrs;
    if ( getifaddrs( &allIfAddrs ) == 0 )
    {
        struct ifaddrs * addr = allIfAddrs;
        char ipString[48] = { 0 };
        while ( addr )
        {
            if ( addr->ifa_addr )
            {
                if ( addr->ifa_addr->sa_family == AF_INET && strcmp( addr->ifa_name, "en0" ) == 0 )
                {
                    struct sockaddr_in * sockaddr = ( struct sockaddr_in * ) addr->ifa_addr;
                    inet_ntop( AF_INET, &sockaddr->sin_addr, ipString, sizeof( ipString ) );
                    hostName = ipString;
                    result = true;
                    break;
                }
            }
            addr = addr->ifa_next;
        }

        freeifaddrs( allIfAddrs );
    }

    return result;
}

#endif // __APPLE__

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
    : m_Availability( false )
    , m_Initialized( false )
    , m_ConnectionPool( nullptr )
    , m_Connection( nullptr )
    , m_WorkerListUpdateReady( false )
{
}

// Init
//------------------------------------------------------------------------------
void WorkerBrokerage::Init()
{
    PROFILE_FUNCTION

    if ( m_Initialized )
    {
        return;
    }

    Network::GetHostName(m_HostName);

#if defined( __APPLE__ )
    ConvertHostNameToLocalIP4(m_HostName);
#endif

    if ( m_CoordinatorAddress.IsEmpty() == true )
    {
        AStackString<> coordinator;
        if ( Env::GetEnvVariable( "FASTBUILD_COORDINATOR", coordinator ) )
        {
            m_CoordinatorAddress = coordinator;
        }
    }

    if ( m_CoordinatorAddress.IsEmpty() == true )
    {
        OUTPUT( "Using brokerage folder\n" );

        // brokerage path includes version to reduce unnecssary comms attempts
        uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

        // root folder
        AStackString<> root;
        if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", root ) )
        {
            // <path>/<group>/<version>/
            #if defined( __WINDOWS__ )
                m_BrokerageRoot.Format( "%s\\main\\%u.windows\\", root.Get(), protocolVersion );
            #elif defined( __OSX__ )
                m_BrokerageRoot.Format( "%s/main/%u.osx/", root.Get(), protocolVersion );
            #else
                m_BrokerageRoot.Format( "%s/main/%u.linux/", root.Get(), protocolVersion );
            #endif
        }

        AStackString<> filePath;
        m_BrokerageFilePath.Format( "%s%s", m_BrokerageRoot.Get(), m_HostName.Get() );
    }
    else
    {
        OUTPUT( "Using coordinator\n" );
    }

    m_TimerLastUpdate.Start();

    m_Initialized = true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage()
{
    // Ensure the file disapears when closing
    if ( m_Availability )
    {
        FileIO::FileDelete( m_BrokerageFilePath.Get() );
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers( Array< AString > & workerList )
{
    PROFILE_FUNCTION

    Init();

    if ( m_BrokerageRoot.IsEmpty() && m_CoordinatorAddress.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root and no coordinator available; did you set FASTBUILD_BROKERAGE_PATH or launched with -coordinator param?" );
        return;
    }

    if ( ConnectToCoordinator() )
    {
        m_WorkerListUpdateReady = false;

        OUTPUT( "Requesting worker list\n");

        Protocol::MsgRequestWorkerList msg;
        msg.Send( m_Connection );

        while ( m_WorkerListUpdateReady == false )
        {
            Thread::Sleep( 1 );
        }

        DisconnectFromCoordinator();

        OUTPUT( "Worker list received: %u workers\n", (uint32_t)m_WorkerListUpdate.GetSize() );
        if ( m_WorkerListUpdate.GetSize() == 0 )
        {
            FLOG_WARN( "No workers received from coordinator" );
            return; // no files found
        }

        // presize
        if ( ( workerList.GetSize() + m_WorkerListUpdate.GetSize() ) > workerList.GetCapacity() )
        {
            workerList.SetCapacity( workerList.GetSize() + m_WorkerListUpdate.GetSize() );
        }

        // convert worker strings
        const uint32_t * const end = m_WorkerListUpdate.End();
        for ( uint32_t * it = m_WorkerListUpdate.Begin(); it != end; ++it )
        {
            AStackString<> workerName;
            TCPConnectionPool::GetAddressAsString( *it, workerName );
            if ( workerName.CompareI( m_HostName ) != 0 && workerName.CompareI( "127.0.0.1" ) )
            {
                workerList.Append( workerName );
            }
            else
            {
                OUTPUT( "Skipping woker %s\n", workerName.Get() );
            }
        }

        m_WorkerListUpdate.Clear();
    }
    else if ( !m_BrokerageRoot.IsEmpty() )
    {
        Array< AString > results( 256, true );
        if ( !FileIO::GetFiles( m_BrokerageRoot,
                                AStackString<>( "*" ),
                                false,
                                &results ) )
        {
            FLOG_WARN( "No workers found in '%s'", m_BrokerageRoot.Get() );
            return; // no files found
        }

        // presize
        if ( ( workerList.GetSize() + results.GetSize() ) > workerList.GetCapacity() )
        {
            workerList.SetCapacity( workerList.GetSize() + results.GetSize() );
        }

        // convert worker strings
        const AString * const end = results.End();
        for ( AString * it = results.Begin(); it != end; ++it )
        {
            const AString & fileName = *it;
            const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
            AStackString<> workerName( lastSlash + 1 );
            if ( workerName.CompareI( m_HostName ) != 0 )
            {
                workerList.Append( workerName );
            }
        }
    }
}

// UpdateWorkerList
//------------------------------------------------------------------------------
void WorkerBrokerage::UpdateWorkerList( Array< uint32_t > &workerListUpdate )
{
    m_WorkerListUpdate.Swap( workerListUpdate );
    m_WorkerListUpdateReady = true;
}

// SetAvailability
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailability(bool available)
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoot.IsEmpty() && m_CoordinatorAddress.IsEmpty() )
    {
        return;
    }

    if ( available )
    {
        // Check the last update time to avoid too much File IO.
        float elapsedTime = m_TimerLastUpdate.GetElapsedMS();
        if ( elapsedTime >= 10000.0f )
        {
            if ( ConnectToCoordinator() )
            {
                Protocol::MsgSetWorkerStatus msg( available );
                msg.Send( m_Connection );
                DisconnectFromCoordinator();
            }
            else
            {
                //
                // Ensure that the file will be recreated if cleanup is done on the brokerage path.
                //
                if ( !FileIO::FileExists( m_BrokerageFilePath.Get() ) )
                {
                    FileIO::EnsurePathExists( m_BrokerageRoot );

                    // create file to signify availability
                    FileStream fs;
                    fs.Open( m_BrokerageFilePath.Get(), FileStream::WRITE_ONLY );

                    // Restart the timer
                    m_TimerLastUpdate.Start();
                }
            }
        }
    }
    else if ( m_Availability != available )
    {
        if ( ConnectToCoordinator() )
        {
            Protocol::MsgSetWorkerStatus msg( available );
            msg.Send( m_Connection );
            DisconnectFromCoordinator();
        }
        else
        {
            // remove file to remove availability
            FileIO::FileDelete( m_BrokerageFilePath.Get() );
        }

        // Restart the timer
        m_TimerLastUpdate.Start();
    }
    m_Availability = available;
}

// ConnectToCoordinator
//------------------------------------------------------------------------------
bool WorkerBrokerage::ConnectToCoordinator()
{
    if ( m_CoordinatorAddress.IsEmpty() == false )
    {
        m_ConnectionPool = FNEW( WorkerConnectionPool );
        m_Connection = m_ConnectionPool->Connect( m_CoordinatorAddress, Protocol::COORDINATOR_PORT, 2000, this ); // 2000ms connection timeout
        if ( m_Connection == nullptr )
        {
            OUTPUT( "Failed to connect to the coordinator at %s\n", m_CoordinatorAddress.Get() );
            FDELETE m_ConnectionPool;
            m_ConnectionPool = nullptr;
            // m_CoordinatorAddress.Clear();
            return false;
        }

        OUTPUT( "Connected to the coordinator\n" );
        return true;
    }

    return false;
}

// DisconnectFromCoordinator
//------------------------------------------------------------------------------
void WorkerBrokerage::DisconnectFromCoordinator()
{
    if ( m_ConnectionPool )
    {
        FDELETE m_ConnectionPool;
        m_ConnectionPool = nullptr;
        m_Connection = nullptr;

        OUTPUT( "Disconnected from the coordinator\n" );
    }
}

//------------------------------------------------------------------------------
