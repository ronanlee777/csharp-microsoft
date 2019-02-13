﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "Scroller.h"
#include "RuntimeProfiler.h"
#include "FocusHelper.h"
#include "ScrollViewerTestHooks.h"

// Change to 'true' to turn on debugging outputs in Output window
bool ScrollViewerTrace::s_IsDebugOutputEnabled{ false };
bool ScrollViewerTrace::s_IsVerboseDebugOutputEnabled{ false };

ScrollViewer::ScrollViewer()
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    EnsureProperties();
    SetDefaultStyleKey(this);
    HookScrollViewerEvents();
}

ScrollViewer::~ScrollViewer()
{
    SCROLLVIEWER_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollerEvents(true /*isForDestructor*/);
    UnhookScrollViewerEvents();
    StopHideIndicatorsTimer(true /*isForDestructor*/);
}

#pragma region IScrollViewer

winrt::CompositionPropertySet ScrollViewer::ExpressionAnimationSources()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ExpressionAnimationSources();
    }

    return nullptr;
}

double ScrollViewer::HorizontalOffset()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.HorizontalOffset();
    }

    return 0.0;
}

double ScrollViewer::VerticalOffset()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.VerticalOffset();
    }

    return 0.0;
}

float ScrollViewer::ZoomFactor()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ZoomFactor();
    }

    return 0.0f;
}

double ScrollViewer::ExtentWidth()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ExtentWidth();
    }

    return 0.0;
}

double ScrollViewer::ExtentHeight()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ExtentHeight();
    }

    return 0.0;
}

double ScrollViewer::ViewportWidth()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ViewportWidth();
    }

    return 0.0;
}

double ScrollViewer::ViewportHeight()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ViewportHeight();
    }

    return 0.0;
}

double ScrollViewer::ScrollableWidth()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ScrollableWidth();
    }

    return 0.0;
}

double ScrollViewer::ScrollableHeight()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.ScrollableHeight();
    }

    return 0.0;
}

winrt::InteractionState ScrollViewer::State()
{
    if (auto scroller = m_scroller.get())
    {
        return scroller.State();
    }

    return winrt::InteractionState::Idle;
}

winrt::InputKind ScrollViewer::IgnoredInputKind()
{
    // Workaround for Bug 17377013: XamlCompiler codegen for Enum CreateFromString always returns boxed int which is wrong for [flags] enums (should be uint)
    // Check if the boxed IgnoredInputKind is an IReference<int> first in which case we unbox as int.
    auto boxedKind = GetValue(s_IgnoredInputKindProperty);
    if (auto boxedInt = boxedKind.try_as<winrt::IReference<int32_t>>())
    {
        return winrt::InputKind{ static_cast<uint32_t>(unbox_value<int32_t>(boxedInt)) };
    }

    return auto_unbox(boxedKind);
}

void ScrollViewer::IgnoredInputKind(winrt::InputKind const& value)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::InputKindToString(value).c_str());
    SetValue(s_IgnoredInputKindProperty, box_value(value));
}

void ScrollViewer::RegisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (auto scroller = m_scroller.get())
    {
        if (const auto scrollerAsAnchorProvider = scroller.try_as<winrt::Controls::IScrollAnchorProvider>())
        {
            scrollerAsAnchorProvider.RegisterAnchorCandidate(element);
            return;
        }
        throw winrt::hresult_error(E_INVALID_OPERATION, s_iScrollAnchorProviderNotImpl);
    }
    throw winrt::hresult_error(E_INVALID_OPERATION, s_noScrollerPart);
}

void ScrollViewer::UnregisterAnchorCandidate(winrt::UIElement const& element)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_PTR, METH_NAME, this, element);

    if (auto scroller = m_scroller.get())
    {
        if (const auto scrollerAsAnchorProvider = scroller.try_as<winrt::Controls::IScrollAnchorProvider>())
        {
            scrollerAsAnchorProvider.UnregisterAnchorCandidate(element);
            return;
        }
        throw winrt::hresult_error(E_INVALID_OPERATION, s_iScrollAnchorProviderNotImpl);
    }
    throw winrt::hresult_error(E_INVALID_OPERATION, s_noScrollerPart);
}

int32_t ScrollViewer::ChangeOffsets(
    winrt::ScrollerChangeOffsetsOptions const& options)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeOffsetsOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeOffsets(options);
    }

    return -1;
}

int32_t ScrollViewer::ChangeOffsetsWithAdditionalVelocity(
    winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions const& options)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeOffsetsWithAdditionalVelocityOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeOffsetsWithAdditionalVelocity(options);
    }

    return -1;
}

int32_t ScrollViewer::ChangeZoomFactor(
    winrt::ScrollerChangeZoomFactorOptions const& options)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeZoomFactorOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeZoomFactor(options);
    }

    return -1;
}

int32_t ScrollViewer::ChangeZoomFactorWithAdditionalVelocity(
    winrt::ScrollerChangeZoomFactorWithAdditionalVelocityOptions const& options)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerChangeZoomFactorWithAdditionalVelocityOptionsToString(options).c_str());

    if (auto scroller = m_scroller.get())
    {
        return scroller.ChangeZoomFactorWithAdditionalVelocity(options);
    }

    return -1;
}

#pragma endregion

#pragma region IFrameworkElementOverrides

