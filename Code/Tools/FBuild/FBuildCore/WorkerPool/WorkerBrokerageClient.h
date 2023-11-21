// WorkerBrokerageClient - Client-side worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerBrokerageClient
//------------------------------------------------------------------------------
class WorkerBrokerageClient : public WorkerBrokerage
{
public:
    WorkerBrokerageClient();
    ~WorkerBrokerageClient();

    void FindWorkers( Array< AString > & outWorkerList );

    virtual void UpdateWorkerList(Array< uint32_t > &workerListUpdate) override;
protected:
    void FindWorkerFromBrokerage( Array< AString > & outWorkerList );
    void FindWorkerFromCoordinator( Array< AString > & outWorkerList );

    Array< uint32_t >   m_WorkerListUpdate;
    bool                m_WorkerListUpdateReady;
};

//------------------------------------------------------------------------------
