﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "ItemTemplateWrapper.h"
#include "Breadcrumb.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithBasicFactory(BreadcrumbItem)
}

#include "BreadcrumbItem.g.cpp"

BreadcrumbItem::BreadcrumbItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_BreadcrumbItem);

    SetDefaultStyleKey(this);
}

BreadcrumbItem::~BreadcrumbItem()
{
    RevokeListeners();
}

void BreadcrumbItem::RevokeListeners()
{
    m_breadcrumbItemButtonLoadedRevoker.revoke();
    m_breadcrumbItemButtonClickRevoker.revoke();
    m_ellipsisRepeaterElementPreparedRevoker.revoke();
}

void BreadcrumbItem::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    RevokeListeners();

    winrt::IControlProtected controlProtected{ *this };

    m_breadcrumbItemButton.set(GetTemplateChildT<winrt::Button>(L"PART_BreadcrumbItemButton", controlProtected));

    RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &BreadcrumbItem::OnFlowDirectionChanged });

    if (auto const& thisAsIUIElement7 = this->try_as<winrt::IUIElement7>())
    {
        thisAsIUIElement7.PreviewKeyDown({ this, &BreadcrumbItem::OnChildPreviewKeyDown });
    }

    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        m_breadcrumbItemButtonLoadedRevoker = breadcrumbItemButton.Loaded(winrt::auto_revoke, { this, &BreadcrumbItem::OnLoadedEvent });

        m_pressedButtonRevoker = RegisterPropertyChanged(breadcrumbItemButton, winrt::ButtonBase::IsPressedProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });
        m_pointerOverButtonRevoker = RegisterPropertyChanged(breadcrumbItemButton, winrt::ButtonBase::IsPointerOverProperty(), { this, &BreadcrumbItem::OnVisualPropertyChanged });

        // Register for pointer events so we can keep track of the last used pointer type
        m_breadcrumbItemPointerEnteredRevoker = breadcrumbItemButton.PointerEntered(winrt::auto_revoke, { this, &BreadcrumbItem::OnPointerEvent });
        m_breadcrumbItemPointerExitedRevoker = breadcrumbItemButton.PointerExited(winrt::auto_revoke, { this, &BreadcrumbItem::OnPointerEvent });
        m_breadcrumbItemPointerPressedRevoker = breadcrumbItemButton.PointerPressed(winrt::auto_revoke, { this, &BreadcrumbItem::OnPointerEvent });
        m_breadcrumbItemPointerReleasedRevoker = breadcrumbItemButton.PointerReleased(winrt::auto_revoke, { this, &BreadcrumbItem::OnPointerEvent });
        m_breadcrumbItemPointerCanceledRevoker = breadcrumbItemButton.PointerCanceled(winrt::auto_revoke, { this, &BreadcrumbItem::OnPointerEvent });
        m_breadcrumbItemPointerCaptureLostRevoker = breadcrumbItemButton.PointerCaptureLost(winrt::auto_revoke, { this, &BreadcrumbItem::OnPointerEvent });
    }
}

void BreadcrumbItem::OnLoadedEvent(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        m_breadcrumbItemButtonClickRevoker.revoke();
        if (m_isEllipsisNode)
        {
            m_breadcrumbItemButtonClickRevoker = breadcrumbItemButton.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnEllipsisItemClick });
        }
        else
        {
            m_breadcrumbItemButtonClickRevoker = breadcrumbItemButton.Click(winrt::auto_revoke, { this, &BreadcrumbItem::OnBreadcrumbItemClick });
        }
    }

    if (m_isEllipsisNode)
    {
        SetPropertiesForEllipsisNode();
    }
    else if (m_isLastNode)
    {
        SetPropertiesForLastNode();
    }
    else
    {
        ResetVisualProperties();
    }

    UpdateCommonVisualState();
}

void BreadcrumbItem::SetParentBreadcrumb(const winrt::Breadcrumb& parent)
{
    m_parentBreadcrumb.set(parent);
}

