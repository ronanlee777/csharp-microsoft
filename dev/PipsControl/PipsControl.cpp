﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "PipsControl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "PipsControlTemplateSettings.h"
#include "PipsControlSelectedIndexChangedEventArgs.h"
#include "PipsControlAutomationPeer.h"

typedef winrt::IndicatorPagerButtonVisibility ButtonVisibility;

constexpr auto c_previousPageButtonVisibleVisualState = L"PreviousPageButtonVisible"sv;
constexpr auto c_previousPageButtonHiddenVisualState = L"PreviousPageButtonHidden"sv;
constexpr auto c_previousPageButtonCollapsedVisualState = L"PreviousPageButtonCollapsed"sv;

constexpr auto c_previousPageButtonEnabledVisualState = L"PreviousPageButtonEnabled"sv;
constexpr auto c_previousPageButtonDisabledVisualState = L"PreviousPageButtonDisabled"sv;

constexpr auto c_nextPageButtonVisibleVisualState = L"NextPageButtonVisible"sv;
constexpr auto c_nextPageButtonHiddenVisualState = L"NextPageButtonHidden"sv;
constexpr auto c_nextPageButtonCollapsedVisualState = L"NextPageButtonCollapsed"sv;

constexpr auto c_nextPageButtonEnabledVisualState = L"NextPageButtonEnabled"sv;
constexpr auto c_nextPageButtonDisabledVisualState = L"NextPageButtonDisabled"sv;

constexpr auto c_infiniteItemsModeState = L"InfiniteItems"sv;

constexpr auto c_rootGridName = L"RootGrid"sv;

constexpr auto c_previousPageButtonName = L"PreviousPageButton"sv;
constexpr auto c_nextPageButtonName = L"NextPageButton"sv;

constexpr auto c_PipsControlPipsRepeaterName = L"VerticalPipsItemsRepeater"sv;
constexpr auto c_PipsControlScrollViewerName = L"VerticalPipsScrollViewer"sv;

constexpr auto c_PipsControlButtonHeightPropertyName = L"PipsControlVerticalPipButtonHeight"sv;

PipsControl::PipsControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PipsControl);

    m_verticalPipsElements = winrt::make<Vector<IInspectable>>().as<winrt::IObservableVector<IInspectable>>();
    const auto templateSettings = winrt::make<PipsControlTemplateSettings>();
    templateSettings.SetValue(PipsControlTemplateSettings::s_VerticalPipsItemsProperty, m_verticalPipsElements);
    SetValue(s_TemplateSettingsProperty, templateSettings);

    SetDefaultStyleKey(this);

}

PipsControl::~PipsControl() {
    m_nextPageButtonClickRevoker.revoke();
    m_previousPageButtonClickRevoker.revoke();
    m_verticalPipsElementPreparedRevoker.revoke();
    m_rootGridPointerEnteredRevoker.revoke();
    m_rootGridPointerExitedRevoker.revoke();
    m_rootGridKeyDownRevoker.revoke();
}

void PipsControl::OnApplyTemplate()
{
    winrt::AutomationProperties::SetName(*this, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlNameText));

    m_previousPageButtonClickRevoker.revoke();
    [this](const winrt::Button button) {
        if (button) {
            winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlPreviousPageButtonText));
            m_previousPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsControl::OnPreviousButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_previousPageButtonName, *this));

    m_nextPageButtonClickRevoker.revoke();
    [this](const winrt::Button button) {
        if (button) {
            winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlNextPageButtonText));
            m_nextPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsControl::OnNextButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_nextPageButtonName, *this));

    m_verticalPipsElementPreparedRevoker.revoke();
    [this](const winrt::ItemsRepeater repeater) {
        m_verticalPipsRepeater.set(repeater);
        if (repeater) {
            m_verticalPipsElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { this, &PipsControl::OnElementPrepared });
        }
    }(GetTemplateChildT<winrt::ItemsRepeater>(c_PipsControlPipsRepeaterName, *this));


    m_verticalPipsScrollViewer.set(GetTemplateChildT<winrt::FxScrollViewer>(c_PipsControlScrollViewerName, *this));

    m_rootGridPointerEnteredRevoker.revoke();
    m_rootGridPointerExitedRevoker.revoke();

    [this](const winrt::Grid grid) {
        if (grid) {
            m_rootGridPointerEnteredRevoker = grid.PointerEntered(winrt::auto_revoke, { this, &PipsControl::OnPipsControlPointerEntered });
            m_rootGridPointerExitedRevoker = grid.PointerExited(winrt::auto_revoke, { this, &PipsControl::OnPipsControlPointerExited });
            m_rootGridKeyDownRevoker = grid.KeyDown(winrt::auto_revoke, { this, &PipsControl::OnRootGridKeyDown });

        }
    }(GetTemplateChildT<winrt::Grid>(c_rootGridName, *this));

    OnNumberOfPagesChanged(0);
    OnSelectedPageIndexChange(-1);
    OnMaxDisplayedPagesChanged(0);
}

