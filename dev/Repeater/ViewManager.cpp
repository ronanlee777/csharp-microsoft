﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ViewManager.h"
#include "ItemsRepeater.h"
#include "ElementFactoryGetArgs.h"
#include "ElementFactoryRecycleArgs.h"

ViewManager::ViewManager(ItemsRepeater* owner) :
    m_owner(owner),
    m_resetPool(owner),
    m_lastFocusedElement(owner),
    m_phaser(owner),
    m_ElementFactoryGetArgs(owner),
    m_ElementFactoryRecycleArgs(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

winrt::UIElement ViewManager::GetElement(int index, bool forceCreate, bool suppressAutoRecycle)
{
    winrt::UIElement element = forceCreate ? nullptr : GetElementIfAlreadyHeldByLayout(index);
    if(!element)
    {
        // check if this is the anchor made through repeater in preparation 
        // for a bring into view.
        if (auto madeAnchor = m_owner->MadeAnchor())
        {
            auto anchorVirtInfo = ItemsRepeater::TryGetVirtualizationInfo(madeAnchor);
            if (anchorVirtInfo->Index() == index)
            {
                element = madeAnchor;
            }
        }
    }
    if (!element) { element = GetElementFromUniqueIdResetPool(index); };
    if (!element) { element = GetElementFromPinnedElements(index); }
    if (!element) { element = GetElementFromElementFactory(index); }

    auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(element);
    if (suppressAutoRecycle)
    {
        virtInfo->AutoRecycleCandidate(false);
        REPEATER_TRACE_INFO(L"%* GetElement: %d Not AutoRecycleCandidate: \n", m_owner->Indent(), virtInfo->Index());
    }
    else
    {
        virtInfo->AutoRecycleCandidate(true);
        virtInfo->KeepAlive(true);
        REPEATER_TRACE_INFO(L"%* GetElement: %d AutoRecycleCandidate: \n", m_owner->Indent(), virtInfo->Index());
    }

    return element;
}

void ViewManager::ClearElement(const winrt::UIElement& element, bool isClearedDueToCollectionChange)
{
    auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
    const int index = virtInfo->Index();
    bool cleared =
        ClearElementToUniqueIdResetPool(element, virtInfo) ||
        ClearElementToAnimator(element, virtInfo) ||
        ClearElementToPinnedPool(element, virtInfo, isClearedDueToCollectionChange);

    if (!cleared)
    {
        ClearElementToElementFactory(element);
    }

    // Both First and Last indices need to be valid or default.
    MUX_ASSERT((m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout == LastRealizedElementIndexDefault) ||
        (m_firstRealizedElementIndexHeldByLayout != FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout != LastRealizedElementIndexDefault));

    if (index == m_firstRealizedElementIndexHeldByLayout && index == m_lastRealizedElementIndexHeldByLayout)
    {
        // First and last were pointing to the same element and that is going away.
        InvalidateRealizedIndicesHeldByLayout();
    }
    else if (index == m_firstRealizedElementIndexHeldByLayout)
    {
        // The FirstElement is going away, shrink the range by one.
        ++m_firstRealizedElementIndexHeldByLayout;
    }
    else if (index == m_lastRealizedElementIndexHeldByLayout)
    {
        // Last element is going away, shrink the range by one at the end.
        --m_lastRealizedElementIndexHeldByLayout;
    }
    else
    {
        // Index is either outside the range we are keeping track of or inside the range.
        // In both these cases, we just keep the range we have. If this clear was due to 
        // a collection change, then in the CollectionChanged event, we will invalidate these guys.
    }
}

void ViewManager::ClearElementToElementFactory(const winrt::UIElement& element)
{
    m_owner->OnElementClearing(element);

    if (m_owner->ItemTemplateShim())
    {
        if (!m_ElementFactoryRecycleArgs)
        {
            // Create one.
            m_ElementFactoryRecycleArgs = tracker_ref<winrt::ElementFactoryRecycleArgs>(m_owner, *winrt::make_self<ElementFactoryRecycleArgs>());
        }

        auto context = m_ElementFactoryRecycleArgs.get();
        context.Element(element);
        context.Parent(*m_owner);

        m_owner->ItemTemplateShim().RecycleElement(context);

        context.Element(nullptr);
        context.Parent(nullptr);
    }
    else
    {
        // No ItemTemplate to recycle to, remove the element from the children collection.
        auto children = m_owner->Children();
        unsigned int childIndex = 0;
        bool found = children.IndexOf(element, childIndex);
        if (!found)
        {
            throw winrt::hresult_error(E_FAIL, L"ItemsRepeater's child not found in its Children collection.");
        }

        children.RemoveAt(childIndex);
    }

    auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);    
    virtInfo->MoveOwnershipToElementFactory();
    m_phaser.StopPhasing(element, virtInfo);
    if (m_lastFocusedElement == element)
    {
        // Focused element is going away. Remove the tracked last focused element
        // and pick a reasonable next focus if we can find one within the layout 
        // realized elements.
        const int clearedIndex = virtInfo->Index();
        MoveFocusFromClearedIndex(clearedIndex);
    }

    REPEATER_TRACE_PERF(L"ElementCleared");
}

