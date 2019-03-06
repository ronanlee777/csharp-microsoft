﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ViewModel.h"
#include "TreeViewNode.h"

class MultiLevelListViewBase
{
public:
    MultiLevelListViewBase(const ITrackerHandleManager* m_owner, const winrt::ListView& lv);

    winrt::com_ptr<ViewModel> ListViewModel() const;
    void ListViewModel(winrt::com_ptr<ViewModel> const& viewModel);

    winrt::TreeViewNode NodeAtFlatIndex(int index) const;
    winrt::TreeViewNode NodeFromContainer(winrt::DependencyObject const& container);
    winrt::TreeViewNode NodeFromItem(winrt::IInspectable const& item);
    winrt::DependencyObject ContainerFromNode(winrt::TreeViewNode const& node);

private:
    tracker_com_ref<ViewModel> m_viewModel;
    winrt::weak_ref<winrt::ListView> m_listView{ nullptr };
};
