// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class WorkerConnectionPool;
class ConnectionInfo;

// WorkerBrokerage
//------------------------------------------------------------------------------
class WorkerBrokerage
{
public:
    WorkerBrokerage();
    ~WorkerBrokerage();

    inline const AString & GetBrokerageRoot() const { return m_BrokerageRoot; }

    // client interface
    void FindWorkers( Array< AString > & workerList );
    void UpdateWorkerList( Array< uint32_t > &workerListUpdate );

    // server interface
    void SetAvailability( bool available );
private:
    void Init();

    bool ConnectToCoordinator();
    void DisconnectFromCoordinator();

    AString             m_BrokerageRoot;
    bool                m_Availability;
    bool                m_Initialized;
    AString             m_HostName;
    AString             m_BrokerageFilePath;
    AString             m_CoordinatorAddress;
    WorkerConnectionPool * m_ConnectionPool;
    const ConnectionInfo * m_Connection;
    Timer               m_TimerLastUpdate;      // Throttle network access
    Array< uint32_t >   m_WorkerListUpdate;
    bool                m_WorkerListUpdateReady;
};

//------------------------------------------------------------------------------