void ScrollViewer::OnApplyTemplate()
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnApplyTemplate();

    m_keepIndicatorsShowing = false;

    winrt::IControlProtected thisAsControlProtected = *this;

    winrt::Scroller scroller = GetTemplateChildT<winrt::Scroller>(s_scrollerPartName, thisAsControlProtected);

    UpdateScroller(scroller);

    winrt::IUIElement horizontalScrollControllerElement = GetTemplateChildT<winrt::IUIElement>(s_horizontalScrollBarPartName, thisAsControlProtected);
    winrt::IScrollController horizontalScrollController = horizontalScrollControllerElement.try_as<winrt::IScrollController>();
    winrt::ScrollBar horizontalScrollBar = nullptr;

    if (horizontalScrollControllerElement && !horizontalScrollController)
    {
        horizontalScrollBar = horizontalScrollControllerElement.try_as<winrt::ScrollBar>();

        if (horizontalScrollBar)
        {
            if (!m_horizontalScrollBarController)
            {
                m_horizontalScrollBarController = winrt::make_self<ScrollBarController>();
            }
            horizontalScrollController = m_horizontalScrollBarController.as<winrt::IScrollController>();
        }
    }

    if (horizontalScrollBar)
    {
        m_horizontalScrollBarController->SetScrollBar(horizontalScrollBar);
    }
    else
    {
        m_horizontalScrollBarController = nullptr;
    }

    UpdateHorizontalScrollController(horizontalScrollController, horizontalScrollControllerElement);

    winrt::IUIElement verticalScrollControllerElement = GetTemplateChildT<winrt::IUIElement>(s_verticalScrollBarPartName, thisAsControlProtected);
    winrt::IScrollController verticalScrollController = verticalScrollControllerElement.try_as<winrt::IScrollController>();
    winrt::ScrollBar verticalScrollBar = nullptr;

    if (verticalScrollControllerElement && !verticalScrollController)
    {
        verticalScrollBar = verticalScrollControllerElement.try_as<winrt::ScrollBar>();

        if (verticalScrollBar)
        {
            if (!m_verticalScrollBarController)
            {
                m_verticalScrollBarController = winrt::make_self<ScrollBarController>();
            }
            verticalScrollController = m_verticalScrollBarController.as<winrt::IScrollController>();
        }
    }

    if (verticalScrollBar)
    {
        m_verticalScrollBarController->SetScrollBar(verticalScrollBar);
    }
    else
    {
        m_verticalScrollBarController = nullptr;
    }

    UpdateVerticalScrollController(verticalScrollController, verticalScrollControllerElement);

    winrt::IUIElement scrollControllersSeparator = GetTemplateChildT<winrt::IUIElement>(s_scrollBarsSeparatorPartName, thisAsControlProtected);

    UpdateScrollControllersSeparator(scrollControllersSeparator);

    UpdateScrollControllersVisibility(true /*horizontalChange*/, true /*verticalChange*/);

    winrt::FrameworkElement root = GetTemplateChildT<winrt::FrameworkElement>(s_rootPartName, thisAsControlProtected);

    if (root)
    {        
        winrt::IVector<winrt::VisualStateGroup> rootVisualStateGroups = winrt::VisualStateManager::GetVisualStateGroups(root);

        if (rootVisualStateGroups)
        {
            uint32_t groupCount = rootVisualStateGroups.Size();

            for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
            {
                winrt::VisualStateGroup group = rootVisualStateGroups.GetAt(groupIndex);

                if (group)
                {
                    winrt::IVector<winrt::VisualState> visualStates = group.States();

                    if (visualStates)
                    {
                        uint32_t stateCount = visualStates.Size();

                        for (uint32_t stateIndex = 0; stateIndex < stateCount; ++stateIndex)
                        {
                            winrt::VisualState state = visualStates.GetAt(stateIndex);

                            if (state)
                            {
                                auto stateName = state.Name();
                                winrt::Storyboard stateStoryboard = state.Storyboard();

                                if (stateStoryboard)
                                {
                                    if (stateName == s_noIndicatorStateName)
                                    {
                                        winrt::event_token noIndicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollViewer::OnNoIndicatorStateStoryboardCompleted });
                                    }
                                    else if (stateName == s_touchIndicatorStateName || stateName == s_mouseIndicatorStateName || stateName == s_mouseIndicatorFullStateName)
                                    {
                                        winrt::event_token indicatorStateStoryboardCompletedToken = stateStoryboard.Completed({ this, &ScrollViewer::OnIndicatorStateStoryboardCompleted });
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    HideIndicators(false /*useTransitions*/);
}

#pragma endregion

#pragma region IControlOverrides

void ScrollViewer::OnGotFocus(winrt::RoutedEventArgs const& args)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnGotFocus(args);

    m_preferMouseIndicators = 
        m_focusInputDeviceKind == winrt::FocusInputDeviceKind::Mouse ||
        m_focusInputDeviceKind == winrt::FocusInputDeviceKind::Pen;

    ShowIndicators();
}

#pragma endregion

void ScrollViewer::OnScrollViewerGettingFocus(
    const winrt::IInspectable& /*sender*/,
    const winrt::GettingFocusEventArgs& args)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_focusInputDeviceKind = args.InputDevice();
}

void ScrollViewer::OnScrollViewerIsEnabledChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (!IsEnabled())
    {
        HideIndicators(true /*useTransitions*/);
    }
}

void ScrollViewer::OnScrollViewerUnloaded(
    const winrt::IInspectable& /*sender*/,
    const winrt::RoutedEventArgs& /*args*/)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_showingMouseIndicators = false;
    m_keepIndicatorsShowing = false;
}

void ScrollViewer::OnScrollViewerPointerEntered(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_preferMouseIndicators = true;
        ShowIndicators();
    }
}

void ScrollViewer::OnScrollViewerPointerMoved(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    // Don't process if this is a generated replay of the event.
    if (SharedHelpers::IsRS3OrHigher() && args.IsGenerated())
    {
        return;
    }

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_preferMouseIndicators = true;
        ShowIndicators();

        if (!SharedHelpers::IsAnimationsEnabled() &&
            m_hideIndicatorsTimer &&
            (m_isPointerOverHorizontalScrollController || m_isPointerOverVerticalScrollController))
        {
            StopHideIndicatorsTimer(false /*isForDestructor*/);
        }
    }
}

void ScrollViewer::OnScrollViewerPointerExited(
    const winrt::IInspectable& sender,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() != winrt::PointerDeviceType::Touch)
    {
        // Mouse/Pen inputs dominate. If touch panning indicators are shown, switch to mouse indicators.
        m_isPointerOverHorizontalScrollController = false;
        m_isPointerOverVerticalScrollController = false;
        m_preferMouseIndicators = true;
        ShowIndicators();

        HideIndicatorsAfterDelay();
    }
}

void ScrollViewer::OnScrollViewerPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Handled())
    {
        return;
    }
    
    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        const winrt::PointerPoint pointerPoint = args.GetCurrentPoint(nullptr);
        const winrt::PointerPointProperties pointerPointProperties = pointerPoint.Properties();

        m_isLeftMouseButtonPressedForFocus = pointerPointProperties.IsLeftButtonPressed();
    }

    // Show the scroll controller indicators as soon as a pointer is pressed on the ScrollViewer.
    ShowIndicators();
}

void ScrollViewer::OnScrollViewerPointerReleased(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    bool takeFocus = false;

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse && m_isLeftMouseButtonPressedForFocus)
    {
        m_isLeftMouseButtonPressedForFocus = false;
        takeFocus = true;
    }

    if (args.Handled())
    {
        return;
    }

    if (takeFocus)
    {
        SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"Focus");

        bool tookFocus = Focus(winrt::FocusState::Pointer);
        args.Handled(tookFocus);
    }
}

void ScrollViewer::OnScrollViewerPointerCanceled(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Mouse)
    {
        m_isLeftMouseButtonPressedForFocus = false;
    }
}

void ScrollViewer::OnHorizontalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = true;
}

void ScrollViewer::OnHorizontalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverHorizontalScrollController = false;
}

void ScrollViewer::OnVerticalScrollControllerPointerEntered(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = true;

    if (!SharedHelpers::IsAnimationsEnabled())
    {
        HideIndicatorsAfterDelay();
    }
}

void ScrollViewer::OnVerticalScrollControllerPointerExited(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.Handled(), args.Pointer().PointerDeviceType());

    m_isPointerOverVerticalScrollController = false;

    HideIndicatorsAfterDelay();
}

// Handler for when the NoIndicator state's storyboard completes animating.
void ScrollViewer::OnNoIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_showingMouseIndicators = false;
}

// Handler for when a TouchIndicator, MouseIndicator or MouseIndicatorFull state's storyboard completes animating.
void ScrollViewer::OnIndicatorStateStoryboardCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    // If the cursor is currently directly over either scroll controller then do not automatically hide the indicators
    if (!m_keepIndicatorsShowing &&
        !m_isPointerOverVerticalScrollController &&
        !m_isPointerOverHorizontalScrollController)
    {
        // Go to the NoIndicator state using transitions.
        if (SharedHelpers::IsAnimationsEnabled())
        {
            // By default there is a delay before the NoIndicator state actually shows.
            HideIndicators(true /*useTransitions*/);
        }
        else
        {
            // Since OS animations are turned off, use a timer to delay the indicators' hiding.
            HideIndicatorsAfterDelay();
        }
    }
}

// Invoked by ScrollViewerTestHooks
winrt::Scroller ScrollViewer::GetScrollerPart()
{
    return safe_cast<winrt::Scroller>(m_scroller.get());
}

void ScrollViewer::ValidateAnchorRatio(double value)
{
    Scroller::ValidateAnchorRatio(value);
}

void ScrollViewer::ValidateZoomFactoryBoundary(double value)
{
    Scroller::ValidateZoomFactoryBoundary(value);
}


// Invoked when a dependency property of this ScrollViewer has changed.
void ScrollViewer::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto dependencyProperty = args.Property();

#ifdef _DEBUG
    SCROLLVIEWER_TRACE_VERBOSE(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToString(dependencyProperty).c_str());
#endif

    bool horizontalChange = dependencyProperty == s_HorizontalScrollBarVisibilityProperty;
    bool verticalChange = dependencyProperty == s_VerticalScrollBarVisibilityProperty;

    if (horizontalChange || verticalChange)
    {
        UpdateScrollControllersVisibility(horizontalChange, verticalChange);
    }
}