void ViewManager::MoveFocusFromClearedIndex(int clearedIndex)
{
    winrt::UIElement focusedChild = nullptr;
    if (auto focusCandidate = FindFocusCandidate(clearedIndex, focusedChild))
    {
        winrt::FocusState focusState = winrt::FocusState::Programmatic;
        if (m_lastFocusedElement)
        {
            if (auto focusedAsControl = m_lastFocusedElement.try_as<winrt::Control>())
            {
                focusState = focusedAsControl.FocusState();
            }
        }

        // If the last focused element has focus, use its focus state, if not use programmatic.
        focusState = focusState == winrt::FocusState::Unfocused ? winrt::FocusState::Programmatic : focusState;
        focusCandidate.Focus(focusState);

        m_lastFocusedElement.set(focusedChild);
        // Add pin to hold the focused element.
        UpdatePin(focusedChild, true /* addPin */);
    }
    else
    {
        // We could not find a candiate.
        m_lastFocusedElement.set(nullptr);
    }
}

winrt::Control ViewManager::FindFocusCandidate(int clearedIndex, winrt::UIElement& focusedChild)
{
    // Walk through all the children and find elements with index before and after the cleared index.
    // Note that during a delete the next element would now have the same index.
    int previousIndex = std::numeric_limits<int>::min();
    int nextIndex = std::numeric_limits<int>::max();
    winrt::UIElement nextElement = nullptr;
    winrt::UIElement previousElement = nullptr;
    auto children = m_owner->Children();
    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(child);
        if (virtInfo && virtInfo->IsHeldByLayout())
        {
            const int currentIndex = virtInfo->Index();
            if (currentIndex < clearedIndex)
            {
                if (currentIndex > previousIndex)
                {
                    previousIndex = currentIndex;
                    previousElement = child;
                }
            }
            else if (currentIndex >= clearedIndex)
            {
                // Note that we use >= above because if we deleted the focused element, 
                // the next element would have the same index now.
                if (currentIndex < nextIndex)
                {
                    nextIndex = currentIndex;
                    nextElement = child;
                }
            }
        }
    }

    // Find the next element if one exists, if not use the previous element.
    // If the container itself is not focusable, find a descendent that is.
    winrt::Control focusCandidate = nullptr;
    if (nextElement)
    {
        focusedChild = nextElement.try_as<winrt::UIElement>();
        focusCandidate = nextElement.try_as<winrt::Control>();
        if (!focusCandidate)
        {
            if (auto firstFocus = winrt::FocusManager::FindFirstFocusableElement(nextElement))
            {
                focusCandidate = firstFocus.try_as<winrt::Control>();
            }
        }
    }

    if (!focusCandidate && previousElement)
    {
        focusedChild = previousElement.try_as<winrt::UIElement>();
        focusCandidate = previousElement.try_as<winrt::Control>();
        if (!previousElement)
        {
            if (auto lastFocus = winrt::FocusManager::FindLastFocusableElement(previousElement))
            {
                focusCandidate = lastFocus.try_as<winrt::Control>();
            }
        }
    }

    return focusCandidate;
}

