﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ColorSpectrum.h"

#include "ColorSpectrumAutomationPeer.h"
#include "SpectrumBrush.h"

using namespace std;

ColorSpectrum::ColorSpectrum()
{
    SetDefaultStyleKey(this);

    m_updatingColor = false;
    m_updatingHsvColor = false;
    m_isPointerOver = false;
    m_isPointerPressed = false;
    m_shouldShowLargeSelection = false;

    m_shapeFromLastBitmapCreation = Shape();
    m_componentsFromLastBitmapCreation = Components();
    m_imageWidthFromLastBitmapCreation = 0;
    m_imageHeightFromLastBitmapCreation = 0;
    m_minHueFromLastBitmapCreation = MinHue();
    m_maxHueFromLastBitmapCreation = MaxHue();
    m_minSaturationFromLastBitmapCreation = MinSaturation();
    m_maxSaturationFromLastBitmapCreation = MaxSaturation();
    m_minValueFromLastBitmapCreation = MinValue();
    m_maxValueFromLastBitmapCreation = MaxValue();

    Unloaded({ this, &ColorSpectrum::OnUnloaded });

    if (SharedHelpers::IsRS1OrHigher())
    {
        IsFocusEngagementEnabled(true);
    }
}

winrt::AutomationPeer ColorSpectrum::OnCreateAutomationPeer()
{
    return winrt::make<ColorSpectrumAutomationPeer>(*this);
}

void ColorSpectrum::OnApplyTemplate()
{
    winrt::IControlProtected thisAsControlProtected = *this;

    m_layoutRoot = GetTemplateChildT<winrt::Grid>(L"LayoutRoot", thisAsControlProtected);
    m_sizingGrid = GetTemplateChildT<winrt::Grid>(L"SizingGrid", thisAsControlProtected);
    m_spectrumRectangle = GetTemplateChildT<winrt::Rectangle>(L"SpectrumRectangle", thisAsControlProtected);
    m_spectrumEllipse = GetTemplateChildT<winrt::Ellipse>(L"SpectrumEllipse", thisAsControlProtected);
    m_spectrumOverlayRectangle = GetTemplateChildT<winrt::Rectangle>(L"SpectrumOverlayRectangle", thisAsControlProtected);
    m_spectrumOverlayEllipse = GetTemplateChildT<winrt::Ellipse>(L"SpectrumOverlayEllipse", thisAsControlProtected);
    m_inputTarget = GetTemplateChildT<winrt::FrameworkElement>(L"InputTarget", thisAsControlProtected);
    m_selectionEllipsePanel = GetTemplateChildT<winrt::Panel>(L"SelectionEllipsePanel", thisAsControlProtected);
    m_colorNameToolTip = GetTemplateChildT<winrt::ToolTip>(L"ColorNameToolTip", thisAsControlProtected);

    if (m_layoutRoot)
    {
        m_layoutRoot.SizeChanged({ this, &ColorSpectrum::OnLayoutRootSizeChanged });
    }

    if (m_inputTarget)
    {
        m_inputTarget.PointerEntered({ this, &ColorSpectrum::OnInputTargetPointerEntered });
        m_inputTarget.PointerExited({ this, &ColorSpectrum::OnInputTargetPointerExited });
        m_inputTarget.PointerPressed({ this, &ColorSpectrum::OnInputTargetPointerPressed });
        m_inputTarget.PointerMoved({ this, &ColorSpectrum::OnInputTargetPointerMoved });
        m_inputTarget.PointerReleased({ this, &ColorSpectrum::OnInputTargetPointerReleased });
    }

    if (m_colorNameToolTip && DownlevelHelper::ToDisplayNameExists())
    {
        m_colorNameToolTip.Content(box_value(winrt::ColorHelper::ToDisplayName(Color())));
    }

    if (m_selectionEllipsePanel)
    {
        m_selectionEllipsePanel.RegisterPropertyChangedCallback(winrt::FrameworkElement::FlowDirectionProperty(), { this, &ColorSpectrum::OnSelectionEllipseFlowDirectionChanged });
    }

    // If we haven't yet created our bitmaps, do so now.
    if (m_hsvValues.size() == 0)
    {
        CreateBitmapsAndColorMap();
    }

    UpdateEllipse();
    UpdateVisualState(false /* useTransitions */);
}

void ColorSpectrum::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if (args.Key() != winrt::VirtualKey::Left &&
        args.Key() != winrt::VirtualKey::Right &&
        args.Key() != winrt::VirtualKey::Up &&
        args.Key() != winrt::VirtualKey::Down)
    {
        __super::OnKeyDown(args);
        return;
    }

    bool isControlDown = (winrt::Window::Current().CoreWindow().GetKeyState(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    winrt::ColorPickerHsvChannel incrementChannel = winrt::ColorPickerHsvChannel::Hue;

    if (args.Key() == winrt::VirtualKey::Left ||
        args.Key() == winrt::VirtualKey::Right)
    {
        switch (Components())
        {
        case winrt::ColorSpectrumComponents::HueSaturation:
        case winrt::ColorSpectrumComponents::HueValue:
            incrementChannel = winrt::ColorPickerHsvChannel::Hue;
            break;

        case winrt::ColorSpectrumComponents::SaturationHue:
        case winrt::ColorSpectrumComponents::SaturationValue:
            incrementChannel = winrt::ColorPickerHsvChannel::Saturation;
            break;

        case winrt::ColorSpectrumComponents::ValueHue:
        case winrt::ColorSpectrumComponents::ValueSaturation:
            incrementChannel = winrt::ColorPickerHsvChannel::Value;
            break;
        }
    }
    else if (args.Key() == winrt::VirtualKey::Up ||
        args.Key() == winrt::VirtualKey::Down)
    {
        switch (Components())
        {
        case winrt::ColorSpectrumComponents::SaturationHue:
        case winrt::ColorSpectrumComponents::ValueHue:
            incrementChannel = winrt::ColorPickerHsvChannel::Hue;
            break;

        case winrt::ColorSpectrumComponents::HueSaturation:
        case winrt::ColorSpectrumComponents::ValueSaturation:
            incrementChannel = winrt::ColorPickerHsvChannel::Saturation;
            break;

        case winrt::ColorSpectrumComponents::HueValue:
        case winrt::ColorSpectrumComponents::SaturationValue:
            incrementChannel = winrt::ColorPickerHsvChannel::Value;
            break;
        }
    }

    double minBound = 0;
    double maxBound = 0;

    switch (incrementChannel)
    {
    case winrt::ColorPickerHsvChannel::Hue:
        minBound = MinHue();
        maxBound = MaxHue();
        break;

    case winrt::ColorPickerHsvChannel::Saturation:
        minBound = MinSaturation();
        maxBound = MaxSaturation();
        break;

    case winrt::ColorPickerHsvChannel::Value:
        minBound = MinValue();
        maxBound = MaxValue();
        break;
    }

    // The order of saturation and value in the spectrum is reversed - the max value is at the bottom while the min value is at the top -
    // so we want left and up to be lower for hue, but higher for saturation and value.
    // This will ensure that the icon always moves in the direction of the key press.
    IncrementDirection direction =
        (incrementChannel == winrt::ColorPickerHsvChannel::Hue && (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::Up)) ||
        (incrementChannel != winrt::ColorPickerHsvChannel::Hue && (args.Key() == winrt::VirtualKey::Right || args.Key() == winrt::VirtualKey::Down)) ?
        IncrementDirection::Lower :
        IncrementDirection::Higher;

    IncrementAmount amount = isControlDown ? IncrementAmount::Large : IncrementAmount::Small;

    winrt::float4 hsvColor = HsvColor();
    UpdateColor(IncrementColorChannel(Hsv(hsv::GetHue(hsvColor), hsv::GetSaturation(hsvColor), hsv::GetValue(hsvColor)), incrementChannel, direction, amount, true /* shouldWrap */, minBound, maxBound));
    args.Handled(true);
}

void ColorSpectrum::OnGotFocus(winrt::RoutedEventArgs const& /*e*/)
{
    // We only want to bother with the color name tool tip if we can provide color names.
    if (m_colorNameToolTip && DownlevelHelper::ToDisplayNameExists())
    {
        m_colorNameToolTip.IsOpen(true);
    }

    UpdateVisualState(true /* useTransitions */);
}

void ColorSpectrum::OnLostFocus(winrt::RoutedEventArgs const& /*e*/)
{
    // We only want to bother with the color name tool tip if we can provide color names.
    if (m_colorNameToolTip && DownlevelHelper::ToDisplayNameExists())
    {
        m_colorNameToolTip.IsOpen(false);
    }

    UpdateVisualState(true /* useTransitions */);
}

void ColorSpectrum::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_ColorProperty)
    {
        OnColorChanged(args);
    }
    else if (property == s_HsvColorProperty)
    {
        OnHsvColorChanged(args);
    }
    else if (property == s_MinHueProperty ||
        property == s_MaxHueProperty)
    {
        OnMinMaxHueChanged(args);
    }
    else if (property == s_MinSaturationProperty ||
        property == s_MaxSaturationProperty)
    {
        OnMinMaxSaturationChanged(args);
    }
    else if (property == s_MinValueProperty ||
        property == s_MaxValueProperty)
    {
        OnMinMaxValueChanged(args);
    }
    else if (property == s_ShapeProperty)
    {
        OnShapeChanged(args);
    }
    else if (property == s_ComponentsProperty)
    {
        OnComponentsChanged(args);
    }
}

