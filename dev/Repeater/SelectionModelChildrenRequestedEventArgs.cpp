﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "SelectionNode.h"
#include "SelectionModelChildrenRequestedEventArgs.h"

SelectionModelChildrenRequestedEventArgs::SelectionModelChildrenRequestedEventArgs(const winrt::IInspectable& source, const std::weak_ptr<SelectionNode>& sourceNode)
{
    Initialize(source, sourceNode);
}

#pragma region ISelectionModelChildrenRequestedEventArgs

winrt::IInspectable SelectionModelChildrenRequestedEventArgs::Source()
{
    auto node = m_sourceNode.lock();
    if (!node)
    {
        throw winrt::hresult_error(E_FAIL, L"Source can only be accesed in the ChildrenRequested event handler.");
    }

    return m_source.get();
}

winrt::IndexPath SelectionModelChildrenRequestedEventArgs::SourceIndex()
{
    auto node = m_sourceNode.lock();
    if (!node)
    {
        throw winrt::hresult_error(E_FAIL, L"SourceIndex can only be accesed in the ChildrenRequested event handler.");
    }

    return node->IndexPath();
}

winrt::IInspectable SelectionModelChildrenRequestedEventArgs::Children()
{
    return m_children.get();
}

void SelectionModelChildrenRequestedEventArgs::Children(winrt::IInspectable const& value)
{
    m_children.set(value);
}

#pragma endregion

void SelectionModelChildrenRequestedEventArgs::Initialize(const winrt::IInspectable& source, const std::weak_ptr<SelectionNode>& sourceNode)
{
    m_source.set(source);
    m_sourceNode = sourceNode;
    m_children.set(nullptr);
}