void ScrollViewer::OnScrollControllerInteractionInfoChanged(
    const winrt::IScrollController& sender,
    const winrt::IInspectable& /*args*/)
{
    if (m_horizontalScrollController && m_horizontalScrollController == sender)
    {
        bool isHorizontalScrollControllerInteracting = sender.IsInteracting();

        if (m_isHorizontalScrollControllerInteracting != isHorizontalScrollControllerInteracting)
        {
            SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"HorizontalScrollController.IsInteracting changed: ", m_isHorizontalScrollControllerInteracting, isHorizontalScrollControllerInteracting);

            m_isHorizontalScrollControllerInteracting = isHorizontalScrollControllerInteracting;

            if (isHorizontalScrollControllerInteracting)
            {
                // Prevent the vertical scroll controller from fading out while the user is interacting with the horizontal one.
                m_keepIndicatorsShowing = true;

                ShowIndicators();
            }
            else
            {
                // Make the scroll controllers fade out, after the normal delay.
                m_keepIndicatorsShowing = false;

                HideIndicators(true /*useTransitions*/);
            }
        }

        // IScrollController::AreInteractionsAllowed might have changed and affect the scroll controller's visibility
        // when its visibility mode is Auto.
        UpdateScrollControllersVisibility(true /*horizontalChange*/, false /*verticalChange*/);
    }
    else if (m_verticalScrollController && m_verticalScrollController == sender)
    {
        bool isVerticalScrollControllerInteracting = sender.IsInteracting();

        if (m_isVerticalScrollControllerInteracting != isVerticalScrollControllerInteracting)
        {
            SCROLLVIEWER_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"VerticalScrollController.IsInteracting changed: ", m_isVerticalScrollControllerInteracting, isVerticalScrollControllerInteracting);

            m_isVerticalScrollControllerInteracting = isVerticalScrollControllerInteracting;

            if (isVerticalScrollControllerInteracting)
            {
                // Prevent the horizontal scroll controller from fading out while the user is interacting with the vertical one.
                m_keepIndicatorsShowing = true;

                ShowIndicators();
            }
            else
            {
                // Make the scroll controllers fade out, after the normal delay.
                m_keepIndicatorsShowing = false;

                HideIndicators(true /*useTransitions*/);
            }
        }

        // IScrollController::AreInteractionsAllowed might have changed and affect the scroll controller's visibility
        // when its visibility mode is Auto.
        UpdateScrollControllersVisibility(false /*horizontalChange*/, true /*verticalChange*/);
    }
}

void ScrollViewer::OnHideIndicatorsTimerTick(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& /*args*/)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    StopHideIndicatorsTimer(false /*isForDestructor*/);
    HideIndicators(true /*useTransitions*/);
}

void ScrollViewer::OnScrollerExtentChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    if (m_extentChangedEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_extentChangedEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollerStateChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scroller = m_scroller.get())
    {
        if (scroller.State() == winrt::InteractionState::Interaction)
        {
            m_preferMouseIndicators = false;
        }
    }

    if (m_stateChangedEventSource)
    {
        m_stateChangedEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollerChangingOffsets(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerChangingOffsetsEventArgs& args)
{
    if (m_changingOffsetsEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_changingOffsetsEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollerChangingZoomFactor(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerChangingZoomFactorEventArgs& args)
{
    if (m_changingZoomFactorEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_changingZoomFactorEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollViewerChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::IInspectable& args)
{
    // Unless the control is still loading, show the scroll controller indicators when the view changes. For example,
    // when using Ctrl+/- to zoom, mouse-wheel to scroll or zoom, or any other input type. Keep the existing indicator type.
    if (IsLoaded())
    {
        ShowIndicators();
    }

    if (m_viewChangedEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangedEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollViewerChangeCompleted(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerViewChangeCompletedEventArgs& args)
{
    if (args.ViewChangeId() == m_horizontalScrollWithKeyboardViewChangeId)
    {
        m_horizontalScrollWithKeyboardViewChangeId = -1;
    }
    else if (args.ViewChangeId() == m_verticalScrollWithKeyboardViewChangeId)
    {
        m_verticalScrollWithKeyboardViewChangeId = -1;
    }

    if (m_viewChangeCompletedEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_viewChangeCompletedEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollerBringingIntoView(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerBringingIntoViewEventArgs& args)
{
    if (m_bringingIntoViewEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_bringingIntoViewEventSource(*this, args);
    }
}

void ScrollViewer::OnScrollerAnchorRequested(
    const winrt::IInspectable& /*sender*/,
    const winrt::ScrollerAnchorRequestedEventArgs& args)
{
    if (m_anchorRequestedEventSource)
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

        m_anchorRequestedEventSource(*this, args);
    }
}

#ifdef USE_SCROLLMODE_AUTO
void ScrollViewer::OnScrollerPropertyChanged(
    const winrt::DependencyObject& /*sender*/,
    const winrt::DependencyProperty& args)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (args == winrt::Scroller::ComputedHorizontalScrollModeProperty())
    {
        SetValue(s_ComputedHorizontalScrollModeProperty, box_value(m_scroller.get().ComputedHorizontalScrollMode()));
    }
    else if (args == winrt::Scroller::ComputedVerticalScrollModeProperty())
    {
        SetValue(s_ComputedVerticalScrollModeProperty, box_value(m_scroller.get().ComputedVerticalScrollMode()));
    }
}
#endif

void ScrollViewer::StopHideIndicatorsTimer(bool isForDestructor)
{
    winrt::DispatcherTimer hideIndicatorsTimer = m_hideIndicatorsTimer.safe_get(isForDestructor /*useSafeGet*/);

    if (hideIndicatorsTimer && hideIndicatorsTimer.IsEnabled())
    {
        hideIndicatorsTimer.Stop();
    }
}

void ScrollViewer::HookScrollViewerEvents()
{
    MUX_ASSERT(!m_onPointerEnteredEventHandler);
    MUX_ASSERT(!m_onPointerMovedEventHandler);
    MUX_ASSERT(!m_onPointerExitedEventHandler);
    MUX_ASSERT(!m_onPointerPressedEventHandler);
    MUX_ASSERT(!m_onPointerReleasedEventHandler);
    MUX_ASSERT(!m_onPointerCanceledEventHandler);
    MUX_ASSERT(m_gettingFocusToken.value == 0);
    MUX_ASSERT(m_isEnabledChangedToken.value == 0);
    MUX_ASSERT(m_unloadedToken.value == 0);

    m_gettingFocusToken = GettingFocus({ this, &ScrollViewer::OnScrollViewerGettingFocus });
    m_isEnabledChangedToken = IsEnabledChanged({ this, &ScrollViewer::OnScrollViewerIsEnabledChanged });
    m_unloadedToken = Unloaded({ this, &ScrollViewer::OnScrollViewerUnloaded });

    m_onPointerEnteredEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnScrollViewerPointerEntered });
    AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onPointerEnteredEventHandler, false);

    m_onPointerMovedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnScrollViewerPointerMoved });
    AddHandler(winrt::UIElement::PointerMovedEvent(), m_onPointerMovedEventHandler, false);

    m_onPointerExitedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnScrollViewerPointerExited });
    AddHandler(winrt::UIElement::PointerExitedEvent(), m_onPointerExitedEventHandler, false);

    m_onPointerPressedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnScrollViewerPointerPressed });
    AddHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler, false);

    m_onPointerReleasedEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnScrollViewerPointerReleased });
    AddHandler(winrt::UIElement::PointerReleasedEvent(), m_onPointerReleasedEventHandler, true);

    m_onPointerCanceledEventHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnScrollViewerPointerCanceled });
    AddHandler(winrt::UIElement::PointerCanceledEvent(), m_onPointerCanceledEventHandler, true);
}