int ViewManager::GetElementIndex(const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    if (!virtInfo)
    {
        //Element is not a child of this ItemsRepeater.
        return -1;
    }

    return virtInfo->IsRealized() || virtInfo->IsInUniqueIdResetPool() ? virtInfo->Index() : -1;
}

void ViewManager::PrunePinnedElements()
{
    EnsureEventSubscriptions();

    // Go through pinned elements and make sure they still have
    // a reason to be pinned.
    for (size_t i = 0; i < m_pinnedPool.size(); ++i)
    {
        auto elementInfo = m_pinnedPool[i];
        auto virtInfo = elementInfo.VirtualizationInfo();

        MUX_ASSERT(virtInfo->Owner() == ElementOwner::PinnedPool);

        if (!virtInfo->IsPinned())
        {
            m_pinnedPool.erase(m_pinnedPool.begin() + i);
            --i;

            // Pinning was the only thing keeping this element alive.
            ClearElementToElementFactory(elementInfo.PinnedElement());
        }
    }
}

void ViewManager::UpdatePin(const winrt::UIElement& element, bool addPin)
{
    auto parent = CachedVisualTreeHelpers::GetParent(element);
    auto child = static_cast<winrt::DependencyObject>(element);

    while (parent)
    {
        if (auto repeater = parent.try_as<winrt::ItemsRepeater>())
        {
            auto virtInfo = ItemsRepeater::GetVirtualizationInfo(child.as<winrt::UIElement>());
            if (virtInfo->IsRealized())
            {
                if (addPin)
                {
                    virtInfo->AddPin();
                }
                else if(virtInfo->IsPinned())
                {
                    if (virtInfo->RemovePin() == 0)
                    {
                        // ElementFactory is invoked during the measure pass.
                        // We will clear the element then.
                        repeater.InvalidateMeasure();
                    }
                }
            }
        }

        child = parent;
        parent = CachedVisualTreeHelpers::GetParent(child);
    }
}

