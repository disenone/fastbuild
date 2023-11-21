// FBuildCoordinatorOptions
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Env/Types.h"

// Forward Declaration
//------------------------------------------------------------------------------
class AString;

// FBuildCoordinatorOptions
//------------------------------------------------------------------------------
class FBuildCoordinatorOptions
{
public:
    FBuildCoordinatorOptions();

    bool ProcessCommandLine( const AString & commandLine );

private:
    void ShowUsageError();
};

//------------------------------------------------------------------------------