void ScrollViewer::UnhookScrollViewerEvents()
{
    if (m_gettingFocusToken.value != 0)
    {
        GettingFocus(m_gettingFocusToken);
        m_gettingFocusToken.value = 0;
    }

    if (m_isEnabledChangedToken.value != 0)
    {
        IsEnabledChanged(m_isEnabledChangedToken);
        m_isEnabledChangedToken.value = 0;
    }

    if (m_unloadedToken.value != 0)
    {
        Unloaded(m_unloadedToken);
        m_unloadedToken.value = 0;
    }

    if (m_onPointerEnteredEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_onPointerEnteredEventHandler);
        m_onPointerEnteredEventHandler = nullptr;
    }

    if (m_onPointerMovedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerMovedEvent(), m_onPointerMovedEventHandler);
        m_onPointerMovedEventHandler = nullptr;
    }

    if (m_onPointerExitedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_onPointerExitedEventHandler);
        m_onPointerExitedEventHandler = nullptr;
    }

    if (m_onPointerPressedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_onPointerPressedEventHandler);
        m_onPointerPressedEventHandler = nullptr;
    }

    if (m_onPointerReleasedEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerReleasedEvent(), m_onPointerReleasedEventHandler);
        m_onPointerReleasedEventHandler = nullptr;
    }

    if (m_onPointerCanceledEventHandler)
    {
        RemoveHandler(winrt::UIElement::PointerCanceledEvent(), m_onPointerCanceledEventHandler);
        m_onPointerCanceledEventHandler = nullptr;
    }
}

void ScrollViewer::HookScrollerEvents()
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_scrollerExtentChangedToken.value == 0);
    MUX_ASSERT(m_scrollerStateChangedToken.value == 0);
    MUX_ASSERT(m_scrollerChangingOffsetsToken.value == 0);
    MUX_ASSERT(m_scrollerChangingZoomFactorToken.value == 0);
    MUX_ASSERT(m_scrollViewerChangedToken.value == 0);
    MUX_ASSERT(m_scrollViewerChangeCompletedToken.value == 0);
    MUX_ASSERT(m_scrollerBringingIntoViewToken.value == 0);
    MUX_ASSERT(m_scrollerAnchorRequestedToken.value == 0);
#ifdef USE_SCROLLMODE_AUTO
    MUX_ASSERT(m_scrollerComputedHorizontalScrollModeChangedToken.value == 0);
    MUX_ASSERT(m_scrollerComputedVerticalScrollModeChangedToken.value == 0);
#endif

    if (auto scroller = m_scroller.get())
    {
        m_scrollerExtentChangedToken = scroller.ExtentChanged({ this, &ScrollViewer::OnScrollerExtentChanged });
        m_scrollerStateChangedToken = scroller.StateChanged({ this, &ScrollViewer::OnScrollerStateChanged });
        m_scrollerChangingOffsetsToken = scroller.ChangingOffsets({ this, &ScrollViewer::OnScrollerChangingOffsets });
        m_scrollerChangingZoomFactorToken = scroller.ChangingZoomFactor({ this, &ScrollViewer::OnScrollerChangingZoomFactor });
        m_scrollViewerChangedToken = scroller.ViewChanged({ this, &ScrollViewer::OnScrollViewerChanged });
        m_scrollViewerChangeCompletedToken = scroller.ViewChangeCompleted({ this, &ScrollViewer::OnScrollViewerChangeCompleted });
        m_scrollerBringingIntoViewToken = scroller.BringingIntoView({ this, &ScrollViewer::OnScrollerBringingIntoView });
        m_scrollerAnchorRequestedToken = scroller.AnchorRequested({ this, &ScrollViewer::OnScrollerAnchorRequested });

#ifdef USE_SCROLLMODE_AUTO
        const winrt::DependencyObject scrollerAsDO = scroller.try_as<winrt::DependencyObject>();

        m_scrollerComputedHorizontalScrollModeChangedToken.value = scrollerAsDO.RegisterPropertyChangedCallback(
            winrt::Scroller::ComputedHorizontalScrollModeProperty(), { this, &ScrollViewer::OnScrollerPropertyChanged });

        m_scrollerComputedVerticalScrollModeChangedToken.value = scrollerAsDO.RegisterPropertyChangedCallback(
            winrt::Scroller::ComputedVerticalScrollModeProperty(), { this, &ScrollViewer::OnScrollerPropertyChanged });
#endif
    }
}

void ScrollViewer::UnhookScrollerEvents(bool isForDestructor)
{
    if (isForDestructor)
    {
        SCROLLVIEWER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (auto scroller = m_scroller.safe_get())
    {
        if (m_scrollerExtentChangedToken.value != 0)
        {
            scroller.ExtentChanged(m_scrollerExtentChangedToken);
            m_scrollerExtentChangedToken.value = 0;
        }

        if (m_scrollerStateChangedToken.value != 0)
        {
            scroller.StateChanged(m_scrollerStateChangedToken);
            m_scrollerStateChangedToken.value = 0;
        }

        if (m_scrollerChangingOffsetsToken.value != 0)
        {
            scroller.ChangingOffsets(m_scrollerChangingOffsetsToken);
            m_scrollerChangingOffsetsToken.value = 0;
        }

        if (m_scrollerChangingZoomFactorToken.value != 0)
        {
            scroller.ChangingZoomFactor(m_scrollerChangingZoomFactorToken);
            m_scrollerChangingZoomFactorToken.value = 0;
        }

        if (m_scrollViewerChangedToken.value != 0)
        {
            scroller.ViewChanged(m_scrollViewerChangedToken);
            m_scrollViewerChangedToken.value = 0;
        }

        if (m_scrollViewerChangeCompletedToken.value != 0)
        {
            scroller.ViewChangeCompleted(m_scrollViewerChangeCompletedToken);
            m_scrollViewerChangeCompletedToken.value = 0;
        }

        if (m_scrollerBringingIntoViewToken.value != 0)
        {
            scroller.BringingIntoView(m_scrollerBringingIntoViewToken);
            m_scrollerBringingIntoViewToken.value = 0;
        }

        if (m_scrollerAnchorRequestedToken.value != 0)
        {
            scroller.AnchorRequested(m_scrollerAnchorRequestedToken);
            m_scrollerAnchorRequestedToken.value = 0;
        }

#ifdef USE_SCROLLMODE_AUTO
        const winrt::DependencyObject scrollerAsDO = scroller.try_as<winrt::DependencyObject>();

        if (m_scrollerComputedHorizontalScrollModeChangedToken.value != 0)
        {
            scrollerAsDO.UnregisterPropertyChangedCallback(winrt::Scroller::ComputedHorizontalScrollModeProperty(), m_scrollerComputedHorizontalScrollModeChangedToken.value);
            m_scrollerComputedHorizontalScrollModeChangedToken.value = 0;
        }

        if (m_scrollerComputedVerticalScrollModeChangedToken.value != 0)
        {
            scrollerAsDO.UnregisterPropertyChangedCallback(winrt::Scroller::ComputedVerticalScrollModeProperty(), m_scrollerComputedVerticalScrollModeChangedToken.value);
            m_scrollerComputedVerticalScrollModeChangedToken.value = 0;
        }
#endif
    }
}

void ScrollViewer::HookHorizontalScrollControllerEvents()
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_horizontalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onHorizontalScrollControllerPointerExitedHandler);

    if (winrt::IScrollController horizontalScrollController = m_horizontalScrollController.get())
    {
        m_horizontalScrollControllerInteractionInfoChangedToken =
            horizontalScrollController.InteractionInfoChanged({ this, &ScrollViewer::OnScrollControllerInteractionInfoChanged });
    }

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.get())
    {
        m_onHorizontalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnHorizontalScrollControllerPointerEntered });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onHorizontalScrollControllerPointerEnteredHandler, true);

        m_onHorizontalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnHorizontalScrollControllerPointerExited });
        horizontalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onHorizontalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollViewer::UnhookHorizontalScrollControllerEvents()
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (winrt::IScrollController horizontalScrollController = m_horizontalScrollController.safe_get())
    {
        if (m_horizontalScrollControllerInteractionInfoChangedToken.value != 0)
        {
            horizontalScrollController.InteractionInfoChanged(m_horizontalScrollControllerInteractionInfoChangedToken);
        }
        m_horizontalScrollControllerInteractionInfoChangedToken.value = 0;
    }

    if (winrt::IUIElement horizontalScrollControllerElement = m_horizontalScrollControllerElement.safe_get())
    {
        if (m_onHorizontalScrollControllerPointerEnteredHandler)
        {
            horizontalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_onHorizontalScrollControllerPointerEnteredHandler);
            m_onHorizontalScrollControllerPointerEnteredHandler = nullptr;
        }

        if (m_onHorizontalScrollControllerPointerExitedHandler)
        {
            horizontalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_onHorizontalScrollControllerPointerExitedHandler);
            m_onHorizontalScrollControllerPointerExitedHandler = nullptr;
        }
    }
}