void PipsControl::OnPipsControlPointerEntered(winrt::IInspectable sender, winrt::PointerRoutedEventArgs args) {
    m_isPointerOver = true;
    UpdateNavigationButtonVisualStates();
}
void PipsControl::OnPipsControlPointerExited(winrt::IInspectable sender, winrt::PointerRoutedEventArgs args) {
    // TODO : Explain WHAT'S UP and mention quarks
    if (!IsWithinBounds(args.GetCurrentPoint(*this).Position())) {
        m_isPointerOver = false;
        UpdateNavigationButtonVisualStates();
        args.Handled(true);
    }

}

bool PipsControl::IsWithinBounds(winrt::Point point) {
    return point.X >= 0 && point.X <= ActualWidth() && point.Y >= 0 && point.Y <= ActualHeight();
}

void PipsControl::UpdateIndividualNavigationButtonVisualState(
    bool hiddenOnEdgeCondition,
    ButtonVisibility visibility,
    const wstring_view visibleStateName,
    const wstring_view hiddenStateName,
    const wstring_view enabledStateName,
    const wstring_view disabledStateName) {

    if (visibility != ButtonVisibility::Collapsed) {
        if ((visibility == ButtonVisibility::Visible || m_isPointerOver) && !hiddenOnEdgeCondition) {
            winrt::VisualStateManager::GoToState(*this, visibleStateName, false);
            winrt::VisualStateManager::GoToState(*this, enabledStateName, false);
        }
        else {
            if (hiddenOnEdgeCondition) {
                winrt::VisualStateManager::GoToState(*this, disabledStateName, false);
            }
            winrt::VisualStateManager::GoToState(*this, hiddenStateName, false);
        }
    }
}

void PipsControl::OnNavigationButtonVisibilityChanged(ButtonVisibility visibility, const wstring_view collapsedStateName) {
    if (visibility == ButtonVisibility::Collapsed) {
        winrt::VisualStateManager::GoToState(*this, collapsedStateName, false);
    }
    else {
        UpdateNavigationButtonVisualStates();
    }
}

void PipsControl::UpdateNavigationButtonVisualStates() {

    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();
    const int maxDisplayedPages = MaxDisplayedPages();
    
    if (!numberOfPages == 0 && maxDisplayedPages > 0) { 
        auto const ifPreviousButtonHiddenOnEdge = selectedPageIndex == 0;
        UpdateIndividualNavigationButtonVisualState(ifPreviousButtonHiddenOnEdge, PreviousButtonVisibility(),
                c_previousPageButtonVisibleVisualState, c_previousPageButtonHiddenVisualState,
                c_previousPageButtonEnabledVisualState, c_previousPageButtonDisabledVisualState);

        auto const ifNextButtonHiddenOnEdge = selectedPageIndex == numberOfPages - 1;
        UpdateIndividualNavigationButtonVisualState(ifNextButtonHiddenOnEdge, NextButtonVisibility(),
            c_nextPageButtonVisibleVisualState, c_nextPageButtonHiddenVisualState,
            c_nextPageButtonEnabledVisualState, c_nextPageButtonDisabledVisualState);
    }
}

void PipsControl::OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (const auto pip = args.Element().try_as<winrt::Button>()) {
        auto const pageNumber = unbox_value<int>(pip.Tag());
        if (unbox_value<int>(pip.Tag()) - 1 != SelectedPageIndex()) {
            pip.Style(DefaultIndicatorStyle());
        }
        // Narrator says: Page 5, Button 5 of 30. Is it expected behavior?
        winrt::AutomationProperties::SetName(pip, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlPageText) + L" " + winrt::to_hstring(pageNumber));
        winrt::AutomationProperties::SetPositionInSet(pip, pageNumber);
        winrt::AutomationProperties::SetSizeOfSet(pip, NumberOfPages());
        const auto buttonClickedFunc = [this](auto const& sender, auto const& args) {
            if (const auto button = sender.try_as<winrt::Button>())
            {
                SelectedPageIndex(winrt::unbox_value<int>(button.Tag()) - 1);
            }
        };
        pip.Click(buttonClickedFunc);

    }
}

void PipsControl::ScrollToCenterOfViewport(winrt::UIElement sender)
{
    winrt::BringIntoViewOptions options;
    options.VerticalAlignmentRatio(0.5);
    options.AnimationDesired(true);
    sender.StartBringIntoView(options);
}


void PipsControl::OnMaxDisplayedPagesChanged(const int oldValue) {
    m_lastMaxDisplayedPages = oldValue;
    auto const numberOfPages = NumberOfPages();
    if (m_lastMaxDisplayedPages != numberOfPages) {
        UpdateVerticalPips(numberOfPages, MaxDisplayedPages());
    }
}

