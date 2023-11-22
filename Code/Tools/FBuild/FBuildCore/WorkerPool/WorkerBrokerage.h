// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

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
    virtual ~WorkerBrokerage();

    const AString & GetWorkingPath() const 
    { 
        if ( !m_CoordinatorAddress.IsEmpty() ) return m_CoordinatorAddress;
        return m_BrokerageRootPaths;
    }
    const AString & GetBrokerageRootPaths() const { return m_BrokerageRootPaths; }
    const AString & GetCoordinatorAddress() const { return m_CoordinatorAddress; }
    void SetCoordinatorAddress(const AString & address) { m_CoordinatorAddress = address; }
    void SetBrokeragePath(const AString & path) 
    { 
        m_BrokerageRoots.Clear();
        m_BrokerageRoots.Append(path);
        m_BrokerageRootPaths.Clear();
        m_BrokerageRootPaths.Append(path);
    }

    virtual void UpdateWorkerList(Array< uint32_t > &) {}

protected:
    void InitBrokerage();

    bool ConnectToCoordinator();
    void DisconnectFromCoordinator();

    Array<AString>      m_BrokerageRoots;
    AString             m_BrokerageRootPaths;
    bool                m_BrokerageInitialized;

    AString             m_HostName;
    AString             m_CoordinatorAddress;
    WorkerConnectionPool * m_ConnectionPool;
    const ConnectionInfo * m_Connection;
};

//------------------------------------------------------------------------------
