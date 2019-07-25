﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "SplitDataSourceBase.h"
#include "TopNavigationViewDataProvider.h"
#include "InspectingDataSource.h"
#include "NavigationViewItem.h"

TopNavigationViewDataProvider::TopNavigationViewDataProvider(const ITrackerHandleManager* m_owner)
    :SplitDataSourceT()
    , m_rawDataSource(m_owner)
    , m_dataSource(m_owner)
{
    auto lambda = [this](const winrt::IInspectable& value)
    {
        return IndexOf(value);
    };

    auto primaryVector = std::make_shared<SplitVectorT>(m_owner, PrimaryList, lambda);
    auto overflowVector = std::make_shared<SplitVectorT>(m_owner, OverflowList, lambda);
    
    InitializeSplitVectors({ primaryVector, overflowVector });
}


winrt::IVector<winrt::IInspectable> TopNavigationViewDataProvider::GetPrimaryItems()
{
    return GetVector(PrimaryList)->GetVector();
}

winrt::IVector<winrt::IInspectable> TopNavigationViewDataProvider::GetOverflowItems()
{
    return GetVector(OverflowList)->GetVector();
}

// The raw data is from MenuItems or MenuItemsSource
void TopNavigationViewDataProvider::SetDataSource(const winrt::IInspectable& rawData)
{
    if (ShouldChangeDataSource(rawData)) // avoid to create multiple of datasource for the same raw data
    {        
        winrt::ItemsSourceView dataSource = nullptr;
        if (rawData)
        {
            dataSource = winrt::ItemsSourceView(rawData);
        }
        ChangeDataSource(dataSource);
        m_rawDataSource.set(rawData);
        if (dataSource)
        {
            MoveAllItemsToPrimaryList();
        }
    }
}

bool TopNavigationViewDataProvider::ShouldChangeDataSource(winrt::IInspectable const& rawData)
{
    return rawData != m_rawDataSource.get();    
}

void TopNavigationViewDataProvider::OnRawDataChanged(std::function<void(winrt::NotifyCollectionChangedEventArgs const& args)> const& dataChangeCallback)
{
    m_dataChangeCallback = dataChangeCallback;
}

int TopNavigationViewDataProvider::IndexOf(const winrt::IInspectable& value)
{
    if (auto dataSource = m_dataSource.get())
    {
        auto inspectingDataSource = static_cast<InspectingDataSource*>(winrt::get_self<ItemsSourceView>(dataSource));

        return inspectingDataSource->IndexOf(value);
    }
    return -1;
}

winrt::IInspectable TopNavigationViewDataProvider::GetAt(int index)
{
    if (auto dataSource = m_dataSource.get())
    {
        return dataSource.GetAt(index);
    }
    return nullptr;
}

int TopNavigationViewDataProvider::Size()
{
    if (auto dataSource = m_dataSource.get())
    {
        return static_cast<int>(dataSource.Count());
    }
    return 0;
}

NavigationViewSplitVectorID TopNavigationViewDataProvider::DefaultVectorIDOnInsert()
{
    return NotInitialized;
}

float TopNavigationViewDataProvider::DefaultAttachedData()
{
    return std::numeric_limits<float>::min();
}

void TopNavigationViewDataProvider::MoveAllItemsToPrimaryList()
{
    for (int i = 0; i < Size(); i++)
    {
        MoveItemToVector(i, PrimaryList);
    }
}

std::vector<int> TopNavigationViewDataProvider::ConvertPrimaryIndexToIndex(std::vector<int> const& indexesInPrimary)
{
    std::vector<int> indexes;
    if (!indexesInPrimary.empty())
    {
        auto vector = GetVector(PrimaryList);
        if (vector)
        {
            // transform PrimaryList index to OrignalVector index
            std::transform(indexesInPrimary.begin(), indexesInPrimary.end(), std::back_inserter(indexes),
                [&vector](int index) -> int
            {
                return vector->IndexToIndexInOriginalVector(index);
            });
        }
    }
    return indexes;
}

void TopNavigationViewDataProvider::MoveItemsOutOfPrimaryList(std::vector<int> const& indexes)
{
    MoveItemsToList(indexes, OverflowList);
}

void TopNavigationViewDataProvider::MoveItemsToPrimaryList(std::vector<int> const& indexes)
{
    MoveItemsToList(indexes, PrimaryList);
}

void TopNavigationViewDataProvider::MoveItemsToList(std::vector<int> const& indexes, NavigationViewSplitVectorID vectorID)
{
    for (auto &index : indexes)
    {
        MoveItemToVector(index, vectorID);
    };
}

int TopNavigationViewDataProvider::GetPrimaryListSize()
{
    return GetPrimaryItems().Size();
}

int TopNavigationViewDataProvider::GetNavigationViewItemCountInPrimaryList()
{
    int count = 0;
    for (int i = 0; i < Size(); i++)
    {
        if (IsItemInPrimaryList(i) && IsContainerNavigationViewItem(i))
        {
            count++;
        }
    }
    return count;
}

int TopNavigationViewDataProvider::GetNavigationViewItemCountInTopNav()
{
    int count = 0;
    for (int i = 0; i < Size(); i++)
    {
        if (IsContainerNavigationViewItem(i))
        {
            count++;
        }
    }
    return count;
}

void TopNavigationViewDataProvider::UpdateWidthForPrimaryItem(int indexInPrimary, float width)
{
    auto vector = GetVector(PrimaryList);
    if (vector)
    {
        auto index = vector->IndexToIndexInOriginalVector(indexInPrimary);
        SetWidthForItem(index, width);
    }
}

