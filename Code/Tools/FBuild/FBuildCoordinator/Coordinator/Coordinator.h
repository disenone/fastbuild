// Coordinator
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------

// Core
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class WorkerConnectionPool;

// Coordinator
//------------------------------------------------------------------------------
class Coordinator
{
public:
 
    explicit Coordinator( const AString & args );
    ~Coordinator();

    uint32_t Start();

private:
    static uint32_t WorkThreadWrapper( void * userData );
    uint32_t WorkThread();

    AString                 m_BaseArgs;
    WorkerConnectionPool    * m_ConnectionPool;
    Thread                  m_WorkThread;
};

//------------------------------------------------------------------------------
