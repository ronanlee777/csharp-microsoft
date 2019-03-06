﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MultiLevelListViewBase.h"

MultiLevelListViewBase::MultiLevelListViewBase(const ITrackerHandleManager* m_owner, const winrt::ListView& lv)
    : m_viewModel(m_owner)
{
    m_listView = winrt::make_weak<winrt::ListView>(lv);
}

com_ptr<ViewModel> MultiLevelListViewBase::ListViewModel() const
{
    return m_viewModel.get();
}

void MultiLevelListViewBase::ListViewModel(com_ptr<ViewModel> const& viewModel)
{
    m_viewModel.set(viewModel);
}

winrt::TreeViewNode MultiLevelListViewBase::NodeAtFlatIndex(int index) const
{
    if (auto viewModel = ListViewModel()) {
        if (index >= 0 && index < static_cast<int32_t>(viewModel->Size()))
        {
            return viewModel->GetNodeAt(index);
        }
    }
    return nullptr;
}

winrt::TreeViewNode MultiLevelListViewBase::NodeFromContainer(winrt::DependencyObject const& container)
{
    int index = m_listView.get().IndexFromContainer(container);
    return NodeAtFlatIndex(index);
}

winrt::DependencyObject MultiLevelListViewBase::ContainerFromNode(winrt::TreeViewNode const& node)
{
    return m_listView.get().ContainerFromItem(node.Content());
}

winrt::TreeViewNode MultiLevelListViewBase::NodeFromItem(winrt::IInspectable const& item)
{
    if (auto container = m_listView.get().ContainerFromItem(item))
    {
        return NodeFromContainer(container);
    }
    return nullptr;
}