void ScrollViewer::HookVerticalScrollControllerEvents()
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_verticalScrollControllerInteractionInfoChangedToken.value == 0);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerEnteredHandler);
    MUX_ASSERT(!m_onVerticalScrollControllerPointerExitedHandler);

    if (winrt::IScrollController verticalScrollController = m_verticalScrollController.get())
    {
        m_verticalScrollControllerInteractionInfoChangedToken =
            verticalScrollController.InteractionInfoChanged({ this, &ScrollViewer::OnScrollControllerInteractionInfoChanged });
    }

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.get())
    {
        m_onVerticalScrollControllerPointerEnteredHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnVerticalScrollControllerPointerEntered });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerEnteredEvent(), m_onVerticalScrollControllerPointerEnteredHandler, true);

        m_onVerticalScrollControllerPointerExitedHandler = winrt::box_value<winrt::PointerEventHandler>({ this, &ScrollViewer::OnVerticalScrollControllerPointerExited });
        verticalScrollControllerElement.AddHandler(winrt::UIElement::PointerExitedEvent(), m_onVerticalScrollControllerPointerExitedHandler, true);
    }
}

void ScrollViewer::UnhookVerticalScrollControllerEvents()
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (winrt::IScrollController verticalScrollController = m_verticalScrollController.safe_get())
    {
        if (m_verticalScrollControllerInteractionInfoChangedToken.value != 0)
        {
            verticalScrollController.InteractionInfoChanged(m_verticalScrollControllerInteractionInfoChangedToken);
        }
        m_verticalScrollControllerInteractionInfoChangedToken.value = 0;
    }

    if (winrt::IUIElement verticalScrollControllerElement = m_verticalScrollControllerElement.safe_get())
    {
        if (m_onVerticalScrollControllerPointerEnteredHandler)
        {
            verticalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerEnteredEvent(), m_onVerticalScrollControllerPointerEnteredHandler);
            m_onVerticalScrollControllerPointerEnteredHandler = nullptr;
        }

        if (m_onVerticalScrollControllerPointerExitedHandler)
        {
            verticalScrollControllerElement.RemoveHandler(winrt::UIElement::PointerExitedEvent(), m_onVerticalScrollControllerPointerExitedHandler);
            m_onVerticalScrollControllerPointerExitedHandler = nullptr;
        }
    }
}

void ScrollViewer::UpdateScroller(const winrt::Scroller& scroller)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookScrollerEvents(false /*isForDestructor*/);
    m_scroller.set(nullptr);

    SetValue(s_ScrollerProperty, scroller);

    if (scroller)
    {
        m_scroller.set(scroller);
        HookScrollerEvents();
    }
}

void ScrollViewer::UpdateHorizontalScrollController(
    const winrt::IScrollController& horizontalScrollController,
    const winrt::IUIElement& horizontalScrollControllerElement)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookHorizontalScrollControllerEvents();

    SetValue(s_HorizontalScrollControllerProperty, horizontalScrollController);

    m_horizontalScrollController.set(horizontalScrollController);
    m_horizontalScrollControllerElement.set(horizontalScrollControllerElement);
    HookHorizontalScrollControllerEvents();
    UpdateScrollerHorizontalScrollController(horizontalScrollController);    
}

void ScrollViewer::UpdateScrollerHorizontalScrollController(const winrt::IScrollController& horizontalScrollController)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scroller = m_scroller.get())
    {
        scroller.HorizontalScrollController(horizontalScrollController);
    }
}

void ScrollViewer::UpdateVerticalScrollController(
    const winrt::IScrollController& verticalScrollController,
    const winrt::IUIElement& verticalScrollControllerElement)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UnhookVerticalScrollControllerEvents();

    SetValue(s_VerticalScrollControllerProperty, verticalScrollController);

    m_verticalScrollController.set(verticalScrollController);
    m_verticalScrollControllerElement.set(verticalScrollControllerElement);
    HookVerticalScrollControllerEvents();
    UpdateScrollerVerticalScrollController(verticalScrollController);
}

void ScrollViewer::UpdateScrollerVerticalScrollController(const winrt::IScrollController& verticalScrollController)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (auto scroller = m_scroller.get())
    {
        scroller.VerticalScrollController(verticalScrollController);
    }
}

void ScrollViewer::UpdateScrollControllersSeparator(const winrt::IUIElement& scrollControllersSeparator)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_scrollControllersSeparatorElement.set(scrollControllersSeparator);
}

void ScrollViewer::UpdateScrollControllersVisibility(
    bool horizontalChange, bool verticalChange)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(horizontalChange || verticalChange);

    bool isHorizontalScrollControllerVisible = false;

    if (horizontalChange)
    {
        winrt::ScrollBarVisibility scrollBarVisibility = HorizontalScrollBarVisibility();

        if (scrollBarVisibility == winrt::ScrollBarVisibility::Auto &&
            m_horizontalScrollController &&
            m_horizontalScrollController.get().AreInteractionsAllowed())
        {
            isHorizontalScrollControllerVisible = true;
        }
        else
        {
            isHorizontalScrollControllerVisible = (scrollBarVisibility == winrt::ScrollBarVisibility::Visible);
        }

        SetValue(s_ComputedHorizontalScrollBarVisibilityProperty, box_value(isHorizontalScrollControllerVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed));
    }
    else
    {
        isHorizontalScrollControllerVisible = ComputedHorizontalScrollBarVisibility() == winrt::Visibility::Visible;
    }

    bool isVerticalScrollControllerVisible = false;

    if (verticalChange)
    {
        winrt::ScrollBarVisibility scrollBarVisibility = VerticalScrollBarVisibility();

        if (scrollBarVisibility == winrt::ScrollBarVisibility::Auto &&
            m_verticalScrollController &&
            m_verticalScrollController.get().AreInteractionsAllowed())
        {
            isVerticalScrollControllerVisible = true;
        }
        else
        {
            isVerticalScrollControllerVisible = (scrollBarVisibility == winrt::ScrollBarVisibility::Visible);
        }

        SetValue(s_ComputedVerticalScrollBarVisibilityProperty, box_value(isVerticalScrollControllerVisible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed));
    }
    else
    {
        isVerticalScrollControllerVisible = ComputedVerticalScrollBarVisibility() == winrt::Visibility::Visible;
    }

    if (m_scrollControllersSeparatorElement)
    {
        m_scrollControllersSeparatorElement.get().Visibility(isHorizontalScrollControllerVisible && isVerticalScrollControllerVisible ?
            winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

bool ScrollViewer::IsLoaded()
{
    return winrt::VisualTreeHelper::GetParent(*this) != nullptr;
}

bool ScrollViewer::IsInputKindIgnored(winrt::InputKind const& inputKind)
{
    return (IgnoredInputKind() & inputKind) == inputKind;
}

bool ScrollViewer::AreAllScrollControllersCollapsed()
{
    return (!m_horizontalScrollControllerElement || m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed) &&
           (!m_verticalScrollControllerElement || m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Collapsed);
}

bool ScrollViewer::AreBothScrollControllersVisible()
{
    return m_horizontalScrollControllerElement && m_horizontalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible &&
           m_verticalScrollControllerElement && m_verticalScrollControllerElement.get().Visibility() == winrt::Visibility::Visible;
}

// Show the appropriate scrolling indicators.
void ScrollViewer::ShowIndicators()
{
    if (!AreAllScrollControllersCollapsed())
    {
        if (m_hideIndicatorsTimer)
        {
            winrt::DispatcherTimer hideIndicatorsTimer = m_hideIndicatorsTimer.get();

            if (hideIndicatorsTimer.IsEnabled())
            {
                hideIndicatorsTimer.Stop();
                hideIndicatorsTimer.Start();
            }
        }

        // Mouse indicators dominate if they are already showing or if we have set the flag to prefer them.
        if (m_preferMouseIndicators || m_showingMouseIndicators)
        {
            if (AreBothScrollControllersVisible() && (m_isPointerOverHorizontalScrollController || m_isPointerOverVerticalScrollController))
            {
                winrt::VisualStateManager::GoToState(*this, s_mouseIndicatorFullStateName, true /*useTransitions*/);

                SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_mouseIndicatorFullStateName);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, s_mouseIndicatorStateName, true /*useTransitions*/);

                SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_mouseIndicatorStateName);
            }

            m_showingMouseIndicators = true;
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, s_touchIndicatorStateName, true /*useTransitions*/);

            SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, s_touchIndicatorStateName);
        }
    }
}