void ViewManager::OnItemsSourceChanged(const winrt::IInspectable&, const winrt::NotifyCollectionChangedEventArgs& args)
{
    // Note: For items that have been removed, the index will not be touched. It will hold
    // the old index before it was removed. It is not valid anymore.
    switch (args.Action())
    {
    case winrt::NotifyCollectionChangedAction::Add:
    {
        auto newIndex = args.NewStartingIndex();
        auto newCount = args.NewItems().Size();
        EnsureFirstLastRealizedIndices();
        if (newIndex <= m_lastRealizedElementIndexHeldByLayout)
        {
            m_lastRealizedElementIndexHeldByLayout += newCount;
            auto children = m_owner->Children();
            auto childCount = children.Size();
            for (unsigned i = 0u; i < childCount; ++i)
            {
                auto element = children.GetAt(i);
                auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
                auto dataIndex = virtInfo->Index();

                if (virtInfo->IsRealized() && dataIndex >= newIndex)
                {
                    UpdateElementIndex(element, virtInfo, dataIndex + newCount);
                }
            }
        }
        else
        {
            // Indices held by layout are not affected
            // We could still have items in the pinned elements that need updates. This is usually a very small vector.
            for (size_t i = 0; i < m_pinnedPool.size(); ++i)
            {
                auto elementInfo = m_pinnedPool[i];
                auto virtInfo = elementInfo.VirtualizationInfo();
                auto dataIndex = virtInfo->Index();

                if (virtInfo->IsRealized() && dataIndex >= newIndex)
                {
                    auto element = elementInfo.PinnedElement();
                    UpdateElementIndex(element, virtInfo, dataIndex + newCount);
                }
            }
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Replace:
    {
        // Requirement: oldStartIndex == newStartIndex. It is not a replace if this is not true.
        // Two cases here
        // case 1: oldCount == newCount 
        //         indices are not affected. nothing to do here.  
        // case 2: oldCount != newCount
        //         Replaced with less or more items. This is like an insert or remove
        //         depending on the counts.
        auto oldStartIndex = args.OldStartingIndex();
        auto newStartingIndex = args.NewStartingIndex();
        auto oldCount = static_cast<int>(args.OldItems().Size());
        auto newCount = static_cast<int>(args.NewItems().Size());
        if (oldStartIndex != newStartingIndex)
        {
            throw winrt::hresult_error(E_FAIL, L"Replace is only allowed with OldStartingIndex equals to NewStartingIndex.");
        }

        if (oldCount == 0)
        {
            throw winrt::hresult_error(E_FAIL, L"Replace notification with args.OldItemsCount value of 0 is not allowed. Use Insert action instead.");
        }

        if (newCount == 0)
        {
            throw winrt::hresult_error(E_FAIL, L"Replace notification with args.NewItemCount value of 0 is not allowed. Use Remove action instead.");
        }

        int countChange = newCount - oldCount;
        if (countChange != 0)
        {
            // countChange > 0 : countChange items were added
            // countChange < 0 : -countChange  items were removed
            auto children = m_owner->Children();
            for (unsigned i = 0u; i < children.Size(); ++i)
            {
                auto element = children.GetAt(i);
                auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
                auto dataIndex = virtInfo->Index();

                if (virtInfo->IsRealized())
                {
                    if (dataIndex >= oldStartIndex + oldCount)
                    {
                        UpdateElementIndex(element, virtInfo, dataIndex + countChange);
                    }
                }
            }

            EnsureFirstLastRealizedIndices();
            m_lastRealizedElementIndexHeldByLayout += countChange;
        }
        break;
    }

    case winrt::NotifyCollectionChangedAction::Remove:
    {
        auto oldStartIndex = args.OldStartingIndex();
        auto oldCount = static_cast<int>(args.OldItems().Size());
        auto children = m_owner->Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            auto element = children.GetAt(i);
            auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
            auto dataIndex = virtInfo->Index();

            if (virtInfo->IsRealized())
            {
                if (virtInfo->AutoRecycleCandidate() && oldStartIndex <= dataIndex && dataIndex < oldStartIndex + oldCount)
                {
                    // If we are doing the mapping, remove the element who's data was removed.
                    m_owner->ClearElementImpl(element);
                }
                else if (dataIndex >= (oldStartIndex + oldCount))
                {
                    UpdateElementIndex(element, virtInfo, dataIndex - oldCount);
                }
            }
        }

        InvalidateRealizedIndicesHeldByLayout();
        break;
    }

    case winrt::NotifyCollectionChangedAction::Reset:
        if (m_owner->ItemsSourceView().HasKeyIndexMapping())
        {
            m_isDataSourceStableResetPending = true;
        }

        // Walk through all the elements and make sure they are cleared, they will go into
        // the stable id reset pool.
        auto children = m_owner->Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            auto element = children.GetAt(i);
            auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
            if (virtInfo->IsRealized() && virtInfo->AutoRecycleCandidate())
            {
                m_owner->ClearElementImpl(element);
            }
        }

        InvalidateRealizedIndicesHeldByLayout();
        break;
    }
}

void ViewManager::EnsureFirstLastRealizedIndices()
{
    if (m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault)
    {
        // This will ensure that the indexes are updated.
        GetElementIfAlreadyHeldByLayout(0);
    }
}

void ViewManager::OnLayoutChanging()
{
    if (m_owner->ItemsSourceView() &&
        m_owner->ItemsSourceView().HasKeyIndexMapping())
    {
        m_isDataSourceStableResetPending = true;
    }
}

void ViewManager::OnOwnerArranged()
{
    if (m_isDataSourceStableResetPending)
    {
        m_isDataSourceStableResetPending = false;

        for (auto& entry : m_resetPool)
        {
            // TODO: Task 14204306: ItemsRepeater: Find better focus candidate when focused element is deleted in the ItemsSource.
            // Focused element is getting cleared. Need to figure out semantics on where
            // focus should go when the focused element is removed from the data collection.
            ClearElement(entry.second.get(), true /* isClearedDueToCollectionChange */);
        }

        m_resetPool.Clear();
    }
}

#pragma region GetElement providers