void PipsControl::OnNumberOfPagesChanged(const int oldValue)
{
    m_lastNumberOfPagesCount = oldValue;
    const int numberOfPages = NumberOfPages();
    if (SelectedPageIndex() > numberOfPages - 1 && numberOfPages > -1) {
        SelectedPageIndex(numberOfPages - 1);
    }
    UpdateVerticalPips(numberOfPages, MaxDisplayedPages());
    UpdateNavigationButtonVisualStates();
}

void PipsControl::OnSelectedPageIndexChange(const int oldValue)
{
    // If we don't have any pages, there is nothing we should do.
    // Ensure that SelectedPageIndex will end up in the valid range of values
    // Special case is NumberOfPages being 0, in that case, don't verify upperbound restrictions
    if (SelectedPageIndex() > NumberOfPages() - 1 && NumberOfPages() > 0)
    {
        SelectedPageIndex(NumberOfPages() - 1);
    }
    else if (SelectedPageIndex() < 0)
    {
        SelectedPageIndex(0);
    }
    else {
        // Now handle the value changes
        m_lastSelectedPageIndex = oldValue;

        // Fire value property change for UIA
        if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::PipsControlAutomationPeer>())
        {
            winrt::get_self<PipsControlAutomationPeer>(peer)->RaiseSelectionChanged(m_lastSelectedPageIndex, SelectedPageIndex());
        }
        UpdateNavigationButtonVisualStates();
        UpdateVerticalPips(NumberOfPages(), SelectedPageIndex());
        RaiseSelectedIndexChanged();
    }
}

void PipsControl::RaiseSelectedIndexChanged()
{
    const auto args = winrt::make_self<PipsControlSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}

void PipsControl::MovePipIdentifierToElement(int index) {

    if (NumberOfPages() != 0) {
        if (const auto repeater = m_verticalPipsRepeater.get())
        {
            if (const auto element = repeater.TryGetElement(m_lastSelectedPageIndex).try_as<winrt::Button>()) {
                element.Style(DefaultIndicatorStyle());
            }
            if (const auto element = repeater.GetOrCreateElement(index).try_as<winrt::Button>()) {
                element.UpdateLayout();
                element.Style(SelectedIndicatorStyle());
                ScrollToCenterOfViewport(element);
            }
        }
    }
}

void PipsControl::SetVerticalPipsSVMaxSize() {
    auto pipHeight = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_PipsControlButtonHeightPropertyName)));
    auto numberOfPages = NumberOfPages() < 0 ? MaxDisplayedPages() : std::min(NumberOfPages(), MaxDisplayedPages());
    auto scrollViewerHeight = pipHeight * numberOfPages;
    m_verticalPipsScrollViewer.get().MaxHeight(scrollViewerHeight);
}



void PipsControl::UpdateVerticalPips(const int numberOfPages, const int maxDisplayedPages) {

    auto pipsListSize = static_cast<int>(m_verticalPipsElements.Size());
    auto const selectedIndex = SelectedPageIndex();

    if (numberOfPages < pipsListSize) {
        m_verticalPipsElements.Clear();
        pipsListSize = 0;
    }

    auto const endIndex = std::min(numberOfPages, selectedIndex + maxDisplayedPages);
    for (int i = pipsListSize; i < endIndex; i++) {
        m_verticalPipsElements.Append(box_value(i + 1));
    }

    if (maxDisplayedPages != m_lastMaxDisplayedPages) {
        SetVerticalPipsSVMaxSize();
    }
    MovePipIdentifierToElement(SelectedPageIndex());

}

void PipsControl::OnRootGridKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args) {
    if (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::Up)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Up);
    }
    else if (args.Key() == winrt::VirtualKey::Right || args.Key() == winrt::VirtualKey::Down)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Down);
    }
}


void PipsControl::OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always greater than 1.
       SelectedPageIndex(SelectedPageIndex() - 1);
}

void PipsControl::OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always less than maximum.
    SelectedPageIndex(SelectedPageIndex() + 1);
}

void PipsControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (this->Template() != nullptr)
    {
        if (property == NumberOfPagesProperty())
        {
            OnNumberOfPagesChanged(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == SelectedPageIndexProperty())
        {
            OnSelectedPageIndexChange(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == MaxDisplayedPagesProperty()) {
            OnMaxDisplayedPagesChanged(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == PreviousButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(PreviousButtonVisibility(), c_previousPageButtonCollapsedVisualState);
        }
        else if (property == NextButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(NextButtonVisibility(), c_nextPageButtonCollapsedVisualState);
        }
        // TODO: Add button styles listeners
    }
}

winrt::AutomationPeer PipsControl::OnCreateAutomationPeer()
{
    return winrt::make<PipsControlAutomationPeer>(*this);
}
