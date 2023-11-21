// FBuildCoordinatorOptions
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildCoordinatorOptions.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// FBuildCoordinatorOptions (CONSTRUCTOR)
//------------------------------------------------------------------------------
FBuildCoordinatorOptions::FBuildCoordinatorOptions()
{
}

// ProcessCommandLine
//------------------------------------------------------------------------------
bool FBuildCoordinatorOptions::ProcessCommandLine( const AString & commandLine )
{
    // Tokenize
    Array< AString > tokens;
    commandLine.Tokenize( tokens );

    return true;
}

// ShowUsageError
//------------------------------------------------------------------------------
void FBuildCoordinatorOptions::ShowUsageError()
{
    OUTPUT( "FBuildCoordinator - " FBUILD_VERSION_STRING " - "
            "Copyright 2012-2019 Franta Fulin - http://www.fastbuild.org\n" );
}

//------------------------------------------------------------------------------