// We optimize for the case where index is not realized to return null as quickly as we can.
// Flow layouts manage containers on their own and will never ask for an index that is already realized.
// If an index that is realized is requested by the layout, we unfortunately have to walk the
// children. Not ideal, but a reasonable default to provide consistent behavior between virtualizing
// and non-virtualizing hosts.
winrt::UIElement ViewManager::GetElementIfAlreadyHeldByLayout(int index)
{
    winrt::UIElement element = nullptr;

    const bool cachedFirstLastIndicesInvalid = m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault;
    MUX_ASSERT(!cachedFirstLastIndicesInvalid || m_lastRealizedElementIndexHeldByLayout == LastRealizedElementIndexDefault);

    const bool isRequestedIndexInRealizedRange = (m_firstRealizedElementIndexHeldByLayout <= index && index <= m_lastRealizedElementIndexHeldByLayout);

    if (cachedFirstLastIndicesInvalid || isRequestedIndexInRealizedRange)
    {
        // Both First and Last indices need to be valid or default.
        MUX_ASSERT((m_firstRealizedElementIndexHeldByLayout == FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout == LastRealizedElementIndexDefault) ||
            (m_firstRealizedElementIndexHeldByLayout != FirstRealizedElementIndexDefault && m_lastRealizedElementIndexHeldByLayout != LastRealizedElementIndexDefault));

        auto children = m_owner->Children();
        for (unsigned i = 0u; i < children.Size(); ++i)
        {
            auto child = children.GetAt(i);
            auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(child);
            if (virtInfo && virtInfo->IsHeldByLayout())
            {
                // Only give back elements held by layout. If someone else is holding it, they will be served by other methods.
                const int childIndex = virtInfo->Index();
                m_firstRealizedElementIndexHeldByLayout = std::min(m_firstRealizedElementIndexHeldByLayout, childIndex);
                m_lastRealizedElementIndexHeldByLayout = std::max(m_lastRealizedElementIndexHeldByLayout, childIndex);
                if (virtInfo->Index() == index)
                {
                    element = child;
                    // If we have valid first/last indices, we don't have to walk the rest, but if we 
                    // do not, then we keep walking through the entire children collection to get accurate
                    // indices once.
                    if (!cachedFirstLastIndicesInvalid)
                    {
                        break;
                    }
                }
            }
        }
    }

    return element;
}

winrt::UIElement ViewManager::GetElementFromUniqueIdResetPool(int index)
{
    winrt::UIElement element = nullptr;
    // See if you can get it from the reset pool.
    if (m_isDataSourceStableResetPending)
    {
        element = m_resetPool.Remove(index);
        if (element)
        {
            // Make sure that the index is updated to the current one
            auto virtInfo = ItemsRepeater::GetVirtualizationInfo(element);
            virtInfo->MoveOwnershipToLayoutFromUniqueIdResetPool();
            UpdateElementIndex(element, virtInfo, index);
        }
    }

    return element;
}

winrt::UIElement ViewManager::GetElementFromPinnedElements(int index)
{
    winrt::UIElement element = nullptr;

    // See if you can find something among the pinned elements.
    for (size_t i = 0; i < m_pinnedPool.size(); ++i)
    {
        auto elementInfo = m_pinnedPool[i];
        auto virtInfo = elementInfo.VirtualizationInfo();

        if (virtInfo->Index() == index)
        {
            m_pinnedPool.erase(m_pinnedPool.begin() + i);
            element = elementInfo.PinnedElement();
            elementInfo.VirtualizationInfo()->MoveOwnershipToLayoutFromPinnedPool();
            break;
        }
    }

    return element;
}

