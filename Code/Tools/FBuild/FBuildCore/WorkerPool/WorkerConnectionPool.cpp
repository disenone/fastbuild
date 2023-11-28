// WorkerConnectionPool
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerConnectionPool.h"
#include "WorkerBrokerage.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"

// Core
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Time/Time.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerConnectionPool::WorkerConnectionPool()
    : TCPConnectionPool()
    , m_CurrentMessage( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerConnectionPool::~WorkerConnectionPool()
{
    ShutdownAllConnections();
}

// OnReceive
//------------------------------------------------------------------------------
void WorkerConnectionPool::OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory )
{
    keepMemory = true; // we'll take care of freeing the memory

    // are we expecting a msg, or the payload for a msg?
    void * payload = nullptr;
    size_t payloadSize = 0;
    if ( m_CurrentMessage == nullptr )
    {
        // message
        m_CurrentMessage = static_cast< const Protocol::IMessage * >( data );
        if ( m_CurrentMessage->HasPayload() )
        {
            return;
        }
    }
    else
    {
        // payload
        ASSERT( m_CurrentMessage->HasPayload() );
        payload = data;
        payloadSize = size;
    }

    const Protocol::IMessage * imsg = m_CurrentMessage;
    Protocol::MessageType messageType = imsg->GetType();

    PROTOCOL_DEBUG( "Coordinator : %u (%s)\n", messageType, GetProtocolMessageDebugName( messageType ) );

    switch ( messageType )
    {
        case Protocol::MSG_REQUEST_WORKER_LIST:
        {
            const Protocol::MsgRequestWorkerList * msg = static_cast< const Protocol::MsgRequestWorkerList * >( imsg );
            Process( connection, msg );
            break;
        }
        case Protocol::MSG_WORKER_LIST:
        {
            const Protocol::MsgWorkerList * msg = static_cast< const Protocol::MsgWorkerList * >( imsg );
            Process( connection, msg, payload, payloadSize );
            break;
        }
        case Protocol::MSG_SET_WORKER_STATUS:
        {
            const Protocol::MsgSetWorkerStatus * msg = static_cast< const Protocol::MsgSetWorkerStatus * >( imsg );
            Process( connection, msg );
            break;
        }
        default:
        {
            // unknown message type
            ASSERT( false ); // this indicates a protocol bug
            // AStackString<> remoteAddr;
            // TCPConnectionPool::GetAddressAsString( connection->GetRemoteAddress(), remoteAddr );
            // DIST_INFO( "Protocol Error: %s\n", remoteAddr.Get() );
            Disconnect( connection );
            break;
        }
    }

    // free everything
    FREE( (void *)( m_CurrentMessage ) );
    FREE( payload );
    m_CurrentMessage = nullptr;
}

// OnConnected
//------------------------------------------------------------------------------
void WorkerConnectionPool::OnConnected( const ConnectionInfo * connection )
{
    AStackString<> remoteAddr;
    TCPConnectionPool::GetAddressAsString( connection->GetRemoteAddress(), remoteAddr );
    // OUTPUT( "OnConnected %s\n", remoteAddr.Get() );
}

// OnDisconnected
//------------------------------------------------------------------------------
void WorkerConnectionPool::OnDisconnected( const ConnectionInfo * )
{
}

// output workers
//------------------------------------------------------------------------------
void GetWorkersString(const Array< uint32_t > & workers, AStackString<> & ret)
{
    for ( size_t i=0; i<workers.GetSize(); ++i )
    {
        AStackString<> remoteAddr;
        TCPConnectionPool::GetAddressAsString( workers[i], remoteAddr );
        if(ret.GetLength() > 0)
            ret += ", ";
        ret += remoteAddr;
    }
}

void GetWorkersString(const Array< WorkerInfo > & workers, AStackString<> & ret)
{
    for ( size_t i=0; i<workers.GetSize(); ++i )
    {
        AStackString<> remoteAddr;
        TCPConnectionPool::GetAddressAsString( workers[i].m_Address, remoteAddr );
        if(ret.GetLength() > 0)
            ret += ", ";
        ret += remoteAddr;
    }
}

// OutputCurrentWorkers
//------------------------------------------------------------------------------
void WorkerConnectionPool::OutputCurrentWorkers()
{
    AStackString<> list;
    GetWorkersString( m_Workers, list );
    OUTPUT_WITH_NOW( "current [%d] workers: [%s]\n", m_Workers.GetSize(), list.Get() );
}