void BreadcrumbItem::SetFlyoutDataTemplate(const winrt::IInspectable& newDataTemplate)
{
    if (auto const& dataTemplate = newDataTemplate.try_as<winrt::DataTemplate>())
    {
        m_ellipsisDataTemplate.set(dataTemplate);
    }
    else if (!newDataTemplate)
    {
        m_ellipsisDataTemplate.set(nullptr);
    }
}

void BreadcrumbItem::OnBreadcrumbItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        auto breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        breadcrumbImpl->RaiseItemClickedEvent(Content());
    }
}

void BreadcrumbItem::OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args)
{
    UpdateCommonVisualState();
}

void BreadcrumbItem::OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    const auto& element = args.Element();

    element.AddHandler(winrt::UIElement::PointerPressedEvent(),
        winrt::box_value<winrt::PointerEventHandler>({ this, &BreadcrumbItem::OnFlyoutElementClickEvent }),
        true);

    // TODO: Investigate an RS2 or lower way to invoke via keyboard. Github issue #3997
    if (SharedHelpers::IsRS3OrHigher())
    {
        element.PreviewKeyDown({ this, &BreadcrumbItem::OnFlyoutElementKeyDownEvent });
    }
    else
    {
        // Prior to RS3, UIElement.PreviewKeyDown is not available.
        AddRoutedEventHandler<RoutedEventType::KeyDown>(element.try_as<winrt::UIElement>(), 
            [this](auto const& sender, auto const& args)
            {
                OnFlyoutElementKeyDownEvent(sender, nullptr);
            },
            true /*handledEventsToo*/);
    }
}

void BreadcrumbItem::OnFlyoutElementKeyDownEvent(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Enter)
    {
        this->OnFlyoutElementClickEvent(sender, nullptr);
        args.Handled(true);
    }
    else
    {
        args.Handled(false);
    }
}

void BreadcrumbItem::OnFlyoutElementClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        const auto& breadcrumbImpl = winrt::get_self<Breadcrumb>(breadcrumb);
        const auto& senderAsContentControl = sender.try_as<winrt::ContentControl>();

        // Once an element has been clicked, close the flyout
        CloseFlyout();

        breadcrumbImpl->RaiseItemClickedEvent(senderAsContentControl.Content());
    }
}

void BreadcrumbItem::OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&)
{
    UpdateItemTypeVisualState();
}

void BreadcrumbItem::OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Enter)
    {
        if (m_isEllipsisNode)
        {
            OnEllipsisItemClick(nullptr, nullptr);
        }
        else
        {
            OnBreadcrumbItemClick(nullptr, nullptr);
        }
        args.Handled(true);
    }
}

winrt::IInspectable BreadcrumbItem::CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource)
{
    // A copy of the hidden elements array in BreadcrumbLayout is created
    // to avoid getting a Layout cycle exception
    auto newItemsSource = winrt::make<Vector<winrt::IInspectable>>();

    // The new list contains all the elements in reverse order
    const int itemsSourceSize = ellipsisItemsSource.Size();

    // The itemsSourceSize should always be at least 1 as it must always contain the ellipsis item
    assert(itemsSourceSize > 0);

    for (int i = itemsSourceSize - 1; i >= 0; --i)
    {
        const auto& item = ellipsisItemsSource.GetAt(i);
        newItemsSource.Append(item);
    }

    return newItemsSource;
}

void BreadcrumbItem::OpenFlyout()
{
    if (auto flyout = m_ellipsisFlyout.get())
    {
        if (SharedHelpers::IsFlyoutShowOptionsAvailable())
        {
            winrt::FlyoutShowOptions options{};
            flyout.ShowAt(*this, options);
        }
        else
        {
            flyout.ShowAt(*this);
        }
    }
}

void BreadcrumbItem::CloseFlyout()
{
    if (auto flyout = m_ellipsisFlyout.get())
    {
        flyout.Hide();
    }
}

void BreadcrumbItem::OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateCommonVisualState();
}

