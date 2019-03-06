﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemPresenter.h"
#include "NavigationViewItemPresenterTemplateSettings.h"
#include "NavigationViewItem.h"
#include "SharedHelpers.h"

static constexpr int s_indentation = 16;

NavigationViewItemPresenter::NavigationViewItemPresenter()
{
    SetValue(s_TemplateSettingsProperty, winrt::make<NavigationViewItemPresenterTemplateSettings>());
    SetDefaultStyleKey(this);
}

void NavigationViewItemPresenter::SetDepth(int depth)
{
    m_depth = depth;
    UpdateIndentations();
}

void NavigationViewItemPresenter::OnApplyTemplate()
{
    // Retrieve pointers to stable controls 
    m_helper.Init(*this);
    if (auto navigationViewItem = GetNavigationViewItem())
    {
        navigationViewItem->UpdateVisualStateNoTransition();
    }
    UpdateIndentations();
}

winrt::UIElement NavigationViewItemPresenter::GetSelectionIndicator()
{
    return m_helper.GetSelectionIndicator();  
}

bool NavigationViewItemPresenter::GoToElementStateCore(winrt::hstring const& state, bool useTransitions)
{
    // GoToElementStateCore: Update visualstate for itself.
    // VisualStateManager::GoToState: update visualstate for it's first child.

    // If NavigationViewItemPresenter is used, two sets of VisualStateGroups are supported. One set is help to switch the style and it's NavigationViewItemPresenter itself and defined in NavigationViewItem
    // Another set is defined in style for NavigationViewItemPresenter.
    // OnLeftNavigation, OnTopNavigationPrimary, OnTopNavigationOverflow only apply to itself.
    if (state == c_OnLeftNavigation || state == c_OnLeftNavigationReveal || state == c_OnTopNavigationPrimary
        || state == c_OnTopNavigationPrimaryReveal || state == c_OnTopNavigationOverflow)
    {
        return __super::GoToElementStateCore(state, useTransitions);
    }
    return winrt::VisualStateManager::GoToState(*this, state, useTransitions);
}

NavigationViewItem* NavigationViewItemPresenter::GetNavigationViewItem()
{
    NavigationViewItem* navigationViewItem = nullptr;

    winrt::DependencyObject obj = operator winrt::DependencyObject();

    if (auto item = SharedHelpers::GetAncestorOfType<winrt::NavigationViewItem>(winrt::VisualTreeHelper::GetParent(obj)))
    {
        navigationViewItem = winrt::get_self<NavigationViewItem>(item);
    }
    return navigationViewItem;
}

void NavigationViewItemPresenter::UpdateIndentations()
{
    auto leftIndentation = s_indentation * m_depth;
    auto thickness = winrt::ThicknessHelper::FromLengths(leftIndentation, 0, 0, 0);
    winrt::get_self<NavigationViewItemPresenterTemplateSettings>(TemplateSettings())->Indentation(thickness);
}
