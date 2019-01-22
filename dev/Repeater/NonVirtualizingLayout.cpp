﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "Layout.h"
#include "NonVirtualizingLayout.h"
#include "RuntimeProfiler.h"

CppWinRTActivatableClassWithBasicFactory(NonVirtualizingLayout)

NonVirtualizingLayout::NonVirtualizingLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NonVirtualizingLayout);
}

#pragma region INonVirtualizingLayoutOverrides

void NonVirtualizingLayout::InitializeForContextCore(winrt::LayoutContext const& context)
{

}

void NonVirtualizingLayout::UninitializeForContextCore(winrt::LayoutContext const& context)
{

}

winrt::Size NonVirtualizingLayout::MeasureOverride(winrt::LayoutContext const& context, winrt::Size const& availableSize)
{
    throw winrt::hresult_not_implemented();
}

winrt::Size NonVirtualizingLayout::ArrangeOverride(winrt::LayoutContext const& context, winrt::Size const& finalSize)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion
