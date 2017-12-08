/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __CPUTRENDERPARAMSDX_H__
#define __CPUTRENDERPARAMSDX_H__

#include "CPUT.h"
#include <d3d11.h>
#include "CPUTRenderParams.h"

class CPUTRenderParametersDX : public CPUTRenderParameters
{
public:
    ID3D11DeviceContext *mpContext;

public:
    CPUTRenderParametersDX(): mpContext(NULL){}
    CPUTRenderParametersDX( ID3D11DeviceContext *pContext, bool drawModels=true, bool renderOnlyVisibleModels=true, bool showBoundingBoxes=false )
        : mpContext(pContext)
    {
        mShowBoundingBoxes       = showBoundingBoxes;
        mDrawModels              = drawModels;
        mRenderOnlyVisibleModels = renderOnlyVisibleModels;
    }
    ~CPUTRenderParametersDX(){}
};

#endif // #ifndef __CPUTRENDERPARAMSDX_H__