void BreadcrumbItem::UpdateItemTypeVisualState()
{
    const bool isLeftToRight = (FlowDirection() == winrt::FlowDirection::LeftToRight);
    hstring visualStateName;

    if (m_isEllipsisNode)
    {
        if (isLeftToRight)
        {
            visualStateName = L"Ellipsis";
        }
        else
        {
            visualStateName = L"EllipsisRTL";
        }
    }
    else if (m_isLastNode)
    {
        visualStateName = L"LastItem";
    }
    else
    {
        if (isLeftToRight)
        {
            visualStateName = L"Default";
        }
        else
        {
            visualStateName = L"DefaultRTL";
        }
    }

    winrt::VisualStateManager::GoToState(*this, visualStateName, false);
}

void BreadcrumbItem::UpdateCommonVisualState()
{
    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        hstring commonVisualStateName = L"";

        // If is last item: place Current as prefix for visual state
        if (m_isLastNode)
        {
            commonVisualStateName = L"Current";
        }

        if (!breadcrumbItemButton.IsEnabled())
        {
            commonVisualStateName = commonVisualStateName + L"Disabled";
        }
        else if (breadcrumbItemButton.IsPressed())
        {
            commonVisualStateName = commonVisualStateName + L"Pressed";
        }
        else if (breadcrumbItemButton.IsPointerOver())
        {
            commonVisualStateName = commonVisualStateName + L"PointerOver";
        }
        else
        {
            commonVisualStateName = commonVisualStateName + L"Normal";
        }

        winrt::VisualStateManager::GoToState(breadcrumbItemButton, commonVisualStateName, false);
    }
}

void BreadcrumbItem::OnEllipsisItemClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (const auto& breadcrumb = m_parentBreadcrumb.get())
    {
        if (const auto& breadcrumbImpl = breadcrumb.try_as<Breadcrumb>())
        {
            const auto& hiddenElements = CloneEllipsisItemSource(breadcrumbImpl->HiddenElements());

            if (const auto& flyoutRepeater = m_ellipsisItemsRepeater.get())
            {
                flyoutRepeater.ItemTemplate(m_ellipsisDataTemplate.get());
                flyoutRepeater.ItemsSource(hiddenElements);
            }

            OpenFlyout();
        }
    }
}

void BreadcrumbItem::SetPropertiesForLastNode()
{
    m_isEllipsisNode = false;
    m_isLastNode = true;

    UpdateItemTypeVisualState();
}

void BreadcrumbItem::ResetVisualProperties()
{
    m_isEllipsisNode = false;
    m_isLastNode = false;

    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        breadcrumbItemButton.Flyout(nullptr);
    }
    m_ellipsisFlyout.set(nullptr);
    m_ellipsisItemsRepeater.set(nullptr);

    UpdateItemTypeVisualState();
}

void BreadcrumbItem::InstantiateFlyout()
{
    // Only if the element has been created visually, instantiate the flyout
    if (const auto& breadcrumbItemButton = m_breadcrumbItemButton.get())
    {
        // Create ItemsRepeater and set the DataTemplate 
        const auto& ellipsisItemsRepeater = winrt::ItemsRepeater();
        ellipsisItemsRepeater.HorizontalAlignment(winrt::HorizontalAlignment::Stretch);

        if (const auto& dataTemplate = m_ellipsisDataTemplate.get())
        {
            ellipsisItemsRepeater.ItemTemplate(dataTemplate);
        }

        m_ellipsisRepeaterElementPreparedRevoker = ellipsisItemsRepeater.ElementPrepared(winrt::auto_revoke, { this, &BreadcrumbItem::OnFlyoutElementPreparedEvent });
        
        m_ellipsisItemsRepeater.set(ellipsisItemsRepeater);

        // Create the Flyout and add the ItemsRepeater as content
        const auto& ellipsisFlyout = winrt::Flyout();
        ellipsisFlyout.Content(ellipsisItemsRepeater);
        ellipsisFlyout.Placement(winrt::FlyoutPlacementMode::Bottom);

        m_ellipsisFlyout.set(ellipsisFlyout);

        // Set the Flyout to the ellipsis button
        breadcrumbItemButton.Flyout(ellipsisFlyout);
    }
}

void BreadcrumbItem::SetPropertiesForEllipsisNode()
{
    m_isEllipsisNode = true;
    m_isLastNode = false;

    InstantiateFlyout();

    UpdateItemTypeVisualState();
}