void ColorSpectrum::OnColorChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    // If we're in the process of internally updating the color, then we don't want to respond to the Color property changing.
    if (!m_updatingColor)
    {
        winrt::Color color = Color();

        m_updatingHsvColor = true;
        Hsv newHsv = RgbToHsv(Rgb(color.R / 255.0, color.G / 255.0, color.B / 255.0));
        HsvColor(winrt::float4{ static_cast<float>(newHsv.h), static_cast<float>(newHsv.s), static_cast<float>(newHsv.v), static_cast<float>(color.A / 255.0) });
        m_updatingHsvColor = false;

        UpdateEllipse();
        UpdateBitmapSources();
    }

    m_oldColor = unbox_value<winrt::Color>(args.OldValue());
}

void ColorSpectrum::OnHsvColorChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    // If we're in the process of internally updating the HSV color, then we don't want to respond to the HsvColor property changing.
    if (!m_updatingHsvColor)
    {
        SetColor();
    }

    m_oldHsvColor = unbox_value<winrt::float4>(args.OldValue());
}

void ColorSpectrum::SetColor()
{
    winrt::float4 hsvColor = HsvColor();

    m_updatingColor = true;
    Rgb newRgb = HsvToRgb(Hsv(hsv::GetHue(hsvColor), hsv::GetSaturation(hsvColor), hsv::GetValue(hsvColor)));

    Color(ColorFromRgba(newRgb, hsv::GetAlpha(hsvColor)));

    m_updatingColor = false;

    UpdateEllipse();
    UpdateBitmapSources();
    RaiseColorChanged();
}

void ColorSpectrum::RaiseColorChanged()
{
    winrt::Color newColor = Color();

    if (m_oldColor.A != newColor.A ||
        m_oldColor.R != newColor.R ||
        m_oldColor.G != newColor.G ||
        m_oldColor.B != newColor.B)
    {
        auto colorChangedEventArgs = winrt::make_self<ColorChangedEventArgs>();

        colorChangedEventArgs->OldColor(m_oldColor);
        colorChangedEventArgs->NewColor(newColor);

        m_colorChangedEventSource(*this, *colorChangedEventArgs);

        if (m_colorNameToolTip && DownlevelHelper::ToDisplayNameExists())
        {
            m_colorNameToolTip.Content(box_value(winrt::ColorHelper::ToDisplayName(newColor)));
        }

        auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this);
        if (peer)
        {
            winrt::ColorSpectrumAutomationPeer colorSpectrumPeer = peer.as<winrt::ColorSpectrumAutomationPeer>();
            winrt::get_self<ColorSpectrumAutomationPeer>(colorSpectrumPeer)->RaisePropertyChangedEvent(m_oldColor, newColor, m_oldHsvColor, HsvColor());
        }
    }
}

void ColorSpectrum::OnMinMaxHueChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    int minHue = MinHue();
    int maxHue = MaxHue();

    if (minHue < 0 || minHue > 359)
    {
        throw winrt::hresult_invalid_argument(L"MinHue must be between 0 and 359.");
    }
    else if (maxHue < 0 || maxHue > 359)
    {
        throw winrt::hresult_invalid_argument(L"MaxHue must be between 0 and 359.");
    }

    winrt::ColorSpectrumComponents components = Components();

    // If hue is one of the axes in the spectrum bitmap, then we'll need to regenerate it
    // if the maximum or minimum value has changed.
    if (components != winrt::ColorSpectrumComponents::SaturationValue &&
        components != winrt::ColorSpectrumComponents::ValueSaturation)
    {
        CreateBitmapsAndColorMap();
    }
}

void ColorSpectrum::OnMinMaxSaturationChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    int minSaturation = MinSaturation();
    int maxSaturation = MaxSaturation();

    if (minSaturation < 0 || minSaturation > 100)
    {
        throw winrt::hresult_invalid_argument(L"MinSaturation must be between 0 and 100.");
    }
    else if (maxSaturation < 0 || maxSaturation > 100)
    {
        throw winrt::hresult_invalid_argument(L"MaxSaturation must be between 0 and 100.");
    }

    winrt::ColorSpectrumComponents components = Components();

    // If value is one of the axes in the spectrum bitmap, then we'll need to regenerate it
    // if the maximum or minimum value has changed.
    if (components != winrt::ColorSpectrumComponents::HueValue &&
        components != winrt::ColorSpectrumComponents::ValueHue)
    {
        CreateBitmapsAndColorMap();
    }
}

void ColorSpectrum::OnMinMaxValueChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    int minValue = MinValue();
    int maxValue = MaxValue();

    if (minValue < 0 || minValue > 100)
    {
        throw winrt::hresult_invalid_argument(L"MinValue must be between 0 and 100.");
    }
    else if (maxValue < 0 || maxValue > 100)
    {
        throw winrt::hresult_invalid_argument(L"MaxValue must be between 0 and 100.");
    }

    winrt::ColorSpectrumComponents components = Components();

    // If value is one of the axes in the spectrum bitmap, then we'll need to regenerate it
    // if the maximum or minimum value has changed.
    if (components != winrt::ColorSpectrumComponents::HueSaturation &&
        components != winrt::ColorSpectrumComponents::SaturationHue)
    {
        CreateBitmapsAndColorMap();
    }
}

void ColorSpectrum::OnShapeChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    CreateBitmapsAndColorMap();
}

void ColorSpectrum::OnComponentsChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    CreateBitmapsAndColorMap();
}

void ColorSpectrum::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // If we're in the middle of creating an image bitmap while being unloaded,
    // we'll want to synchronously cancel it so we don't have any asynchronous actions
    // lingering beyond our lifetime.
    CancelAsyncAction(m_createImageBitmapAction);
}

winrt::Rect ColorSpectrum::GetBoundingRectangle()
{
    winrt::Rect localRect{ 0, 0, 0, 0 };

    if (m_inputTarget)
    {
        localRect.Width = static_cast<float>(m_inputTarget.ActualWidth());
        localRect.Height = static_cast<float>(m_inputTarget.ActualHeight());
    }

    const auto globalBounds = TransformToVisual(nullptr).TransformBounds(localRect);
    return SharedHelpers::ConvertDipsToPhysical(*this, globalBounds);
}

