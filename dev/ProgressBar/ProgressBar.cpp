﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressBar.h"
#include "ProgressBarAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ProgressBar::ProgressBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressBar);

    SetDefaultStyleKey(this);

    SizeChanged({ this, &ProgressBar::OnSizeChanged });

    // NOTE: This is necessary only because Value isn't one of OUR properties, it's implemented in RangeBase.
    // If it was one of ProgressBar's properties, defined in the IDL, you'd do it differently (see IsIndeterminate).
    RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), { this, &ProgressBar::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::MinimumProperty(), { this, &ProgressBar::OnRangeBasePropertyChanged });
    RegisterPropertyChangedCallback(winrt::RangeBase::MaximumProperty(), { this, &ProgressBar::OnRangeBasePropertyChanged });

    SetValue(s_TemplateSettingsProperty, winrt::make<::ProgressBarTemplateSettings>());
}

winrt::AutomationPeer ProgressBar::OnCreateAutomationPeer()
{
    return winrt::make<ProgressBarAutomationPeer>(*this);
}


void ProgressBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // NOTE: Example of how named parts are loaded from the template. Important to remember that it's possible for
    // any of them not to be found, since devs can replace the template with their own.

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(s_LayoutRootName, controlProtected));
    m_progressBarIndicator.set(GetTemplateChildT<winrt::Rectangle>(s_ProgressBarIndicatorName, controlProtected));

    UpdateStates();
}

void ProgressBar::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    SetProgressBarIndicatorWidth();
    UpdateWidthBasedTemplateSettings();
}

void ProgressBar::OnRangeBasePropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    // NOTE: This hits when the Value property changes, because we called RegisterPropertyChangedCallback.
    SetProgressBarIndicatorWidth();
}

void ProgressBar::OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // NOTE: This hits when IsIndeterminate changes because we set MUX_PROPERTY_CHANGED_CALLBACK to true in the idl.
    SetProgressBarIndicatorWidth();
    UpdateStates(); 
}

void ProgressBar::OnShowPausedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressBar::OnShowErrorPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateStates();
}

void ProgressBar::UpdateStates()
{
    m_shouldUpdateWidthBasedTemplateSettings = false;

    if (ShowError())
    {
        winrt::VisualStateManager::GoToState(*this, s_ErrorStateName, true);
    }
    else if (ShowPaused() && IsIndeterminate())
    {
        winrt::VisualStateManager::GoToState(*this, s_ErrorStateName, true); // Paused-Indeterminate state same visual treatment as Error state
    }
    else if (ShowPaused())
    {
        winrt::VisualStateManager::GoToState(*this, s_PausedStateName, true);
    }
    else if (IsIndeterminate())
    {
        m_shouldUpdateWidthBasedTemplateSettings = true;
        UpdateWidthBasedTemplateSettings();
        winrt::VisualStateManager::GoToState(*this, s_IndeterminateStateName, true);
    }
    else if (!IsIndeterminate())
    {
        winrt::VisualStateManager::GoToState(*this, s_DeterminateStateName, true);
    }
}

void ProgressBar::SetProgressBarIndicatorWidth()
{
    const auto templateSettings = winrt::get_self<::ProgressBarTemplateSettings>(TemplateSettings());

    if (auto&& progressBar = m_layoutRoot.get())
    {
        if (auto&& progressBarIndicator = m_progressBarIndicator.get())
        {
            const double progressBarWidth = progressBar.ActualWidth();
            const double prevIndicatorWidth = progressBarIndicator.ActualWidth();
            const double maximum = Maximum();
            const double minimum = Minimum();
            const auto padding = Padding();

            // Adds "Updating" state in between to trigger RepositionThemeAnimation Visual Transition
            // in ProgressBar.xaml when reverting back to previous state
            winrt::VisualStateManager::GoToState(*this, s_UpdatingStateName, true); 

            if (IsIndeterminate())
            {
                progressBarIndicator.Width(progressBarWidth * 0.4);
            }
            else if (std::abs(maximum - minimum) > DBL_EPSILON)
            {
                const double maxIndicatorWidth = progressBarWidth - (padding.Left + padding.Right);
                const double increment = maxIndicatorWidth / (maximum - minimum);
                const double indicatorWidth = increment * (Value() - minimum);
                const double widthDelta = indicatorWidth - prevIndicatorWidth;
                templateSettings->IndicatorLengthDelta(-widthDelta);
                progressBarIndicator.Width(indicatorWidth);
            }
            else
            {
                progressBarIndicator.Width(0); // Error
            }
           
            UpdateStates(); // Reverts back to previous state
        }
    }
}

void ProgressBar::UpdateWidthBasedTemplateSettings()
{
    const auto templateSettings = winrt::get_self<::ProgressBarTemplateSettings>(TemplateSettings());

    if (auto&& progressBarIndicator = m_progressBarIndicator.get())
    {
        const auto [width, height] = [progressBar = m_layoutRoot.get()]()
        {
            if (progressBar)
            {
                const float width = static_cast<float>(progressBar.ActualWidth());
                const float height = static_cast<float>(progressBar.ActualHeight());
                return std::make_tuple(width, height);
            }
            return std::make_tuple(0.0f, 0.0f);
        }();

        const double indicatorWidthMultiplier = -0.4;

        templateSettings->ContainerAnimationStartPosition(width * indicatorWidthMultiplier);
        templateSettings->ContainerAnimationEndPosition(width);

        const auto rectangle = [width, height, padding = Padding()]()
        {
            const auto returnValue = winrt::RectangleGeometry();
            returnValue.Rect({
                static_cast<float>(padding.Left),
                static_cast<float>(padding.Top),
                width - static_cast<float>(padding.Right + padding.Left),
                height - static_cast<float>(padding.Bottom + padding.Top)
                });
            return returnValue;
        }();

        templateSettings->ClipRect(rectangle);
    }
}