void ScrollViewer::HideIndicators(
    bool useTransitions)
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, s_noIndicatorStateName, useTransitions, m_keepIndicatorsShowing);

    if (!m_keepIndicatorsShowing)
    {
        winrt::VisualStateManager::GoToState(*this, s_noIndicatorStateName, useTransitions);
    }
}

void ScrollViewer::HideIndicatorsAfterDelay()
{
    SCROLLVIEWER_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, m_keepIndicatorsShowing);

    if (!m_keepIndicatorsShowing)
    {
        winrt::DispatcherTimer hideIndicatorsTimer = nullptr;

        if (m_hideIndicatorsTimer)
        {
            hideIndicatorsTimer = m_hideIndicatorsTimer.get();
            if (hideIndicatorsTimer.IsEnabled())
            {
                hideIndicatorsTimer.Stop();
            }
        }
        else
        {
            hideIndicatorsTimer = winrt::DispatcherTimer();
            hideIndicatorsTimer.Interval(winrt::TimeSpan::duration(s_noIndicatorCountdown));
            hideIndicatorsTimer.Tick({ this, &ScrollViewer::OnHideIndicatorsTimerTick });
            m_hideIndicatorsTimer.set(hideIndicatorsTimer);
        }

        hideIndicatorsTimer.Start();
    }
}

void ScrollViewer::OnKeyDown(winrt::KeyRoutedEventArgs const& e)
{
    __super::OnKeyDown(e);
    
    m_preferMouseIndicators = false;

    if (m_scroller)
    {
        winrt::KeyRoutedEventArgs eventArgs = e.as<winrt::KeyRoutedEventArgs>();
        if (!eventArgs.Handled())
        {
            auto originalKey = eventArgs.OriginalKey();
            bool isGamepadKey = FocusHelper::IsGamepadNavigationDirection(originalKey) || FocusHelper::IsGamepadPageNavigationDirection(originalKey);

            if (isGamepadKey)
            {
                if (IsInputKindIgnored(winrt::InputKind::Gamepad))
                {
                    return;
                }
            }
            else
            {
                if (IsInputKindIgnored(winrt::InputKind::Keyboard))
                {
                    return;
                }
            }

            bool isXYFocusEnabledForKeyboard = XYFocusKeyboardNavigation() == winrt::XYFocusKeyboardNavigationMode::Enabled;
            bool doXYFocusScrolling = isGamepadKey || isXYFocusEnabledForKeyboard;

            if (doXYFocusScrolling)
            {
                HandleKeyDownForXYNavigation(eventArgs);
            }
            else
            {
                HandleKeyDownForStandardScroll(eventArgs);
            }
        }
    }
}

void ScrollViewer::HandleKeyDownForStandardScroll(winrt::KeyRoutedEventArgs args)
{
    // Up/Down/Left/Right will scroll by 15% the size of the viewport.
    static const double smallScrollProportion = 0.15;

    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scroller != nullptr);

    bool isHandled = DoScrollForKey(args.Key(), smallScrollProportion);

    args.Handled(isHandled);
}