winrt::UIElement ViewManager::GetElementFromElementFactory(int index)
{
    // The view generator is the provider of last resort.
    auto data = m_owner->ItemsSourceView().GetAt(index);
    
    auto itemTemplateFactory = m_owner->ItemTemplateShim();

    winrt::UIElement element = nullptr;
    bool itemsSourceContainsElements = false;
    if (!itemTemplateFactory)
    {
        element = data.try_as<winrt::UIElement>();
        // No item template provided and ItemsSource contains objects derived from UIElement.
        // In this case, just use the data directly as elements.
        itemsSourceContainsElements = element != nullptr;
    }

    if (!element)
    {
        if (!itemTemplateFactory)
        {
            // If no ItemTemplate was provided, use a default 
            auto factory = winrt::XamlReader::Load(L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}'/></DataTemplate>").as<winrt::DataTemplate>();
            m_owner->ItemTemplate(factory);
            itemTemplateFactory = m_owner->ItemTemplateShim();
        }

        if (!m_ElementFactoryGetArgs)
        {
            // Create one.
            m_ElementFactoryGetArgs = tracker_ref<winrt::ElementFactoryGetArgs>(m_owner, *winrt::make_self<ElementFactoryGetArgs>());
        }

        auto args = m_ElementFactoryGetArgs.get();
        args.Data(data);
        args.Parent(*m_owner);
        args.as<ElementFactoryGetArgs>()->Index(index);

        element = itemTemplateFactory.GetElement(args);

        args.Data(nullptr);
        args.Parent(nullptr);
    }

    auto virtInfo = ItemsRepeater::TryGetVirtualizationInfo(element);
    if (!virtInfo)
    {
        virtInfo = ItemsRepeater::CreateAndInitializeVirtualizationInfo(element);
        REPEATER_TRACE_PERF(L"ElementCreated");
    }
    else
    {
        // View obtained from ElementFactory already has a VirtualizationInfo attached to it
        // which means that the element has been recycled and not created from scratch.
        REPEATER_TRACE_PERF(L"ElementRecycled");
    }

    if (!itemsSourceContainsElements)
    {
        // Prepare the element
        // If we are phasing, run phase 0 before setting DataContext. If phase 0 is not 
        // run before setting DataContext, when setting DataContext all the phases will be
        // run in the OnDataContextChanged handler in code generated by the xaml compiler (code-gen).
        auto extension = CachedVisualTreeHelpers::GetDataTemplateComponent(element);
        if (extension)
        {
            // Clear out old data. 
            extension.Recycle();
            int nextPhase = VirtualizationInfo::PhaseReachedEnd;
            // Run Phase 0
            extension.ProcessBindings(data, index, 0 /* currentPhase */, nextPhase);

            // Setup phasing information, so that Phaser can pick up any pending phases left.
            // Update phase on virtInfo. Set data and templateComponent only if x:Phase was used.
            virtInfo->UpdatePhasingInfo(nextPhase, nextPhase > 0 ? data : nullptr, nextPhase > 0 ? extension : nullptr);
        }
        else
        {
            // Set data context only if no x:Bind was used. ie. No data template component on the root.
            auto elementAsFE = element.try_as<winrt::FrameworkElement>();
            elementAsFE.DataContext(data);
        }
    }

    virtInfo->MoveOwnershipToLayoutFromElementFactory(
        index,
        /* uniqueId: */
        m_owner->ItemsSourceView().HasKeyIndexMapping() ?
        m_owner->ItemsSourceView().KeyFromIndex(index) :
        winrt::hstring{});

    // The view generator is the only provider that prepares the element.
    auto repeater = m_owner;

    // Add the element to the children collection here before raising OnElementPrepared so 
    // that handlers can walk up the tree in case they want to find their IndexPath in the 
    // nested case.
    auto children = repeater->Children();
    if (CachedVisualTreeHelpers::GetParent(element) != static_cast<winrt::DependencyObject>(*repeater))
    {
        children.Append(element);
    }

    repeater->AnimationManager().OnElementPrepared(element);

    repeater->OnElementPrepared(element, index);

    if (!itemsSourceContainsElements)
    {
        m_phaser.PhaseElement(element, virtInfo);
    }

    // Update realized indices
    m_firstRealizedElementIndexHeldByLayout = std::min(m_firstRealizedElementIndexHeldByLayout, index);
    m_lastRealizedElementIndexHeldByLayout = std::max(m_lastRealizedElementIndexHeldByLayout, index);

    return element;
}

#pragma endregion

#pragma region ClearElement handlers

bool ViewManager::ClearElementToUniqueIdResetPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    if (m_isDataSourceStableResetPending)
    {
        m_resetPool.Add(element);
        virtInfo->MoveOwnershipToUniqueIdResetPoolFromLayout();
    }

    return m_isDataSourceStableResetPending;
}

bool ViewManager::ClearElementToAnimator(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    const bool cleared = m_owner->AnimationManager().ClearElement(element);
    if (cleared)
    {
        const int clearedIndex = virtInfo->Index();
        virtInfo->MoveOwnershipToAnimator();
        if (m_lastFocusedElement == element)
        {
            // Focused element is going away. Remove the tracked last focused element
            // and pick a reasonable next focus if we can find one within the layout 
            // realized elements.
            MoveFocusFromClearedIndex(clearedIndex);
        }

    }
    return cleared;
}

bool ViewManager::ClearElementToPinnedPool(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, bool isClearedDueToCollectionChange)
{
    bool moveToPinnedPool =
        !isClearedDueToCollectionChange && virtInfo->IsPinned();

    if (moveToPinnedPool)
    {
#ifdef _DEBUG
        for (size_t i = 0; i < m_pinnedPool.size(); ++i)
        {
            MUX_ASSERT(m_pinnedPool[i].PinnedElement() != element);
        }
#endif
        m_pinnedPool.push_back(PinnedElementInfo(m_owner, element));
        virtInfo->MoveOwnershipToPinnedPool();
    }

    return moveToPinnedPool;
}

#pragma endregion

void ViewManager::UpdateFocusedElement()
{
    winrt::UIElement focusedElement = nullptr;

    auto child = winrt::FocusManager::GetFocusedElement().as<winrt::DependencyObject>();

    if (child)
    {
        auto parent = CachedVisualTreeHelpers::GetParent(child);
        auto owner = static_cast<winrt::UIElement>(*m_owner);

        // Find out if the focused element belongs to one of our direct
        // children.
        while (parent)
        {
            auto repeater = parent.try_as<winrt::ItemsRepeater>();
            if (repeater)
            {
                auto element = child.as<winrt::UIElement>();
                if (repeater == owner && ItemsRepeater::GetVirtualizationInfo(element)->IsRealized())
                {
                    focusedElement = element;
                }

                break;
            }

            child = parent;
            parent = CachedVisualTreeHelpers::GetParent(child);
        }
    }

    // If the focused element has changed,
    // we need to unpin the old one and pin the new one.
    if (m_lastFocusedElement != focusedElement)
    {
        if (m_lastFocusedElement)
        {
            UpdatePin(m_lastFocusedElement.get(), false /* addPin */);
        }

        if (focusedElement)
        {
            UpdatePin(focusedElement, true /* addPin */);
        }

        m_lastFocusedElement.set(focusedElement);
    }
}

void ViewManager::OnFocusChanged(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    UpdateFocusedElement();
}

void ViewManager::EnsureEventSubscriptions()
{
    if (!m_gotFocus)
    {
        MUX_ASSERT(!m_lostFocus);
        m_gotFocus = m_owner->GotFocus(winrt::auto_revoke, { this, &ViewManager::OnFocusChanged });
        m_lostFocus = m_owner->LostFocus(winrt::auto_revoke, { this, &ViewManager::OnFocusChanged });
    }
}

void ViewManager::UpdateElementIndex(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo, int index)
{
    auto oldIndex = virtInfo->Index();
    if (oldIndex != index)
    {
        virtInfo->UpdateIndex(index);
        m_owner->OnElementIndexChanged(element, oldIndex, index);
    }
}

void ViewManager::InvalidateRealizedIndicesHeldByLayout()
{
    m_firstRealizedElementIndexHeldByLayout = FirstRealizedElementIndexDefault;
    m_lastRealizedElementIndexHeldByLayout = LastRealizedElementIndexDefault;
}

ViewManager::PinnedElementInfo::PinnedElementInfo(const ITrackerHandleManager* owner, const winrt::UIElement& element) :
    m_pinnedElement(owner, element),
    m_virtInfo(owner, ItemsRepeater::GetVirtualizationInfo(element))
{ }