// Process ( MsgRequestWorkerList )
//------------------------------------------------------------------------------
void WorkerConnectionPool::Process( const ConnectionInfo * connection, const Protocol::MsgRequestWorkerList * msg )
{
    MutexHolder mh( m_Mutex );
    ClearTimeoutWorkerWithoutMutex();

    MemoryStream ms;
    const size_t numWorkers( m_Workers.GetSize() );
    ms.Write( (uint32_t)numWorkers );
    for ( size_t i = 0; i < numWorkers; ++i )
    {
        if ( m_Workers[ i ].m_ProtocolVersion == msg->GetProtocolVersion() && m_Workers[ i ].m_Platform == msg->GetPlatform() )
        {
            ms.Write( m_Workers[ i ].m_Address );
        }
    }

    Protocol::MsgWorkerList resultMsg;
    resultMsg.Send( connection, ms );

    AStackString<> remoteAddr;
    TCPConnectionPool::GetAddressAsString( connection->GetRemoteAddress(), remoteAddr );
    
    OUTPUT_WITH_NOW( "%s request worker list\n", remoteAddr.Get());
    OutputCurrentWorkers();
}

// Process ( MsgWorkerList )
//------------------------------------------------------------------------------
void WorkerConnectionPool::Process( const ConnectionInfo * connection, const Protocol::MsgWorkerList * /*msg*/, const void * payload, size_t payloadSize )
{
    OUTPUT( "Get Worker List from Coordinator.\n");

    ConstMemoryStream ms( payload, payloadSize );

    uint32_t numWorkers( 0 );
    ms.Read( numWorkers );

    Array< uint32_t > workers;
    workers.SetCapacity( numWorkers );
    
    for ( size_t i=0; i<(size_t)numWorkers; ++i )
    {
        uint32_t workerAddress( 0 );
		ms.Read( workerAddress );
        workers.Append( workerAddress );
    }

    AStackString<> list;
    GetWorkersString( workers, list );
    OUTPUT( "%u workers in payload: [%s]\n", numWorkers, list.Get() );

    WorkerBrokerage * brokerage = ( WorkerBrokerage *)connection->GetUserData();
    ASSERT( brokerage );
    brokerage->UpdateWorkerList( workers );
}

// Process ( MsgSetWorkerStatus )
//------------------------------------------------------------------------------
void WorkerConnectionPool::Process( const ConnectionInfo * connection, const Protocol::MsgSetWorkerStatus * msg )
{
    MutexHolder mh( m_Mutex );

    bool need_log = false;
    const uint32_t workerAddress = connection->GetRemoteAddress();
    if ( msg->IsAvailable() )
    {
        if ( m_Workers.Find( workerAddress ) == nullptr )
        {
            AStackString<> remoteAddr;
            TCPConnectionPool::GetAddressAsString( workerAddress, remoteAddr );
            OUTPUT_WITH_NOW( "New worker available: %s\n", remoteAddr.Get() );
            m_Workers.Append( WorkerInfo( workerAddress, msg->GetProtocolVersion(), msg->GetPlatform() ) );
            need_log = true;
        }
    }
    else
    {
        if( m_Workers.FindAndErase( workerAddress ) ) need_log = true;
    }

    WorkerInfo * info = m_Workers.Find(workerAddress);
    if ( info ) info->m_LastHeartTick = Time::GetNow();
    need_log = ClearTimeoutWorkerWithoutMutex() || need_log;

    if ( need_log )
    {
        OutputCurrentWorkers();
    }
}

// ClearTimeoutWorker
//------------------------------------------------------------------------------
bool WorkerConnectionPool::ClearTimeoutWorker()
{
    MutexHolder mh( m_Mutex );
    return ClearTimeoutWorkerWithoutMutex();
}

// ClearTimeoutWorker
//------------------------------------------------------------------------------
bool WorkerConnectionPool::ClearTimeoutWorkerWithoutMutex()
{
    const int16_t timeout = 30;
    Array< WorkerInfo > newWorkers;
    int64_t now = Time::GetNow();
    bool hasTimeout = false;

    for ( const auto & worker_info: m_Workers )
    {
        if ( now - worker_info.m_LastHeartTick < 30 )
        {
            newWorkers.Append(worker_info);
        }
        else
        {
            hasTimeout = true;
            AStackString<> remoteAddr;
            TCPConnectionPool::GetAddressAsString( worker_info.m_Address, remoteAddr );
            OUTPUT_WITH_NOW( "worker timeout: %s\n", remoteAddr.Get() );
        }
    }

    m_Workers.Swap(newWorkers);

    if ( hasTimeout ) OutputCurrentWorkers();
    return hasTimeout;
}

//------------------------------------------------------------------------------
