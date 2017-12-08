/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
/////////////////////////////////////////////////////////////////////////////////////////////


#include "CPUTTexture.h"
#ifdef CPUT_FOR_DX11
#include "CPUTTextureDX11.h"
#elif (defined(CPUT_FOR_OGL) || defined(CPUT_FOR_OGLES))
#include "CPUTTextureOGL.h"
#else
    #error You must supply a target graphics API (ex: #define CPUT_FOR_DX11), or implement the target API for this file.
#endif


//--------------------------------------------------------------------------------------
CPUTTexture *CPUTTexture::CreateTexture( const cString &name, const cString absolutePathAndFilename, bool loadAsSRGB )
{
    // TODO: accept DX11/OGL param to control which platform we generate.
    // TODO: be sure to support the case where we want to support only one of them
#ifdef CPUT_FOR_DX11
    return CPUTTextureDX11::CreateTexture( name, absolutePathAndFilename, loadAsSRGB );
#elif (defined(CPUT_FOR_OGL) || defined(CPUT_FOR_OGLES))
    return CPUTTextureOGL::CreateTexture( name, absolutePathAndFilename, loadAsSRGB );
#else    
    #error You must supply a target graphics API (ex: #define CPUT_FOR_DX11), or implement the target API for this file.
#endif
    
}