void ColorSpectrum::UpdateVisualState(bool useTransitions)
{
    winrt::Control thisAsControl = *this;

    if (m_isPointerPressed)
    {
        winrt::VisualStateManager::GoToState(thisAsControl, m_shouldShowLargeSelection ? L"PressedLarge" : L"Pressed", useTransitions);
    }
    else if (m_isPointerOver)
    {
        winrt::VisualStateManager::GoToState(thisAsControl, L"PointerOver", useTransitions);
    }
    else
    {
        winrt::VisualStateManager::GoToState(thisAsControl, L"Normal", useTransitions);
    }

    winrt::VisualStateManager::GoToState(thisAsControl, m_shapeFromLastBitmapCreation == winrt::ColorSpectrumShape::Box ? L"BoxSelected" : L"RingSelected", useTransitions);
    winrt::VisualStateManager::GoToState(thisAsControl, SelectionEllipseShouldBeLight() ? L"SelectionEllipseLight" : L"SelectionEllipseDark", useTransitions);

    if (IsEnabled() && FocusState() != winrt::FocusState::Unfocused)
    {
        if (FocusState() == winrt::FocusState::Pointer)
        {
            winrt::VisualStateManager::GoToState(thisAsControl, L"PointerFocused", useTransitions);
        }
        else
        {
            winrt::VisualStateManager::GoToState(thisAsControl, L"Focused", useTransitions);
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(thisAsControl, L"Unfocused", useTransitions);
    }
}

void ColorSpectrum::UpdateColor(Hsv newHsv)
{
    m_updatingColor = true;
    m_updatingHsvColor = true;

    Rgb newRgb = HsvToRgb(newHsv);
    float alpha = hsv::GetAlpha(HsvColor());

    Color(ColorFromRgba(newRgb, alpha));
    HsvColor({ static_cast<float>(newHsv.h), static_cast<float>(newHsv.s), static_cast<float>(newHsv.v), alpha });

    UpdateEllipse();
    UpdateVisualState(true /* useTransitions */);

    m_updatingHsvColor = false;
    m_updatingColor = false;

    RaiseColorChanged();
}

void ColorSpectrum::UpdateColorFromPoint(const winrt::PointerPoint& point)
{
    // If we haven't initialized our HSV value array yet, then we should just ignore any user input -
    // we don't yet know what to do with it.
    if (m_hsvValues.size() == 0)
    {
        return;
    }

    double xPosition = point.Position().X;
    double yPosition = point.Position().Y;
    double radius = min(m_imageWidthFromLastBitmapCreation, m_imageHeightFromLastBitmapCreation) / 2;
    double distanceFromRadius = sqrt(pow(xPosition - radius, 2) + pow(yPosition - radius, 2));

    auto shape = Shape();

    // If the point is outside the circle, we should bring it back into the circle.
    if (distanceFromRadius > radius && shape == winrt::ColorSpectrumShape::Ring)
    {
        xPosition = (radius / distanceFromRadius) * (xPosition - radius) + radius;
        yPosition = (radius / distanceFromRadius) * (yPosition - radius) + radius;
    }

    // Now we need to find the index into the array of HSL values at each point in the spectrum m_image.
    int x = static_cast<int>(round(xPosition));
    int y = static_cast<int>(round(yPosition));
    int width = static_cast<int>(round(m_imageWidthFromLastBitmapCreation));

    if (x < 0)
    {
        x = 0;
    }
    else if (x >= m_imageWidthFromLastBitmapCreation)
    {
        x = static_cast<int>(round(m_imageWidthFromLastBitmapCreation)) - 1;
    }

    if (y < 0)
    {
        y = 0;
    }
    else if (y >= m_imageHeightFromLastBitmapCreation)
    {
        y = static_cast<int>(round(m_imageHeightFromLastBitmapCreation)) - 1;
    }

    // The gradient image contains two dimensions of HSL information, but not the third.
    // We should keep the third where it already was.
    Hsv hsvAtPoint = m_hsvValues[y * width + x];

    auto components = Components();
    auto hsvColor = HsvColor();

    switch (components)
    {
    case winrt::ColorSpectrumComponents::HueValue:
    case winrt::ColorSpectrumComponents::ValueHue:
        hsvAtPoint.s = hsv::GetSaturation(hsvColor);
        break;

    case winrt::ColorSpectrumComponents::HueSaturation:
    case winrt::ColorSpectrumComponents::SaturationHue:
        hsvAtPoint.v = hsv::GetValue(hsvColor);
        break;

    case winrt::ColorSpectrumComponents::ValueSaturation:
    case winrt::ColorSpectrumComponents::SaturationValue:
        hsvAtPoint.h = hsv::GetHue(hsvColor);
        break;
    }

    UpdateColor(hsvAtPoint);
}

void ColorSpectrum::UpdateEllipse()
{
    if (!m_selectionEllipsePanel)
    {
        return;
    }

    // If we don't have an image size yet, we shouldn't be showing the ellipse.
    if (m_imageWidthFromLastBitmapCreation == 0 ||
        m_imageHeightFromLastBitmapCreation == 0)
    {
        m_selectionEllipsePanel.Visibility(winrt::Visibility::Collapsed);
        return;
    }
    else
    {
        m_selectionEllipsePanel.Visibility(winrt::Visibility::Visible);
    }

    double xPosition;
    double yPosition;

    winrt::float4 hsvColor = HsvColor();

    hsv::SetHue(hsvColor, clamp(hsv::GetHue(hsvColor), static_cast<float>(m_minHueFromLastBitmapCreation), static_cast<float> (m_maxHueFromLastBitmapCreation)));
    hsv::SetSaturation(hsvColor, clamp(hsv::GetSaturation(hsvColor), m_minSaturationFromLastBitmapCreation / 100.0f, m_maxSaturationFromLastBitmapCreation / 100.0f));
    hsv::SetValue(hsvColor, clamp(hsv::GetValue(hsvColor), m_minValueFromLastBitmapCreation / 100.0f, m_maxValueFromLastBitmapCreation / 100.0f));

    if (m_shapeFromLastBitmapCreation == winrt::ColorSpectrumShape::Box)
    {
        double xPercent = 0;
        double yPercent = 0;

        double hPercent = (hsv::GetHue(hsvColor) - m_minHueFromLastBitmapCreation) / (m_maxHueFromLastBitmapCreation - m_minHueFromLastBitmapCreation);
        double sPercent = (hsv::GetSaturation(hsvColor) * 100.0 - m_minSaturationFromLastBitmapCreation) / (m_maxSaturationFromLastBitmapCreation - m_minSaturationFromLastBitmapCreation);
        double vPercent = (hsv::GetValue(hsvColor) * 100.0 - m_minValueFromLastBitmapCreation) / (m_maxValueFromLastBitmapCreation - m_minValueFromLastBitmapCreation);

        // In the case where saturation was an axis in the spectrum with hue, or value is an axis, full stop,
        // we inverted the direction of that axis in order to put more hue on the outside of the ring,
        // so we need to do similarly here when positioning the ellipse.
        if (m_componentsFromLastBitmapCreation == winrt::ColorSpectrumComponents::HueSaturation ||
            m_componentsFromLastBitmapCreation == winrt::ColorSpectrumComponents::SaturationHue)
        {
            sPercent = 1 - sPercent;
        }
        else
        {
            vPercent = 1 - vPercent;
        }

        switch (m_componentsFromLastBitmapCreation)
        {
        case winrt::ColorSpectrumComponents::HueValue:
            xPercent = hPercent;
            yPercent = vPercent;
            break;

        case winrt::ColorSpectrumComponents::HueSaturation:
            xPercent = hPercent;
            yPercent = sPercent;
            break;

        case winrt::ColorSpectrumComponents::ValueHue:
            xPercent = vPercent;
            yPercent = hPercent;
            break;

        case winrt::ColorSpectrumComponents::ValueSaturation:
            xPercent = vPercent;
            yPercent = sPercent;
            break;

        case winrt::ColorSpectrumComponents::SaturationHue:
            xPercent = sPercent;
            yPercent = hPercent;
            break;

        case winrt::ColorSpectrumComponents::SaturationValue:
            xPercent = sPercent;
            yPercent = vPercent;
            break;
        }

        xPosition = m_imageWidthFromLastBitmapCreation * xPercent;
        yPosition = m_imageHeightFromLastBitmapCreation * yPercent;
    }
    else
    {
        double thetaValue = 0;
        double rValue = 0;

        double hThetaValue =
            m_maxHueFromLastBitmapCreation != m_minHueFromLastBitmapCreation ?
            360 * (hsv::GetHue(hsvColor) - m_minHueFromLastBitmapCreation) / (m_maxHueFromLastBitmapCreation - m_minHueFromLastBitmapCreation) :
            0;
        double sThetaValue =
            m_maxSaturationFromLastBitmapCreation != m_minSaturationFromLastBitmapCreation ?
            360 * (hsv::GetSaturation(hsvColor) * 100.0 - m_minSaturationFromLastBitmapCreation) / (m_maxSaturationFromLastBitmapCreation - m_minSaturationFromLastBitmapCreation) :
            0;
        double vThetaValue =
            m_maxValueFromLastBitmapCreation != m_minValueFromLastBitmapCreation ?
            360 * (hsv::GetValue(hsvColor) * 100.0 - m_minValueFromLastBitmapCreation) / (m_maxValueFromLastBitmapCreation - m_minValueFromLastBitmapCreation) :
            0;
        double hRValue = m_maxHueFromLastBitmapCreation != m_minHueFromLastBitmapCreation ?
            (hsv::GetHue(hsvColor) - m_minHueFromLastBitmapCreation) / (m_maxHueFromLastBitmapCreation - m_minHueFromLastBitmapCreation) - 1 :
            0;
        double sRValue = m_maxSaturationFromLastBitmapCreation != m_minSaturationFromLastBitmapCreation ?
            (hsv::GetSaturation(hsvColor) * 100.0 - m_minSaturationFromLastBitmapCreation) / (m_maxSaturationFromLastBitmapCreation - m_minSaturationFromLastBitmapCreation) - 1 :
            0;
        double vRValue = m_maxValueFromLastBitmapCreation != m_minValueFromLastBitmapCreation ?
            (hsv::GetValue(hsvColor) * 100.0 - m_minValueFromLastBitmapCreation) / (m_maxValueFromLastBitmapCreation - m_minValueFromLastBitmapCreation) - 1 :
            0;

        // In the case where saturation was an axis in the spectrum with hue, or value is an axis, full stop,
        // we inverted the direction of that axis in order to put more hue on the outside of the ring,
        // so we need to do similarly here when positioning the ellipse.
        if (m_componentsFromLastBitmapCreation == winrt::ColorSpectrumComponents::HueSaturation ||
            m_componentsFromLastBitmapCreation == winrt::ColorSpectrumComponents::ValueHue)
        {
            sThetaValue = 360 - sThetaValue;
            sRValue = -sRValue - 1;
        }
        else
        {
            vThetaValue = 360 - vThetaValue;
            vRValue = -vRValue - 1;
        }

        switch (m_componentsFromLastBitmapCreation)
        {
        case winrt::ColorSpectrumComponents::HueValue:
            thetaValue = hThetaValue;
            rValue = vRValue;
            break;

        case winrt::ColorSpectrumComponents::HueSaturation:
            thetaValue = hThetaValue;
            rValue = sRValue;
            break;

        case winrt::ColorSpectrumComponents::ValueHue:
            thetaValue = vThetaValue;
            rValue = hRValue;
            break;

        case winrt::ColorSpectrumComponents::ValueSaturation:
            thetaValue = vThetaValue;
            rValue = sRValue;
            break;

        case winrt::ColorSpectrumComponents::SaturationHue:
            thetaValue = sThetaValue;
            rValue = hRValue;
            break;

        case winrt::ColorSpectrumComponents::SaturationValue:
            thetaValue = sThetaValue;
            rValue = vRValue;
            break;
        }

        double radius = min(m_imageWidthFromLastBitmapCreation, m_imageHeightFromLastBitmapCreation) / 2;

        xPosition = (cos((thetaValue * M_PI / 180) + M_PI) * radius * rValue) + radius;
        yPosition = (sin((thetaValue * M_PI / 180) + M_PI) * radius * rValue) + radius;
    }

    winrt::Canvas::SetLeft(m_selectionEllipsePanel, xPosition - (m_selectionEllipsePanel.Width() / 2));
    winrt::Canvas::SetTop(m_selectionEllipsePanel, yPosition - (m_selectionEllipsePanel.Height() / 2));

    // We only want to bother with the color name tool tip if we can provide color names.
    if (m_colorNameToolTip && DownlevelHelper::ToDisplayNameExists())
    {
        // ToolTip doesn't currently provide any way to re-run its placement logic if its placement target moves,
        // so toggling IsEnabled induces it to do that without incurring any visual glitches.
        m_colorNameToolTip.IsEnabled(false);
        m_colorNameToolTip.IsEnabled(true);
    }

    UpdateVisualState(true /* useTransitions */);
}

void ColorSpectrum::OnLayoutRootSizeChanged(winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& /*args*/)
{
    // We want ColorSpectrum to always be a square, so we'll take the smaller of the dimensions
    // and size the sizing grid to that.
    CreateBitmapsAndColorMap();
}

void ColorSpectrum::OnInputTargetPointerEntered(winrt::IInspectable const& /*sender*/, winrt::PointerRoutedEventArgs const& args)
{
    m_isPointerOver = true;
    UpdateVisualState(true /* useTransitions*/);
    args.Handled(true);
}

void ColorSpectrum::OnInputTargetPointerExited(winrt::IInspectable const& /*sender*/, winrt::PointerRoutedEventArgs const& args)
{
    m_isPointerOver = false;
    UpdateVisualState(true /* useTransitions*/);
    args.Handled(true);
}

void ColorSpectrum::OnInputTargetPointerPressed(winrt::IInspectable const& /*sender*/, winrt::PointerRoutedEventArgs const& args)
{
    Focus(winrt::FocusState::Pointer);

    m_isPointerPressed = true;
    m_shouldShowLargeSelection =
        args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Pen ||
        args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Touch;

    m_inputTarget.CapturePointer(args.Pointer());
    UpdateColorFromPoint(args.GetCurrentPoint(m_inputTarget));
    UpdateVisualState(true /* useTransitions*/);
    UpdateEllipse();

    args.Handled(true);
}

void ColorSpectrum::OnInputTargetPointerMoved(winrt::IInspectable const& /*sender*/, winrt::PointerRoutedEventArgs const& args)
{
    if (!m_isPointerPressed)
    {
        return;
    }

    UpdateColorFromPoint(args.GetCurrentPoint(m_inputTarget));
    args.Handled(true);
}

void ColorSpectrum::OnInputTargetPointerReleased(winrt::IInspectable const& /*sender*/, winrt::PointerRoutedEventArgs const& args)
{
    m_isPointerPressed = false;
    m_shouldShowLargeSelection = false;

    m_inputTarget.ReleasePointerCapture(args.Pointer());
    UpdateVisualState(true /* useTransitions*/);
    UpdateEllipse();

    args.Handled(true);
}

void ColorSpectrum::OnSelectionEllipseFlowDirectionChanged(winrt::DependencyObject const& /*o*/, winrt::DependencyProperty const& /*p*/)
{
    UpdateEllipse();
}

void ColorSpectrum::CreateBitmapsAndColorMap()
{
    if (!m_layoutRoot ||
        !m_sizingGrid ||
        !m_inputTarget ||
        !m_spectrumRectangle ||
        !m_spectrumEllipse ||
        !m_spectrumOverlayRectangle ||
        !m_spectrumOverlayEllipse ||
        SharedHelpers::IsInDesignMode())
    {
        return;
    }

    double minDimension = min(m_layoutRoot.ActualWidth(), m_layoutRoot.ActualHeight());

    if (minDimension == 0)
    {
        return;
    }

    m_sizingGrid.Width(minDimension);
    m_sizingGrid.Height(minDimension);

    if (m_sizingGrid.Clip())
    {
        m_sizingGrid.Clip().Rect({ 0, 0, static_cast<float>(minDimension), static_cast<float>(minDimension) });
    }

    m_inputTarget.Width(minDimension);
    m_inputTarget.Height(minDimension);
    m_spectrumRectangle.Width(minDimension);
    m_spectrumRectangle.Height(minDimension);
    m_spectrumEllipse.Width(minDimension);
    m_spectrumEllipse.Height(minDimension);
    m_spectrumOverlayRectangle.Width(minDimension);
    m_spectrumOverlayRectangle.Height(minDimension);
    m_spectrumOverlayEllipse.Width(minDimension);
    m_spectrumOverlayEllipse.Height(minDimension);

    winrt::float4 hsvColor = HsvColor();
    int minHue = MinHue();
    int maxHue = MaxHue();
    int minSaturation = MinSaturation();
    int maxSaturation = MaxSaturation();
    int minValue = MinValue();
    int maxValue = MaxValue();
    winrt::ColorSpectrumShape shape = Shape();
    winrt::ColorSpectrumComponents components = Components();

    // If min >= max, then by convention, min is the only number that a property can have.
    if (minHue >= maxHue)
    {
        maxHue = minHue;
    }

    if (minSaturation >= maxSaturation)
    {
        maxSaturation = minSaturation;
    }

    if (minValue >= maxValue)
    {
        maxValue = minValue;
    }

    Hsv hsv = { hsv::GetHue(hsvColor), hsv::GetSaturation(hsvColor), hsv::GetValue(hsvColor) };

    // The middle 4 are only needed and used in the case of hue as the third dimension.
    // Saturation and luminosity need only a min and max.
    shared_ptr<vector<::byte>> bgraMinPixelData = make_shared<vector<::byte>>();
    shared_ptr<vector<::byte>> bgraMiddle1PixelData = make_shared<vector<::byte>>();
    shared_ptr<vector<::byte>> bgraMiddle2PixelData = make_shared<vector<::byte>>();
    shared_ptr<vector<::byte>> bgraMiddle3PixelData = make_shared<vector<::byte>>();
    shared_ptr<vector<::byte>> bgraMiddle4PixelData = make_shared<vector<::byte>>();
    shared_ptr<vector<::byte>> bgraMaxPixelData = make_shared<vector<::byte>>();
    shared_ptr<vector<Hsv>> newHsvValues = make_shared<vector<Hsv>>();

    auto pixelCount = static_cast<size_t>(round(minDimension) * round(minDimension));
    size_t pixelDataSize = pixelCount * 4;
    bgraMinPixelData->reserve(pixelDataSize);

    // We'll only save pixel data for the middle bitmaps if our third dimension is hue.
    if (components == winrt::ColorSpectrumComponents::ValueSaturation ||
        components == winrt::ColorSpectrumComponents::SaturationValue)
    {
        bgraMiddle1PixelData->reserve(pixelDataSize);
        bgraMiddle2PixelData->reserve(pixelDataSize);
        bgraMiddle3PixelData->reserve(pixelDataSize);
        bgraMiddle4PixelData->reserve(pixelDataSize);
    }

    bgraMaxPixelData->reserve(pixelDataSize);
    newHsvValues->reserve(pixelCount);

    int minDimensionInt = static_cast<int>(round(minDimension));
    winrt::WorkItemHandler workItemHandler(
        [minDimensionInt, hsv, minHue, maxHue, minSaturation, maxSaturation, minValue, maxValue, shape, components,
        bgraMinPixelData, bgraMiddle1PixelData, bgraMiddle2PixelData, bgraMiddle3PixelData, bgraMiddle4PixelData, bgraMaxPixelData, newHsvValues]
    (winrt::IAsyncAction workItem)
        {
            // As the user perceives it, every time the third dimension not represented in the ColorSpectrum changes,
            // the ColorSpectrum will visually change to accommodate that value.  For example, if the ColorSpectrum handles hue and luminosity,
            // and the saturation externally goes from 1.0 to 0.5, then the ColorSpectrum will visually change to look more washed out
            // to represent that third dimension's new value.
            // Internally, however, we don't want to regenerate the ColorSpectrum bitmap every single time this happens, since that's very expensive.
            // In order to make it so that we don't have to, we implement an optimization where, rather than having only one bitmap,
            // we instead have multiple that we blend together using opacity to create the effect that we want.
            // In the case where the third dimension is saturation or luminosity, we only need two: one bitmap at the minimum value
            // of the third dimension, and one bitmap at the maximum.  Then we set the second's opacity at whatever the value of
            // the third dimension is - e.g., a saturation of 0.5 implies an opacity of 50%.
            // In the case where the third dimension is hue, we need six: one bitmap corresponding to red, yellow, green, cyan, blue, and purple.
            // We'll then blend between whichever colors our hue exists between - e.g., an orange color would use red and yellow with an opacity of 50%.
            // This optimization does incur slightly more startup time initially since we have to generate multiple bitmaps at once instead of only one,
            // but the running time savings after that are *huge* when we can just set an opacity instead of generating a brand new bitmap.
            if (shape == winrt::ColorSpectrumShape::Box)
            {
                for (int x = minDimensionInt - 1; x >= 0; --x)
                {
                    for (int y = minDimensionInt - 1; y >= 0; --y)
                    {
                        if (workItem.Status() == winrt::AsyncStatus::Canceled)
                        {
                            break;
                        }

                        ColorSpectrum::FillPixelForBox(
                            x, y, hsv, minDimensionInt, components, minHue, maxHue, minSaturation, maxSaturation, minValue, maxValue,
                            bgraMinPixelData, bgraMiddle1PixelData, bgraMiddle2PixelData, bgraMiddle3PixelData, bgraMiddle4PixelData, bgraMaxPixelData,
                            newHsvValues);
                    }
                }
            }
            else
            {
                for (int y = 0; y < minDimensionInt; ++y)
                {
                    for (int x = 0; x < minDimensionInt; ++x)
                    {
                        if (workItem.Status() == winrt::AsyncStatus::Canceled)
                        {
                            break;
                        }

                        ColorSpectrum::FillPixelForRing(
                            x, y, minDimensionInt / 2.0, hsv, components, minHue, maxHue, minSaturation, maxSaturation, minValue, maxValue,
                            bgraMinPixelData, bgraMiddle1PixelData, bgraMiddle2PixelData, bgraMiddle3PixelData, bgraMiddle4PixelData, bgraMaxPixelData,
                            newHsvValues);
                    }
                }
            }
        });

    if (m_createImageBitmapAction)
    {
        m_createImageBitmapAction.Cancel();
    }

    m_createImageBitmapAction = winrt::ThreadPool::RunAsync(workItemHandler);
    auto strongThis = get_strong();
    m_createImageBitmapAction.Completed(winrt::AsyncActionCompletedHandler(
        [strongThis, minDimension, components, bgraMinPixelData, bgraMiddle1PixelData, bgraMiddle2PixelData, bgraMiddle3PixelData, bgraMiddle4PixelData, bgraMaxPixelData, newHsvValues]
    (winrt::IAsyncAction asyncInfo, winrt::AsyncStatus asyncStatus)
    {
        if (asyncStatus != winrt::AsyncStatus::Completed)
        {
            return;
        }

        strongThis->m_createImageBitmapAction = nullptr;

        strongThis->m_dispatcherHelper.RunAsync(
            [strongThis, minDimension, bgraMinPixelData, bgraMiddle1PixelData, bgraMiddle2PixelData, bgraMiddle3PixelData, bgraMiddle4PixelData, bgraMaxPixelData, newHsvValues]()
        {
            int pixelWidth = static_cast<int>(round(minDimension));
            int pixelHeight = static_cast<int>(round(minDimension));

            winrt::ColorSpectrumComponents components = strongThis->Components();

            if (SharedHelpers::IsRS2OrHigher())
            {
                winrt::LoadedImageSurface minSurface = CreateSurfaceFromPixelData(pixelWidth, pixelHeight, bgraMinPixelData);
                winrt::LoadedImageSurface maxSurface = CreateSurfaceFromPixelData(pixelWidth, pixelHeight, bgraMaxPixelData);

                switch (components)
                {
                case winrt::ColorSpectrumComponents::HueValue:
                case winrt::ColorSpectrumComponents::ValueHue:
                    strongThis->m_saturationMinimumSurface = minSurface;
                    strongThis->m_saturationMaximumSurface = maxSurface;
                    break;
                case winrt::ColorSpectrumComponents::HueSaturation:
                case winrt::ColorSpectrumComponents::SaturationHue:
                    strongThis->m_valueSurface = maxSurface;
                    break;
                case winrt::ColorSpectrumComponents::ValueSaturation:
                case winrt::ColorSpectrumComponents::SaturationValue:
                    strongThis->m_hueRedSurface = minSurface;
                    strongThis->m_hueYellowSurface = CreateSurfaceFromPixelData(pixelWidth, pixelHeight, bgraMiddle1PixelData);
                    strongThis->m_hueGreenSurface = CreateSurfaceFromPixelData(pixelWidth, pixelHeight, bgraMiddle2PixelData);
                    strongThis->m_hueCyanSurface = CreateSurfaceFromPixelData(pixelWidth, pixelHeight, bgraMiddle3PixelData);
                    strongThis->m_hueBlueSurface = CreateSurfaceFromPixelData(pixelWidth, pixelHeight, bgraMiddle4PixelData);
                    strongThis->m_huePurpleSurface = maxSurface;
                    break;
                }
            }
            else
            {
                winrt::WriteableBitmap minBitmap = CreateBitmapFromPixelData(pixelWidth, pixelHeight, bgraMinPixelData);
                winrt::WriteableBitmap maxBitmap = CreateBitmapFromPixelData(pixelWidth, pixelHeight, bgraMaxPixelData);

                switch (components)
                {
                case winrt::ColorSpectrumComponents::HueValue:
                case winrt::ColorSpectrumComponents::ValueHue:
                    strongThis->m_saturationMinimumBitmap = minBitmap;
                    strongThis->m_saturationMaximumBitmap = maxBitmap;
                    break;
                case winrt::ColorSpectrumComponents::HueSaturation:
                case winrt::ColorSpectrumComponents::SaturationHue:
                    strongThis->m_valueBitmap = maxBitmap;
                    break;
                case winrt::ColorSpectrumComponents::ValueSaturation:
                case winrt::ColorSpectrumComponents::SaturationValue:
                    strongThis->m_hueRedBitmap = minBitmap;
                    strongThis->m_hueYellowBitmap = CreateBitmapFromPixelData(pixelWidth, pixelHeight, bgraMiddle1PixelData);
                    strongThis->m_hueGreenBitmap = CreateBitmapFromPixelData(pixelWidth, pixelHeight, bgraMiddle2PixelData);
                    strongThis->m_hueCyanBitmap = CreateBitmapFromPixelData(pixelWidth, pixelHeight, bgraMiddle3PixelData);
                    strongThis->m_hueBlueBitmap = CreateBitmapFromPixelData(pixelWidth, pixelHeight, bgraMiddle4PixelData);
                    strongThis->m_huePurpleBitmap = maxBitmap;
                    break;
                }
            }

            strongThis->m_shapeFromLastBitmapCreation = strongThis->Shape();
            strongThis->m_componentsFromLastBitmapCreation = strongThis->Components();
            strongThis->m_imageWidthFromLastBitmapCreation = minDimension;
            strongThis->m_imageHeightFromLastBitmapCreation = minDimension;
            strongThis->m_minHueFromLastBitmapCreation = strongThis->MinHue();
            strongThis->m_maxHueFromLastBitmapCreation = strongThis->MaxHue();
            strongThis->m_minSaturationFromLastBitmapCreation = strongThis->MinSaturation();
            strongThis->m_maxSaturationFromLastBitmapCreation = strongThis->MaxSaturation();
            strongThis->m_minValueFromLastBitmapCreation = strongThis->MinValue();
            strongThis->m_maxValueFromLastBitmapCreation = strongThis->MaxValue();

            strongThis->m_hsvValues = *newHsvValues;

            strongThis->UpdateBitmapSources();
            strongThis->UpdateEllipse();
        });
    }));
}

void ColorSpectrum::FillPixelForBox(
    double x,
    double y,
    const Hsv &baseHsv,
    double minDimension,
    winrt::ColorSpectrumComponents components,
    double minHue,
    double maxHue,
    double minSaturation,
    double maxSaturation,
    double minValue,
    double maxValue,
    shared_ptr<vector<::byte>> bgraMinPixelData,
    shared_ptr<vector<::byte>> bgraMiddle1PixelData,
    shared_ptr<vector<::byte>> bgraMiddle2PixelData,
    shared_ptr<vector<::byte>> bgraMiddle3PixelData,
    shared_ptr<vector<::byte>> bgraMiddle4PixelData,
    shared_ptr<vector<::byte>> bgraMaxPixelData,
    shared_ptr<vector<Hsv>> newHsvValues)
{
    double hMin = minHue;
    double hMax = maxHue;
    double sMin = minSaturation / 100.0;
    double sMax = maxSaturation / 100.0;
    double vMin = minValue / 100.0;
    double vMax = maxValue / 100.0;

    Hsv hsvMin = baseHsv;
    Hsv hsvMiddle1 = baseHsv;
    Hsv hsvMiddle2 = baseHsv;
    Hsv hsvMiddle3 = baseHsv;
    Hsv hsvMiddle4 = baseHsv;
    Hsv hsvMax = baseHsv;

    double xPercent = (minDimension - 1 - x) / (minDimension - 1);
    double yPercent = (minDimension - 1 - y) / (minDimension - 1);

    switch (components)
    {
    case winrt::ColorSpectrumComponents::HueValue:
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + yPercent * (hMax - hMin);
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + xPercent * (vMax - vMin);
        hsvMin.s = 0;
        hsvMax.s = 1;
        break;

    case winrt::ColorSpectrumComponents::HueSaturation:
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + yPercent * (hMax - hMin);
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + xPercent * (sMax - sMin);
        hsvMin.v = 0;
        hsvMax.v = 1;
        break;

    case winrt::ColorSpectrumComponents::ValueHue:
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + yPercent * (vMax - vMin);
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + xPercent * (hMax - hMin);
        hsvMin.s = 0;
        hsvMax.s = 1;
        break;

    case winrt::ColorSpectrumComponents::ValueSaturation:
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + yPercent * (vMax - vMin);
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + xPercent * (sMax - sMin);
        hsvMin.h = 0;
        hsvMiddle1.h = 60;
        hsvMiddle2.h = 120;
        hsvMiddle3.h = 180;
        hsvMiddle4.h = 240;
        hsvMax.h = 300;
        break;

    case winrt::ColorSpectrumComponents::SaturationHue:
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + yPercent * (sMax - sMin);
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + xPercent * (hMax - hMin);
        hsvMin.v = 0;
        hsvMax.v = 1;
        break;

    case winrt::ColorSpectrumComponents::SaturationValue:
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + yPercent * (sMax - sMin);
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + xPercent * (vMax - vMin);
        hsvMin.h = 0;
        hsvMiddle1.h = 60;
        hsvMiddle2.h = 120;
        hsvMiddle3.h = 180;
        hsvMiddle4.h = 240;
        hsvMax.h = 300;
        break;
    }

    // If saturation is an axis in the spectrum with hue, or value is an axis, then we want
    // that axis to go from maximum at the top to minimum at the bottom,
    // or maximum at the outside to minimum at the inside in the case of the ring configuration,
    // so we'll invert the number before assigning the HSL value to the array.
    // Otherwise, we'll have a very narrow section in the middle that actually has meaningful hue
    // in the case of the ring configuration.
    if (components == winrt::ColorSpectrumComponents::HueSaturation ||
        components == winrt::ColorSpectrumComponents::SaturationHue)
    {
        hsvMin.s = sMax - hsvMin.s + sMin;
        hsvMiddle1.s = sMax - hsvMiddle1.s + sMin;
        hsvMiddle2.s = sMax - hsvMiddle2.s + sMin;
        hsvMiddle3.s = sMax - hsvMiddle3.s + sMin;
        hsvMiddle4.s = sMax - hsvMiddle4.s + sMin;
        hsvMax.s = sMax - hsvMax.s + sMin;
    }
    else
    {
        hsvMin.v = vMax - hsvMin.v + vMin;
        hsvMiddle1.v = vMax - hsvMiddle1.v + vMin;
        hsvMiddle2.v = vMax - hsvMiddle2.v + vMin;
        hsvMiddle3.v = vMax - hsvMiddle3.v + vMin;
        hsvMiddle4.v = vMax - hsvMiddle4.v + vMin;
        hsvMax.v = vMax - hsvMax.v + vMin;
    }

    newHsvValues->push_back(hsvMin);

    Rgb rgbMin = HsvToRgb(hsvMin);
    bgraMinPixelData->push_back(static_cast<::byte>(round(rgbMin.b * 255))); // b
    bgraMinPixelData->push_back(static_cast<::byte>(round(rgbMin.g * 255))); // g
    bgraMinPixelData->push_back(static_cast<::byte>(round(rgbMin.r * 255))); // r
    bgraMinPixelData->push_back(255); // a - ignored

    // We'll only save pixel data for the middle bitmaps if our third dimension is hue.
    if (components == winrt::ColorSpectrumComponents::ValueSaturation ||
        components == winrt::ColorSpectrumComponents::SaturationValue)
    {
        Rgb rgbMiddle1 = HsvToRgb(hsvMiddle1);
        bgraMiddle1PixelData->push_back(static_cast<::byte>(round(rgbMiddle1.b * 255))); // b
        bgraMiddle1PixelData->push_back(static_cast<::byte>(round(rgbMiddle1.g * 255))); // g
        bgraMiddle1PixelData->push_back(static_cast<::byte>(round(rgbMiddle1.r * 255))); // r
        bgraMiddle1PixelData->push_back(255); // a - ignored

        Rgb rgbMiddle2 = HsvToRgb(hsvMiddle2);
        bgraMiddle2PixelData->push_back(static_cast<::byte>(round(rgbMiddle2.b * 255))); // b
        bgraMiddle2PixelData->push_back(static_cast<::byte>(round(rgbMiddle2.g * 255))); // g
        bgraMiddle2PixelData->push_back(static_cast<::byte>(round(rgbMiddle2.r * 255))); // r
        bgraMiddle2PixelData->push_back(255); // a - ignored

        Rgb rgbMiddle3 = HsvToRgb(hsvMiddle3);
        bgraMiddle3PixelData->push_back(static_cast<::byte>(round(rgbMiddle3.b * 255))); // b
        bgraMiddle3PixelData->push_back(static_cast<::byte>(round(rgbMiddle3.g * 255))); // g
        bgraMiddle3PixelData->push_back(static_cast<::byte>(round(rgbMiddle3.r * 255))); // r
        bgraMiddle3PixelData->push_back(255); // a - ignored

        Rgb rgbMiddle4 = HsvToRgb(hsvMiddle4);
        bgraMiddle4PixelData->push_back(static_cast<::byte>(round(rgbMiddle4.b * 255))); // b
        bgraMiddle4PixelData->push_back(static_cast<::byte>(round(rgbMiddle4.g * 255))); // g
        bgraMiddle4PixelData->push_back(static_cast<::byte>(round(rgbMiddle4.r * 255))); // r
        bgraMiddle4PixelData->push_back(255); // a - ignored
    }

    Rgb rgbMax = HsvToRgb(hsvMax);
    bgraMaxPixelData->push_back(static_cast<::byte>(round(rgbMax.b * 255))); // b
    bgraMaxPixelData->push_back(static_cast<::byte>(round(rgbMax.g * 255))); // g
    bgraMaxPixelData->push_back(static_cast<::byte>(round(rgbMax.r * 255))); // r
    bgraMaxPixelData->push_back(255); // a - ignored
}

void ColorSpectrum::FillPixelForRing(
    double x,
    double y,
    double radius,
    const Hsv &baseHsv,
    winrt::ColorSpectrumComponents components,
    double minHue,
    double maxHue,
    double minSaturation,
    double maxSaturation,
    double minValue,
    double maxValue,
    shared_ptr<vector<::byte>> bgraMinPixelData,
    shared_ptr<vector<::byte>> bgraMiddle1PixelData,
    shared_ptr<vector<::byte>> bgraMiddle2PixelData,
    shared_ptr<vector<::byte>> bgraMiddle3PixelData,
    shared_ptr<vector<::byte>> bgraMiddle4PixelData,
    shared_ptr<vector<::byte>> bgraMaxPixelData,
    shared_ptr<vector<Hsv>> newHsvValues)
{
    double hMin = minHue;
    double hMax = maxHue;
    double sMin = minSaturation / 100.0;
    double sMax = maxSaturation / 100.0;
    double vMin = minValue / 100.0;
    double vMax = maxValue / 100.0;

    double distanceFromRadius = sqrt(pow(x - radius, 2) + pow(y - radius, 2));

    double xToUse = x;
    double yToUse = y;

    // If we're outside the ring, then we want the pixel to appear as blank.
    // However, to avoid issues with rounding errors, we'll act as though this point
    // is on the edge of the ring for the purposes of returning an HSL value.
    // That way, hittesting on the edges will always return the correct value.
    if (distanceFromRadius > radius)
    {
        xToUse = (radius / distanceFromRadius) * (x - radius) + radius;
        yToUse = (radius / distanceFromRadius) * (y - radius) + radius;
        distanceFromRadius = radius;
    }

    Hsv hsvMin = baseHsv;
    Hsv hsvMiddle1 = baseHsv;
    Hsv hsvMiddle2 = baseHsv;
    Hsv hsvMiddle3 = baseHsv;
    Hsv hsvMiddle4 = baseHsv;
    Hsv hsvMax = baseHsv;

    double r = 1 - distanceFromRadius / radius;

    double theta = atan2((radius - yToUse), (radius - xToUse)) * 180.0 / M_PI;
    theta += 180.0;
    theta = floor(theta);

    while (theta > 360)
    {
        theta -= 360;
    }

    double thetaPercent = theta / 360;

    switch (components)
    {
    case winrt::ColorSpectrumComponents::HueValue:
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + thetaPercent * (hMax - hMin);
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + r * (vMax - vMin);
        hsvMin.s = 0;
        hsvMax.s = 1;
        break;

    case winrt::ColorSpectrumComponents::HueSaturation:
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + thetaPercent * (hMax - hMin);
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + r * (sMax - sMin);
        hsvMin.v = 0;
        hsvMax.v = 1;
        break;

    case winrt::ColorSpectrumComponents::ValueHue:
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + thetaPercent * (vMax - vMin);
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + r * (hMax - hMin);
        hsvMin.s = 0;
        hsvMax.s = 1;
        break;

    case winrt::ColorSpectrumComponents::ValueSaturation:
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + thetaPercent * (vMax - vMin);
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + r * (sMax - sMin);
        hsvMin.h = 0;
        hsvMiddle1.h = 60;
        hsvMiddle2.h = 120;
        hsvMiddle3.h = 180;
        hsvMiddle4.h = 240;
        hsvMax.h = 300;
        break;

    case winrt::ColorSpectrumComponents::SaturationHue:
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + thetaPercent * (sMax - sMin);
        hsvMin.h = hsvMiddle1.h = hsvMiddle2.h = hsvMiddle3.h = hsvMiddle4.h = hsvMax.h = hMin + r * (hMax - hMin);
        hsvMin.v = 0;
        hsvMax.v = 1;
        break;

    case winrt::ColorSpectrumComponents::SaturationValue:
        hsvMin.s = hsvMiddle1.s = hsvMiddle2.s = hsvMiddle3.s = hsvMiddle4.s = hsvMax.s = sMin + thetaPercent * (sMax - sMin);
        hsvMin.v = hsvMiddle1.v = hsvMiddle2.v = hsvMiddle3.v = hsvMiddle4.v = hsvMax.v = vMin + r * (vMax - vMin);
        hsvMin.h = 0;
        hsvMiddle1.h = 60;
        hsvMiddle2.h = 120;
        hsvMiddle3.h = 180;
        hsvMiddle4.h = 240;
        hsvMax.h = 300;
        break;
    }

    // If saturation is an axis in the spectrum with hue, or value is an axis, then we want
    // that axis to go from maximum at the top to minimum at the bottom,
    // or maximum at the outside to minimum at the inside in the case of the ring configuration,
    // so we'll invert the number before assigning the HSL value to the array.
    // Otherwise, we'll have a very narrow section in the middle that actually has meaningful hue
    // in the case of the ring configuration.
    if (components == winrt::ColorSpectrumComponents::HueSaturation ||
        components == winrt::ColorSpectrumComponents::SaturationHue)
    {
        hsvMin.s = sMax - hsvMin.s + sMin;
        hsvMiddle1.s = sMax - hsvMiddle1.s + sMin;
        hsvMiddle2.s = sMax - hsvMiddle2.s + sMin;
        hsvMiddle3.s = sMax - hsvMiddle3.s + sMin;
        hsvMiddle4.s = sMax - hsvMiddle4.s + sMin;
        hsvMax.s = sMax - hsvMax.s + sMin;
    }
    else
    {
        hsvMin.v = vMax - hsvMin.v + vMin;
        hsvMiddle1.v = vMax - hsvMiddle1.v + vMin;
        hsvMiddle2.v = vMax - hsvMiddle2.v + vMin;
        hsvMiddle3.v = vMax - hsvMiddle3.v + vMin;
        hsvMiddle4.v = vMax - hsvMiddle4.v + vMin;
        hsvMax.v = vMax - hsvMax.v + vMin;
    }

    newHsvValues->push_back(hsvMin);

    Rgb rgbMin = HsvToRgb(hsvMin);
    bgraMinPixelData->push_back(static_cast<::byte>(round(rgbMin.b * 255))); // b
    bgraMinPixelData->push_back(static_cast<::byte>(round(rgbMin.g * 255))); // g
    bgraMinPixelData->push_back(static_cast<::byte>(round(rgbMin.r * 255))); // r
    bgraMinPixelData->push_back(255); // a

    // We'll only save pixel data for the middle bitmaps if our third dimension is hue.
    if (components == winrt::ColorSpectrumComponents::ValueSaturation ||
        components == winrt::ColorSpectrumComponents::SaturationValue)
    {
        Rgb rgbMiddle1 = HsvToRgb(hsvMiddle1);
        bgraMiddle1PixelData->push_back(static_cast<::byte>(round(rgbMiddle1.b * 255))); // b
        bgraMiddle1PixelData->push_back(static_cast<::byte>(round(rgbMiddle1.g * 255))); // g
        bgraMiddle1PixelData->push_back(static_cast<::byte>(round(rgbMiddle1.r * 255))); // r
        bgraMiddle1PixelData->push_back(255); // a

        Rgb rgbMiddle2 = HsvToRgb(hsvMiddle2);
        bgraMiddle2PixelData->push_back(static_cast<::byte>(round(rgbMiddle2.b * 255))); // b
        bgraMiddle2PixelData->push_back(static_cast<::byte>(round(rgbMiddle2.g * 255))); // g
        bgraMiddle2PixelData->push_back(static_cast<::byte>(round(rgbMiddle2.r * 255))); // r
        bgraMiddle2PixelData->push_back(255); // a

        Rgb rgbMiddle3 = HsvToRgb(hsvMiddle3);
        bgraMiddle3PixelData->push_back(static_cast<::byte>(round(rgbMiddle3.b * 255))); // b
        bgraMiddle3PixelData->push_back(static_cast<::byte>(round(rgbMiddle3.g * 255))); // g
        bgraMiddle3PixelData->push_back(static_cast<::byte>(round(rgbMiddle3.r * 255))); // r
        bgraMiddle3PixelData->push_back(255); // a

        Rgb rgbMiddle4 = HsvToRgb(hsvMiddle4);
        bgraMiddle4PixelData->push_back(static_cast<::byte>(round(rgbMiddle4.b * 255))); // b
        bgraMiddle4PixelData->push_back(static_cast<::byte>(round(rgbMiddle4.g * 255))); // g
        bgraMiddle4PixelData->push_back(static_cast<::byte>(round(rgbMiddle4.r * 255))); // r
        bgraMiddle4PixelData->push_back(255); // a
    }

    Rgb rgbMax = HsvToRgb(hsvMax);
    bgraMaxPixelData->push_back(static_cast<::byte>(round(rgbMax.b * 255))); // b
    bgraMaxPixelData->push_back(static_cast<::byte>(round(rgbMax.g * 255))); // g
    bgraMaxPixelData->push_back(static_cast<::byte>(round(rgbMax.r * 255))); // r
    bgraMaxPixelData->push_back(255); // a
}

void ColorSpectrum::UpdateBitmapSources()
{
    if (!m_spectrumOverlayRectangle ||
        !m_spectrumOverlayEllipse)
    {
        return;
    }

    winrt::float4 hsvColor = HsvColor();
    winrt::ColorSpectrumComponents components = Components();

    // We'll set the base image and the overlay image based on which component is our third dimension.
    // If it's saturation or luminosity, then the base image is that dimension at its minimum value,
    // while the overlay image is that dimension at its maximum value.
    // If it's hue, then we'll figure out where in the color wheel we are, and then use the two
    // colors on either side of our position as our base image and overlay image.
    // For example, if our hue is orange, then the base image would be red and the overlay image yellow.
    switch (components)
    {
    case winrt::ColorSpectrumComponents::HueValue:
    case winrt::ColorSpectrumComponents::ValueHue:
        if (SharedHelpers::IsRS2OrHigher())
        {
            if (!m_saturationMinimumSurface ||
                !m_saturationMaximumSurface)
            {
                return;
            }

            winrt::SpectrumBrush spectrumBrush{ winrt::make<SpectrumBrush>() };

            spectrumBrush.MinSurface(m_saturationMinimumSurface);
            spectrumBrush.MaxSurface(m_saturationMaximumSurface);
            spectrumBrush.MaxSurfaceOpacity(hsv::GetSaturation(hsvColor));
            m_spectrumRectangle.Fill(spectrumBrush);
            m_spectrumEllipse.Fill(spectrumBrush);
        }
        else
        {
            if (!m_saturationMinimumBitmap ||
                !m_saturationMaximumBitmap)
            {
                return;
            }

            winrt::ImageBrush spectrumBrush;
            winrt::ImageBrush spectrumOverlayBrush;

            spectrumBrush.ImageSource(m_saturationMinimumBitmap);
            spectrumOverlayBrush.ImageSource(m_saturationMaximumBitmap);
            m_spectrumOverlayRectangle.Opacity(hsv::GetSaturation(hsvColor));
            m_spectrumOverlayEllipse.Opacity(hsv::GetSaturation(hsvColor));
            m_spectrumRectangle.Fill(spectrumBrush);
            m_spectrumEllipse.Fill(spectrumBrush);
            m_spectrumOverlayRectangle.Fill(spectrumOverlayBrush);
            m_spectrumOverlayRectangle.Fill(spectrumOverlayBrush);
        }
        break;

    case winrt::ColorSpectrumComponents::HueSaturation:
    case winrt::ColorSpectrumComponents::SaturationHue:
        if (SharedHelpers::IsRS2OrHigher())
        {
            if (!m_valueSurface)
            {
                return;
            }

            winrt::SpectrumBrush spectrumBrush{ winrt::make<SpectrumBrush>() };

            spectrumBrush.MinSurface(m_valueSurface);
            spectrumBrush.MaxSurface(m_valueSurface);
            spectrumBrush.MaxSurfaceOpacity(1);
            m_spectrumRectangle.Fill(spectrumBrush);
            m_spectrumEllipse.Fill(spectrumBrush);
        }
        else
        {
            if (!m_valueBitmap)
            {
                return;
            }

            winrt::ImageBrush spectrumBrush;
            winrt::ImageBrush spectrumOverlayBrush;

            spectrumBrush.ImageSource(m_valueBitmap);
            spectrumOverlayBrush.ImageSource(m_valueBitmap);
            m_spectrumOverlayRectangle.Opacity(1);
            m_spectrumOverlayEllipse.Opacity(1);
            m_spectrumRectangle.Fill(spectrumBrush);
            m_spectrumEllipse.Fill(spectrumBrush);
            m_spectrumOverlayRectangle.Fill(spectrumOverlayBrush);
            m_spectrumOverlayRectangle.Fill(spectrumOverlayBrush);
        }
        break;

    case winrt::ColorSpectrumComponents::ValueSaturation:
    case winrt::ColorSpectrumComponents::SaturationValue:
        if (SharedHelpers::IsRS2OrHigher())
        {
            if (!m_hueRedSurface ||
                !m_hueYellowSurface ||
                !m_hueGreenSurface ||
                !m_hueCyanSurface ||
                !m_hueBlueSurface ||
                !m_huePurpleSurface)
            {
                return;
            }

            winrt::SpectrumBrush spectrumBrush{ winrt::make<SpectrumBrush>() };

            double sextant = hsv::GetHue(hsvColor) / 60.0;

            if (sextant < 1)
            {
                spectrumBrush.MinSurface(m_hueRedSurface);
                spectrumBrush.MaxSurface(m_hueYellowSurface);
            }
            else if (sextant >= 1 && sextant < 2)
            {
                spectrumBrush.MinSurface(m_hueYellowSurface);
                spectrumBrush.MaxSurface(m_hueGreenSurface);
            }
            else if (sextant >= 2 && sextant < 3)
            {
                spectrumBrush.MinSurface(m_hueGreenSurface);
                spectrumBrush.MaxSurface(m_hueCyanSurface);
            }
            else if (sextant >= 3 && sextant < 4)
            {
                spectrumBrush.MinSurface(m_hueCyanSurface);
                spectrumBrush.MaxSurface(m_hueBlueSurface);
            }
            else if (sextant >= 4 && sextant < 5)
            {
                spectrumBrush.MinSurface(m_hueBlueSurface);
                spectrumBrush.MaxSurface(m_huePurpleSurface);
            }
            else
            {
                spectrumBrush.MinSurface(m_huePurpleSurface);
                spectrumBrush.MaxSurface(m_hueRedSurface);
            }

            spectrumBrush.MaxSurfaceOpacity(sextant - static_cast<int>(sextant));
            m_spectrumRectangle.Fill(spectrumBrush);
            m_spectrumEllipse.Fill(spectrumBrush);
        }
        else
        {
            if (!m_hueRedBitmap ||
                !m_hueYellowBitmap ||
                !m_hueGreenBitmap ||
                !m_hueCyanBitmap ||
                !m_hueBlueBitmap ||
                !m_huePurpleBitmap)
            {
                return;
            }

            winrt::ImageBrush spectrumBrush;
            winrt::ImageBrush spectrumOverlayBrush;

            double sextant = hsv::GetHue(hsvColor) / 60.0;

            if (sextant < 1)
            {
                spectrumBrush.ImageSource(m_hueRedBitmap);
                spectrumOverlayBrush.ImageSource(m_hueYellowBitmap);
            }
            else if (sextant >= 1 && sextant < 2)
            {
                spectrumBrush.ImageSource(m_hueYellowBitmap);
                spectrumOverlayBrush.ImageSource(m_hueGreenBitmap);
            }
            else if (sextant >= 2 && sextant < 3)
            {
                spectrumBrush.ImageSource(m_hueGreenBitmap);
                spectrumOverlayBrush.ImageSource(m_hueCyanBitmap);
            }
            else if (sextant >= 3 && sextant < 4)
            {
                spectrumBrush.ImageSource(m_hueCyanBitmap);
                spectrumOverlayBrush.ImageSource(m_hueBlueBitmap);
            }
            else if (sextant >= 4 && sextant < 5)
            {
                spectrumBrush.ImageSource(m_hueBlueBitmap);
                spectrumOverlayBrush.ImageSource(m_huePurpleBitmap);
            }
            else
            {
                spectrumBrush.ImageSource(m_huePurpleBitmap);
                spectrumOverlayBrush.ImageSource(m_hueRedBitmap);
            }

            m_spectrumOverlayRectangle.Opacity(sextant - static_cast<int>(sextant));
            m_spectrumOverlayEllipse.Opacity(sextant - static_cast<int>(sextant));
            m_spectrumRectangle.Fill(spectrumBrush);
            m_spectrumEllipse.Fill(spectrumBrush);
            m_spectrumOverlayRectangle.Fill(spectrumOverlayBrush);
            m_spectrumOverlayRectangle.Fill(spectrumOverlayBrush);
        }
        break;
    }
}

bool ColorSpectrum::SelectionEllipseShouldBeLight()
{
    // The selection ellipse should be light if and only if the chosen color
    // contrasts more with black than it does with white.
    // To find how much something contrasts with white, we use the equation
    // for relative luminance, which is given by
    //
    // L = 0.2126 * Rg + 0.7152 * Gg + 0.0722 * Bg
    //
    // where Xg = { X/3294 if X <= 10, (R/269 + 0.0513)^2.4 otherwise }
    //
    // If L is closer to 1, then the color is closer to white; if it is closer to 0,
    // then the color is closer to black.  This is based on the fact that the human
    // eye perceives green to be much brighter than red, which in turn is perceived to be
    // brighter than blue.
    //
    // If the third dimension is value, then we won't be updating the spectrum's displayed colors,
    // so in that case we should use a value of 1 when considering the backdrop
    // for the selection ellipse.
    winrt::Color displayedColor = {};

    if (Components() == winrt::ColorSpectrumComponents::HueSaturation ||
        Components() == winrt::ColorSpectrumComponents::SaturationHue)
    {
        winrt::float4 hsvColor = HsvColor();
        Rgb color = HsvToRgb(Hsv(hsv::GetHue(hsvColor), hsv::GetSaturation(hsvColor), 1.0));
        displayedColor = ColorFromRgba(color, hsv::GetAlpha(hsvColor));
    }
    else
    {
        displayedColor = Color();
    }

    double rg = displayedColor.R <= 10 ? displayedColor.R / 3294.0 : pow(displayedColor.R / 269.0 + 0.0513, 2.4);
    double gg = displayedColor.G <= 10 ? displayedColor.G / 3294.0 : pow(displayedColor.G / 269.0 + 0.0513, 2.4);
    double bg = displayedColor.B <= 10 ? displayedColor.B / 3294.0 : pow(displayedColor.B / 269.0 + 0.0513, 2.4);

    return 0.2126 * rg + 0.7152 * gg + 0.0722 * bg <= 0.5;
}