float TopNavigationViewDataProvider::WidthRequiredToRecoveryAllItemsToPrimary()
{
    auto width = 0.f;
    for (int i=0; i<Size(); i++)
    {
        if (!IsItemInPrimaryList(i))
        {
            width += GetWidthForItem(i);
        }
    }
    width -= m_overflowButtonCachedWidth;
    return std::max(0.f, width);
}

bool TopNavigationViewDataProvider::HasInvalidWidth(std::vector<int> & items)
{
    bool hasInvalidWidth = false;
    for (auto &index : items)
    {
        if (!IsValidWidthForItem(index))
        {
            hasInvalidWidth = true;
            break;
        }
    }
    return hasInvalidWidth;
}

float TopNavigationViewDataProvider::GetWidthForItem(int index)
{
    auto width = AttachedData(index);
    if (!IsValidWidth(width))
    {
        width = 0;
    }
    return width;
}

float TopNavigationViewDataProvider::CalculateWidthForItems(std::vector<int> &items)
{
    float width = 0.f;
    for (auto &index : items)
    {
        width += GetWidthForItem(index);
    }
    return width;
}

void TopNavigationViewDataProvider::InvalidWidthCache()
{
    ResetAttachedData(-1.0f);
}

float TopNavigationViewDataProvider::OverflowButtonWidth()
{
    return m_overflowButtonCachedWidth;
}

void TopNavigationViewDataProvider::OverflowButtonWidth(float width)
{
    m_overflowButtonCachedWidth = width;
}

bool TopNavigationViewDataProvider::IsItemSelectableInPrimaryList(const winrt::IInspectable& value)
{
    int index = IndexOf(value);
    return (index != -1);
}

int TopNavigationViewDataProvider::IndexOf(const winrt::IInspectable& value, NavigationViewSplitVectorID vectorID)
{
    return IndexOfImpl(value, vectorID);
}

void TopNavigationViewDataProvider::OnDataSourceChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args)
{
    switch (args.Action())
    {
        case winrt::NotifyCollectionChangedAction::Add:
        {   
            OnInsertAt(args.NewStartingIndex(), args.NewItems().Size());
            break;
        }

        case winrt::NotifyCollectionChangedAction::Remove:
        {
            OnRemoveAt(args.OldStartingIndex(), args.OldItems().Size());
            break;
        }

        case winrt::NotifyCollectionChangedAction::Reset:
        {
            OnClear();
            break;
        }

        case winrt::NotifyCollectionChangedAction::Replace:
        {
            OnRemoveAt(args.OldStartingIndex(), args.OldItems().Size());
            OnInsertAt(args.NewStartingIndex(), args.NewItems().Size());
            break;
        }
    }
    if (m_dataChangeCallback)
    {
        m_dataChangeCallback(args);
    }
}

bool TopNavigationViewDataProvider::IsValidWidth(float width)
{
    return (width >= 0) && (width < std::numeric_limits<float>::max());
}

bool TopNavigationViewDataProvider::IsValidWidthForItem(int index)
{
    auto width = AttachedData(index);
    return IsValidWidth(width);
}

void TopNavigationViewDataProvider::InvalidWidthCacheIfOverflowItemContentChanged()
{
    bool shouldRefreshCache = false;
    for (int i = 0; i < Size(); i++)
    {
        if (!IsItemInPrimaryList(i))
        {
            if (auto navItem = GetAt(i).try_as<winrt::NavigationViewItem>())
            {
                auto itemPointer = winrt::get_self<NavigationViewItem>(navItem);
                if (itemPointer->IsContentChangeHandlingDelayedForTopNav())
                {
                    itemPointer->ClearIsContentChangeHandlingDelayedForTopNavFlag();
                    shouldRefreshCache = true;
                }
            }
        }
    }

    if (shouldRefreshCache)
    {
        InvalidWidthCache();
    }
}

void TopNavigationViewDataProvider::SetWidthForItem(int index, float width)
{
    if (IsValidWidth(width))
    {
        AttachedData(index, width);
    }
}

void TopNavigationViewDataProvider::ChangeDataSource(winrt::ItemsSourceView newValue)
{
    auto oldValue = m_dataSource.get();
    if (oldValue != newValue)
    {
        // update to the new datasource.

        if (oldValue)
        {
            oldValue.CollectionChanged(m_dataSourceChanged);
        }

        Clear();

        m_dataSource.set(newValue);
        SyncAndInitVectorFlagsWithID(NotInitialized, DefaultAttachedData());

        if (newValue)
        {
            m_dataSourceChanged = newValue.CollectionChanged({ this, &TopNavigationViewDataProvider::OnDataSourceChanged });
        }
    }

    // Move all to primary list
    MoveItemsToVector(NotInitialized);
}

bool TopNavigationViewDataProvider::IsItemInPrimaryList(int index)
{
    return GetVectorIDForItem(index) == PrimaryList;
}

bool TopNavigationViewDataProvider::IsContainerNavigationViewItem(int index)
{
    bool isContainerNavigationViewItem = true;

    auto item = GetAt(index);
    if (item && (item.try_as<winrt::NavigationViewItemHeader>() || item.try_as<winrt::NavigationViewItemSeparator>()))
    {
        isContainerNavigationViewItem = false;
    }
    return isContainerNavigationViewItem;
}

bool TopNavigationViewDataProvider::IsContainerNavigationViewHeader(int index)
{
    bool isContainerNavigationViewHeader = false;

    auto item = GetAt(index);
    if (item && item.try_as<winrt::NavigationViewItemHeader>())
    {
        isContainerNavigationViewHeader = true;
    }
    return isContainerNavigationViewHeader;
}
