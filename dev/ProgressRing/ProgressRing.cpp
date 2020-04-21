﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "math.h"

static constexpr wstring_view s_IndeterminateAnimatedVisualPlayerName{ L"IndeterminateAnimatedVisualPlayer"sv };
static constexpr wstring_view s_LayoutRootName { L"LayoutRoot"sv };
static constexpr wstring_view s_DefaultForegroundThemeResourceName{ L"SystemControlHighlightAccentBrush"sv };
static constexpr wstring_view s_DefaultBackgroundThemeResourceName{ L"SystemControlBackgroundBaseLowBrush"sv };
static constexpr wstring_view s_ForegroundName{ L"Foreground"sv };
static constexpr wstring_view s_BackgroundName{ L"Background"sv };

ProgressRing::ProgressRing()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressRing);

    SetDefaultStyleKey(this);

    RegisterPropertyChangedCallback(winrt::Control::ForegroundProperty(), { this, &ProgressRing::OnForegroundPropertyChanged });
    RegisterPropertyChangedCallback(winrt::Control::BackgroundProperty(), { this, &ProgressRing::OnBackgroundPropertyChanged });
    
    SetValue(s_TemplateSettingsProperty, winrt::make<::ProgressRingTemplateSettings>());

    SizeChanged({ this, &ProgressRing::OnSizeChanged });
}

winrt::AutomationPeer ProgressRing::OnCreateAutomationPeer()
{
    return winrt::make<ProgressRingAutomationPeer>(*this);
}

void ProgressRing::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_player.set(GetTemplateChildT<winrt::AnimatedVisualPlayer>(s_IndeterminateAnimatedVisualPlayerName, controlProtected));
    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(s_LayoutRootName, controlProtected));

    SetAnimatedVisualPlayerSource();
    ChangeVisualState();
}

void ProgressRing::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    ApplyTemplateSettings();
}

void ProgressRing::OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (const auto foreground = Foreground().try_as<winrt::SolidColorBrush>())
    {
        foreground.RegisterPropertyChangedCallback(winrt::SolidColorBrush::ColorProperty(), { this, &ProgressRing::OnForegroundColorPropertyChanged });
    }

    OnForegroundColorPropertyChanged(nullptr, nullptr);
}

void ProgressRing::OnForegroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (auto&& player = m_player.get())
    {
        if (auto const progressRingIndeterminate = player.Source().try_as<AnimatedVisuals::ProgressRingIndeterminate>())
        {
            SetLottieForegroundColor(progressRingIndeterminate);
        }
    }
}

void ProgressRing::OnBackgroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (const auto background = Background().try_as<winrt::SolidColorBrush>())
    {
        background.RegisterPropertyChangedCallback(winrt::SolidColorBrush::ColorProperty(), { this, &ProgressRing::OnBackgroundColorPropertyChanged });
    }

    OnBackgroundColorPropertyChanged(nullptr, nullptr);
}

void ProgressRing::OnBackgroundColorPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    if (auto&& player = m_player.get())
    {
        if (auto const progressRingIndeterminate = player.Source().try_as<AnimatedVisuals::ProgressRingIndeterminate>())
        {
            SetLottieBackgroundColor(progressRingIndeterminate);
        }
    }
}

void ProgressRing::OnIsActivePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ChangeVisualState();
}

void ProgressRing::SetAnimatedVisualPlayerSource()
{
    if (auto&& player = m_player.get())
    {
        if (!player.Source())
        {
            player.Source(winrt::make<AnimatedVisuals::ProgressRingIndeterminate>());

            if (const auto progressRingIndeterminate = player.Source().try_as<AnimatedVisuals::ProgressRingIndeterminate>())
            {
                SetLottieForegroundColor(progressRingIndeterminate);
                SetLottieBackgroundColor(progressRingIndeterminate);
            }
        }
    }
}

void ProgressRing::SetLottieForegroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate)
{
    const auto compositor = winrt::Window::Current().Compositor();

    const auto foregroundColor = [foreground = Foreground().try_as<winrt::SolidColorBrush>()]()
    {
        if (foreground)
        {
            return foreground.Color();
        }
        else
        {
            // Default color fallback if Foreground() Brush does not contain SolidColorBrush with Color property.
            return SharedHelpers::FindInApplicationResources(s_DefaultForegroundThemeResourceName).as<winrt::SolidColorBrush>().Color();
        }
    }();

    progressRingIndeterminate->GetThemeProperties(compositor).InsertVector4(s_ForegroundName, SharedHelpers::RgbaColor(foregroundColor));
}

void ProgressRing::SetLottieBackgroundColor(winrt::impl::com_ref<AnimatedVisuals::ProgressRingIndeterminate> progressRingIndeterminate)
{
    const auto compositor = winrt::Window::Current().Compositor();

    const auto backgroundColor = [background = Background().try_as<winrt::SolidColorBrush>()]()
    {
        if (background)
        {
            return background.Color();
        }
        else
        {
            // Default color fallback if Background() Brush does not contain SolidColorBrush with Color property.
            return SharedHelpers::FindInApplicationResources(s_DefaultBackgroundThemeResourceName).as<winrt::SolidColorBrush>().Color();
        }
    }();

    progressRingIndeterminate->GetThemeProperties(compositor).InsertVector4(s_BackgroundName, SharedHelpers::RgbaColor(backgroundColor));
}

void ProgressRing::ChangeVisualState()
{
    if (IsActive())
    {
        winrt::VisualStateManager::GoToState(*this, L"Active", true);

        if (auto&& player = m_player.get())
        {
            const auto _ = player.PlayAsync(0, 1, true);
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Inactive", true);

        if (auto&& player = m_player.get())
        {
            player.Stop();
        }
    }
}

void ProgressRing::ApplyTemplateSettings()
{
    // TemplateSetting properties from WUXC for backwards compatibility.
    const auto templateSettings = winrt::get_self<::ProgressRingTemplateSettings>(TemplateSettings());

    const auto [width, diameterValue, anchorPoint] = [this]()
    {
        if (this->ActualWidth())
        {
            const float width = static_cast<float>(this->ActualWidth());

            const auto diameterAdditive = [width]()
            {
                if (width <= 40.0f)
                {
                    return 1.0f;
                }
                return 0.0f;
            }();

            const float diamaterValue = (width * 0.1f) + diameterAdditive;
            const float anchorPoint = (width * 0.5f) - diamaterValue;
            return std::make_tuple(width, diamaterValue, anchorPoint);
        }

        return std::make_tuple(0.0f, 0.0f, 0.0f);
    }();
  
    templateSettings->EllipseDiameter(diameterValue);

    const winrt::Thickness thicknessEllipseOffset = { 0, anchorPoint, 0, 0 };

    templateSettings->EllipseOffset(thicknessEllipseOffset);
    templateSettings->MaxSideLength(width);
}
