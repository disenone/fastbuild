// WorkerConnectionPool
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------

// Core
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Time/Time.h"

// Forward Declarations
//------------------------------------------------------------------------------
namespace Protocol
{
    class IMessage;
    class MsgRequestWorkerList;
    class MsgWorkerList;
    class MsgSetWorkerStatus;
}

// WorkerInfo
//------------------------------------------------------------------------------
struct WorkerInfo
{
    WorkerInfo( uint32_t address, uint32_t protocolVersion, uint8_t platform )
        : m_Address( address )
        , m_ProtocolVersion( protocolVersion )
        , m_Platform( platform )
        , m_LastHeartTick( Time::GetNow() )
    {}

    bool operator == ( uint32_t address ) const { return address == m_Address; }

    uint32_t    m_Address;
    uint32_t    m_ProtocolVersion;
    uint8_t     m_Platform;
    int64_t     m_LastHeartTick;
};

// WorkerConnectionPool
//------------------------------------------------------------------------------
class WorkerConnectionPool : public TCPConnectionPool
{
public:
    WorkerConnectionPool();
    virtual ~WorkerConnectionPool();
    bool ClearTimeoutWorker();
    void OutputCurrentWorkers();

private:
    // network events - NOTE: these happen in another thread! (but never at the same time)
    virtual void OnReceive( const ConnectionInfo *, void * /*data*/, uint32_t /*size*/, bool & /*keepMemory*/ ) override;
    virtual void OnConnected( const ConnectionInfo * ) override;
    virtual void OnDisconnected( const ConnectionInfo * ) override;

    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestWorkerList * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgWorkerList * msg, const void * payload, size_t payloadSize );
    void Process( const ConnectionInfo * connection, const Protocol::MsgSetWorkerStatus * msg );
    bool ClearTimeoutWorkerWithoutMutex();

    Mutex                       m_Mutex;
    Array< WorkerInfo >         m_Workers;
    const Protocol::IMessage    * m_CurrentMessage;
};

//------------------------------------------------------------------------------
