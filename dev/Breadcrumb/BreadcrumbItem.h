﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "BreadcrumbDropDownElementFactory.h"

#include "BreadcrumbItem.g.h"

class BreadcrumbItem :
    public ReferenceTracker<BreadcrumbItem, winrt::implementation::BreadcrumbItemT>
{
public:
    BreadcrumbItem();
    ~BreadcrumbItem();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElement
    void OnApplyTemplate();
    void RevokeListeners();

    // internal
    void ResetVisualProperties();
    void SetPropertiesForLastNode();
    void SetPropertiesForEllipsisNode();
    void SetParentBreadcrumb(const winrt::Breadcrumb& parent);
    void SetDropDownItemDataTemplate(const winrt::IInspectable& newDataTemplate);
    void RaiseItemClickedEvent(const winrt::IInspectable& content);
    void CloseFlyout();
    void OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

private:
    void OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnEllipsisItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnBreadcrumbItemClick(const winrt::IInspectable& sender, const winrt::RoutedEventArgs & args);
    void OnFlyoutElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void OnFlyoutElementKeyDownEvent(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs&);
    void OnFlyoutElementClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);

    void InstantiateFlyout();
    void OpenFlyout();

    void UpdateItemTypeVisualState();
    void UpdateCommonVisualState();

    winrt::IInspectable CloneEllipsisItemSource(const winrt::Collections::IVector<winrt::IInspectable>& ellipsisItemsSource);

    bool m_isEllipsisNode{};
    bool m_isLastNode{};

    // BreadcrumbItem visual representation
    tracker_ref<winrt::Button> m_breadcrumbItemButton{ this };

    // Flyout content for ellipsis item
    tracker_ref<winrt::FlyoutBase> m_ellipsisFlyout{ this };
    tracker_ref<winrt::ItemsRepeater> m_ellipsisItemsRepeater{ this };
    tracker_ref<winrt::DataTemplate> m_dropDownItemDataTemplate{ this };
    com_ptr<BreadcrumbDropDownElementFactory> m_ellipsisElementFactory{ nullptr };

    // Parent BreadcrumbItem to ask for hidden elements
    tracker_ref<winrt::Breadcrumb> m_parentBreadcrumb{ this };

    winrt::Button::Loaded_revoker m_breadcrumbItemButtonLoadedRevoker{};
    winrt::Button::Click_revoker m_breadcrumbItemButtonClickRevoker{};
    RoutedEventHandler_revoker m_breadcrumbItemKeyDownHandlerRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_ellipsisRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_ellipsisRepeaterElementClearingRevoker{};

    PropertyChanged_revoker m_pressedButtonRevoker{};
    PropertyChanged_revoker m_pointerOverButtonRevoker{};

    // Revokers for events that change visual state changes
    winrt::UIElement::PointerEntered_revoker m_breadcrumbItemPointerEnteredRevoker{};
    winrt::UIElement::PointerExited_revoker m_breadcrumbItemPointerExitedRevoker{};
    winrt::UIElement::PointerPressed_revoker m_breadcrumbItemPointerPressedRevoker{};
    winrt::UIElement::PointerReleased_revoker m_breadcrumbItemPointerReleasedRevoker{};
    winrt::UIElement::PointerCanceled_revoker m_breadcrumbItemPointerCanceledRevoker{};
    winrt::UIElement::PointerCaptureLost_revoker m_breadcrumbItemPointerCaptureLostRevoker{};
};