void ScrollViewer::HandleKeyDownForXYNavigation(winrt::KeyRoutedEventArgs args)
{
    MUX_ASSERT(!args.Handled());
    MUX_ASSERT(m_scroller != nullptr);

    bool isHandled = false;
    auto originalKey = args.OriginalKey();
    auto scroller = m_scroller.get().as<winrt::Scroller>();
    bool isPageNavigation = FocusHelper::IsGamepadPageNavigationDirection(originalKey);
    double scrollAmountProportion = isPageNavigation ? 1.0 : 0.5;
    bool shouldProcessKeyEvent = true;
    winrt::FocusNavigationDirection navigationDirection;

    if (isPageNavigation)
    {
        navigationDirection = FocusHelper::GetPageNavigationDirection(originalKey);

        // We should only handle page navigation if we can scroll in that direction.
        // Note: For non-paging navigation, we might want to move focus even if we cannot scroll.
        shouldProcessKeyEvent = CanScrollInDirection(navigationDirection);
    }
    else
    {
        navigationDirection = FocusHelper::GetNavigationDirection(originalKey);
    }

    if (shouldProcessKeyEvent)
    {
        bool shouldScroll = false;
        bool shouldMoveFocus = false;
        winrt::DependencyObject nextElement = nullptr;

        if (navigationDirection != winrt::FocusNavigationDirection::None)
        {
            nextElement = GetNextFocusCandidate(navigationDirection, isPageNavigation);
        }

        if (nextElement && nextElement != winrt::FocusManager::GetFocusedElement())
        {
            winrt::UIElement nextElementAsUIE = FocusHelper::GetUIElementForFocusCandidate(nextElement);
            MUX_ASSERT(nextElementAsUIE != nullptr);

            auto nextElementAsFe = nextElementAsUIE.as<winrt::FrameworkElement>();
            auto rect = winrt::Rect{ 0, 0, static_cast<float>(nextElementAsFe.ActualWidth()), static_cast<float>(nextElementAsFe.ActualHeight()) };
            auto elementBounds = nextElementAsUIE.TransformToVisual(scroller).TransformBounds(rect);
            auto viewport = winrt::Rect{ 0, 0, static_cast<float>(scroller.ActualWidth()), static_cast<float>(scroller.ActualHeight()) };

            // Extend the viewport in the direction we are moving:
            winrt::Rect extendedViewport = viewport;
            switch (navigationDirection)
            {
            case winrt::FocusNavigationDirection::Down:
                extendedViewport.Height += viewport.Height;
                break;
            case winrt::FocusNavigationDirection::Up:
                extendedViewport.Y -= viewport.Height;
                extendedViewport.Height += viewport.Height;
                break;
            case winrt::FocusNavigationDirection::Left:
                extendedViewport.X -= viewport.Width;
                extendedViewport.Width += viewport.Width;
                break;
            case winrt::FocusNavigationDirection::Right:
                extendedViewport.Width += viewport.Width;
                break;
            }

            bool isElementInExtendedViewport = winrt::RectHelper::Intersect(elementBounds, extendedViewport) != winrt::RectHelper::Empty();
            bool isElementFullyInExtendedViewport = winrt::RectHelper::Union(elementBounds, extendedViewport) == extendedViewport;

            if (isElementInExtendedViewport)
            {
                if (isPageNavigation)
                {
                    // Always scroll for page navigation
                    shouldScroll = true;

                    if (isElementFullyInExtendedViewport)
                    {
                        // Move focus:
                        shouldMoveFocus = true;
                    }
                }
                else
                {
                    // Non-paging scroll allows partial candidates
                    shouldMoveFocus = true;
                }
            }
            else
            {
                // Element is outside extended viewport - scroll but don't focus.
                shouldScroll = true;
            }
        }
        else
        {
            // No focus candidate: scroll
            shouldScroll = true;
        }

        if (shouldMoveFocus)
        {
            winrt::FocusManager::TryFocusAsync(nextElement, winrt::FocusState::Keyboard);
            isHandled = true;
        }

        if (shouldScroll)
        {
            if (navigationDirection == winrt::FocusNavigationDirection::None)
            {
                isHandled = DoScrollForKey(args.Key(), scrollAmountProportion);
            }
            else
            {
                if (navigationDirection == winrt::FocusNavigationDirection::Down && CanScrollDown())
                {
                    isHandled = true;
                    DoScroll(scroller.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Up && CanScrollUp())
                {
                    isHandled = true;
                    DoScroll(-scroller.ActualHeight() * scrollAmountProportion, winrt::Orientation::Vertical);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Right && CanScrollRight())
                {
                    isHandled = true;
                    DoScroll(scroller.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
                else if (navigationDirection == winrt::FocusNavigationDirection::Left && CanScrollLeft())
                {
                    isHandled = true;
                    DoScroll(-scroller.ActualWidth() * scrollAmountProportion * (FlowDirection() == winrt::FlowDirection::RightToLeft ? -1 : 1), winrt::Orientation::Horizontal);
                }
            }
        }
    }

    args.Handled(isHandled);
}

winrt::DependencyObject ScrollViewer::GetNextFocusCandidate(winrt::FocusNavigationDirection navigationDirection, bool isPageNavigation)
{
    MUX_ASSERT(m_scroller != nullptr);
    MUX_ASSERT(navigationDirection != winrt::FocusNavigationDirection::None);
    auto scroller = m_scroller.get().as<winrt::Scroller>();

    winrt::FocusNavigationDirection focusDirection = navigationDirection;

    winrt::FindNextElementOptions findNextElementOptions;
    findNextElementOptions.SearchRoot(scroller.Content());

    if (isPageNavigation)
    {
        auto localBounds = winrt::Rect{ 0, 0, static_cast<float>(scroller.ActualWidth()), static_cast<float>(scroller.ActualHeight()) };
        auto globalBounds = scroller.TransformToVisual(nullptr).TransformBounds(localBounds);
        const int numPagesLookAhead = 2;

        auto hintRect = globalBounds;
        switch (navigationDirection)
        {
        case winrt::FocusNavigationDirection::Down:
            hintRect.Y += globalBounds.Height * numPagesLookAhead;
            break;
        case winrt::FocusNavigationDirection::Up:
            hintRect.Y -= globalBounds.Height * numPagesLookAhead;
            break;
        case winrt::FocusNavigationDirection::Left:
            hintRect.X -= globalBounds.Width * numPagesLookAhead;
            break;
        case winrt::FocusNavigationDirection::Right:
            hintRect.X += globalBounds.Width * numPagesLookAhead;
            break;
        default:
            MUX_ASSERT(false);
            break;
        }
        
        findNextElementOptions.HintRect(hintRect);
        findNextElementOptions.ExclusionRect(hintRect);
        focusDirection = FocusHelper::GetOppositeDirection(navigationDirection);
    }

    return winrt::FocusManager::FindNextElement(focusDirection, findNextElementOptions);
}

bool ScrollViewer::DoScrollForKey(winrt::VirtualKey key, double scrollProportion)
{
    MUX_ASSERT(m_scroller != nullptr);

    bool isScrollTriggered = false;
    auto scroller = m_scroller.get().as<winrt::Scroller>();

    if (key == winrt::VirtualKey::Down && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scroller.ActualHeight() * scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Up && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(scroller.ActualHeight() * -scrollProportion, winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageDown && CanScrollDown())
    {
        isScrollTriggered = true;
        DoScroll(scroller.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::PageUp && CanScrollUp())
    {
        isScrollTriggered = true;
        DoScroll(-scroller.ActualHeight(), winrt::Orientation::Vertical);
    }
    else if (key == winrt::VirtualKey::Left || key == winrt::VirtualKey::Right)
    {
        double scrollAmount = scroller.ActualWidth() * scrollProportion;
        if (FlowDirection() == winrt::FlowDirection::RightToLeft)
        {
            scrollAmount *= -1;
        }

        if (key == winrt::VirtualKey::Right && CanScrollRight())
        {
            isScrollTriggered = true;
            DoScroll(scrollAmount, winrt::Orientation::Horizontal);
        }
        else if (key == winrt::VirtualKey::Left && CanScrollLeft())
        {
            isScrollTriggered = true;
            DoScroll(-scrollAmount, winrt::Orientation::Horizontal);
        }
    }
    else if (key == winrt::VirtualKey::Home)
    {
        bool canScrollUp = CanScrollUp();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        winrt::ScrollMode verticalScrollMode = VerticalScrollMode();
#endif

        if (canScrollUp || (verticalScrollMode == winrt::ScrollMode::Disabled && CanScrollLeft()))
        {
            isScrollTriggered = true;
            auto horizontalOffset = canScrollUp ? scroller.HorizontalOffset() : 0.0;
            auto verticalOffset = canScrollUp ? 0.0 : scroller.VerticalOffset();

            if (!canScrollUp && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = scroller.ExtentWidth() * scroller.ZoomFactor() - scroller.ActualWidth();
            }

            winrt::ScrollerChangeOffsetsOptions options(horizontalOffset, verticalOffset, winrt::ScrollerViewKind::Absolute, winrt::ScrollerViewChangeKind::AllowAnimation, winrt::ScrollerViewChangeSnapPointRespect::RespectSnapPoints);
            scroller.ChangeOffsets(options);
        }
    }
    else if (key == winrt::VirtualKey::End)
    {
        bool canScrollDown = CanScrollDown();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        winrt::ScrollMode verticalScrollMode = VerticalScrollMode();
#endif

        if (canScrollDown || (verticalScrollMode == winrt::ScrollMode::Disabled && CanScrollRight()))
        {
            isScrollTriggered = true;
            auto zoomedExtent = (canScrollDown ? scroller.ExtentHeight() : scroller.ExtentWidth()) * scroller.ZoomFactor();
            auto horizontalOffset = canScrollDown ? scroller.HorizontalOffset() : zoomedExtent - scroller.ActualWidth();
            auto verticalOffset = canScrollDown ? zoomedExtent - scroller.ActualHeight() : scroller.VerticalOffset();

            if (!canScrollDown && FlowDirection() == winrt::FlowDirection::RightToLeft)
            {
                horizontalOffset = 0.0;
            }

            winrt::ScrollerChangeOffsetsOptions options(horizontalOffset, verticalOffset, winrt::ScrollerViewKind::Absolute, winrt::ScrollerViewChangeKind::AllowAnimation, winrt::ScrollerViewChangeSnapPointRespect::RespectSnapPoints);
            scroller.ChangeOffsets(options);
        }
    }

    return isScrollTriggered;
}

void ScrollViewer::DoScroll(double offset, winrt::Orientation orientation)
{
    static const winrt::float2 inertiaDecayRate(0.9995f, 0.9995f);

    // A velocity less than or equal to this value has no effect.
    static const double minVelocity = 30.0;

    // We need to add this much velocity over minVelocity per pixel we want to move:
    static constexpr double s_velocityNeededPerPixel{ 7.600855902349023 };

    bool isVertical = orientation == winrt::Orientation::Vertical;

    if (auto scroller = m_scroller.get().as<winrt::Scroller>())
    {
        int scrollDir = offset > 0 ? 1 : -1;

        // The minimum velocity required to move in the given direction.
        double baselineVelocity = minVelocity * scrollDir;

        // If there is already a scroll animation running for a previous key press, we want to take that into account
        // for calculating the baseline velocity. 
        auto previousScrollViewChangeId = isVertical ? m_verticalScrollWithKeyboardViewChangeId : m_horizontalScrollWithKeyboardViewChangeId;
        if (previousScrollViewChangeId != -1)
        {
            auto directionOfPreviousScrollOperation = isVertical ? m_verticalScrollWithKeyboardDirection : m_horizontalScrollWithKeyboardDirection;
            if (directionOfPreviousScrollOperation == 1)
            {
                baselineVelocity -= minVelocity;
            }
            else if (directionOfPreviousScrollOperation == -1)
            {
                baselineVelocity += minVelocity;
            }
        }

        float velocity = static_cast<float>(baselineVelocity + (offset * s_velocityNeededPerPixel));

        if (isVertical)
        {
            winrt::float2 additionalVelocity(0.0f, velocity);
            winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions options(additionalVelocity, inertiaDecayRate);
            m_verticalScrollWithKeyboardViewChangeId = scroller.ChangeOffsetsWithAdditionalVelocity(options);
            m_verticalScrollWithKeyboardDirection = scrollDir;
        }
        else
        {
            winrt::float2 additionalVelocity(velocity, 0.0f);
            winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions options(additionalVelocity, inertiaDecayRate);
            m_horizontalScrollWithKeyboardViewChangeId = scroller.ChangeOffsetsWithAdditionalVelocity(options);
            m_horizontalScrollWithKeyboardDirection = scrollDir;
        }
    }
}

bool ScrollViewer::CanScrollInDirection(winrt::FocusNavigationDirection direction)
{
    bool result = false;
    switch (direction)
    {
    case winrt::FocusNavigationDirection::Down:
        result = CanScrollDown();
        break;
    case winrt::FocusNavigationDirection::Up:
        result = CanScrollUp();
        break;
    case winrt::FocusNavigationDirection::Left:
        result = CanScrollLeft();
        break;
    case winrt::FocusNavigationDirection::Right:
        result = CanScrollRight();
        break;
    }

    return result;
}

bool ScrollViewer::CanScrollDown()
{
    return CanScrollVerticallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollViewer::CanScrollUp()
{
    return CanScrollVerticallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollViewer::CanScrollRight()
{
    return CanScrollHorizontallyInDirection(true /*inPositiveDirection*/);
}

bool ScrollViewer::CanScrollLeft()
{
    return CanScrollHorizontallyInDirection(false /*inPositiveDirection*/);
}

bool ScrollViewer::CanScrollVerticallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;
    if (m_scroller)
    {
        auto scroller = m_scroller.get().as<winrt::Scroller>();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollMode verticalScrollMode = ComputedVerticalScrollMode();
#else
        winrt::ScrollMode verticalScrollMode = VerticalScrollMode();
#endif

        if (verticalScrollMode == winrt::ScrollMode::Enabled)
        {
            auto zoomedExtentHeight = scroller.ExtentHeight() * scroller.ZoomFactor();
            auto viewportHeight = scroller.ActualHeight();
            if (zoomedExtentHeight > viewportHeight)
            {
                if (inPositiveDirection)
                {
                    auto maxVerticalOffset = zoomedExtentHeight - viewportHeight;
                    if (scroller.VerticalOffset() < maxVerticalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scroller.VerticalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    return canScrollInDirection;
}

bool ScrollViewer::CanScrollHorizontallyInDirection(bool inPositiveDirection)
{
    bool canScrollInDirection = false;

    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        inPositiveDirection = !inPositiveDirection;
    }
    
    if (m_scroller)
    {
        auto scroller = m_scroller.get().as<winrt::Scroller>();
#ifdef USE_SCROLLMODE_AUTO
        winrt::ScrollMode horizontalScrollMode = ComputedHorizontalScrollMode();
#else
        winrt::ScrollMode horizontalScrollMode = HorizontalScrollMode();
#endif

        if (horizontalScrollMode == winrt::ScrollMode::Enabled)
        {
            auto zoomedExtentWidth = scroller.ExtentWidth() * scroller.ZoomFactor();
            auto viewportWidth = scroller.ActualWidth();
            if (zoomedExtentWidth > viewportWidth)
            {
                if (inPositiveDirection)
                {
                    auto maxHorizontalOffset = zoomedExtentWidth - viewportWidth;
                    if (scroller.HorizontalOffset() < maxHorizontalOffset)
                    {
                        canScrollInDirection = true;
                    }
                }
                else
                {
                    if (scroller.HorizontalOffset() > 0)
                    {
                        canScrollInDirection = true;
                    }
                }
            }
        }
    }

    return canScrollInDirection;
}


#ifdef _DEBUG

winrt::hstring ScrollViewer::DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty)
{
    if (dependencyProperty == s_ContentProperty)
    {
        return L"Content";
    }
    else if (dependencyProperty == s_ScrollerProperty)
    {
        return L"Scroller";
    }
    else if (dependencyProperty == s_HorizontalScrollControllerProperty)
    {
        return L"HorizontalScrollController";
    }
    else if (dependencyProperty == s_VerticalScrollControllerProperty)
    {
        return L"VerticalScrollController";
    }
    else if (dependencyProperty == s_HorizontalScrollBarVisibilityProperty)
    {
        return L"HorizontalScrollBarVisibility";
    }
    else if (dependencyProperty == s_VerticalScrollBarVisibilityProperty)
    {
        return L"VerticalScrollBarVisibility";
    }
    else if (dependencyProperty == s_ContentOrientationProperty)
    {
        return L"ContentOrientation";
    }
    else if (dependencyProperty == s_VerticalScrollChainingModeProperty)
    {
        return L"VerticalScrollChainingMode";
    }
    else if (dependencyProperty == s_ZoomChainingModeProperty)
    {
        return L"ZoomChainingMode";
    }
    else if (dependencyProperty == s_HorizontalScrollRailingModeProperty)
    {
        return L"HorizontalScrollRailingMode";
    }
    else if (dependencyProperty == s_VerticalScrollRailingModeProperty)
    {
        return L"VerticalScrollRailingMode";
    }
    else if (dependencyProperty == s_HorizontalScrollModeProperty)
    {
        return L"HorizontalScrollMode";
    }
    else if (dependencyProperty == s_VerticalScrollModeProperty)
    {
        return L"VerticalScrollMode";
    }    
    else if (dependencyProperty == s_ComputedHorizontalScrollBarVisibilityProperty)
    {
        return L"ComputedHorizontalScrollBarVisibility";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollBarVisibilityProperty)
    {
        return L"ComputedVerticalScrollBarVisibility";
    }
#ifdef USE_SCROLLMODE_AUTO
    else if (dependencyProperty == s_ComputedHorizontalScrollModeProperty)
    {
        return L"ComputedHorizontalScrollMode";
    }
    else if (dependencyProperty == s_ComputedVerticalScrollModeProperty)
    {
        return L"ComputedVerticalScrollMode";
    }
#endif
    else if (dependencyProperty == s_ZoomModeProperty)
    {
        return L"ZoomMode";
    }
    else if (dependencyProperty == s_IgnoredInputKindProperty)
    {
        return L"IgnoredInputKind";
    }
    else if (dependencyProperty == s_MinZoomFactorProperty)
    {
        return L"MinZoomFactor";
    }
    else if (dependencyProperty == s_MaxZoomFactorProperty)
    {
        return L"MaxZoomFactor";
    }
    else if (dependencyProperty == s_HorizontalAnchorRatioProperty)
    {
        return L"HorizontalAnchorRatio";
    }
    else if (dependencyProperty == s_VerticalAnchorRatioProperty)
    {
        return L"VerticalAnchorRatio";
    }
    else
    {
        return L"UNKNOWN";
    }
}

#endif
