﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBox.h"
#include "NumberBoxAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Utils.h"

static constexpr wstring_view c_tabViewDownButtonName{ L"DownSpinButton"sv };
static constexpr wstring_view c_tabViewUpButtonName{ L"UpSpinButton"sv };
static constexpr wstring_view c_tabViewTextBoxName{ L"InputBox"sv };

NumberBox::NumberBox()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NumberBox);

    // Default values for the number formatter
    const auto formatter = winrt::DecimalFormatter();
    formatter.IntegerDigits(1);
    formatter.FractionDigits(0);
    NumberFormatter(formatter);

    PointerWheelChanged({ this, &NumberBox::OnScroll });

    SetDefaultStyleKey(this);
}

winrt::AutomationPeer NumberBox::OnCreateAutomationPeer()
{
    return winrt::make<NumberBoxAutomationPeer>(*this);
}

void NumberBox::OnApplyTemplate()
{
    const winrt::IControlProtected controlProtected = *this;

    if (const auto spinDown = GetTemplateChildT<winrt::RepeatButton>(c_tabViewDownButtonName, controlProtected))
    {
        m_upButtonClickRevoker = spinDown.Click(winrt::auto_revoke, { this, &NumberBox::OnSpinDownClick });

        // Do localization for the down button
        if (winrt::AutomationProperties::GetName(spinDown).empty())
        {
            const auto spinDownName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxDownSpinButtonName);
            winrt::AutomationProperties::SetName(spinDown, spinDownName);
        }
    }

    if (const auto spinUp = GetTemplateChildT<winrt::RepeatButton>(c_tabViewUpButtonName, controlProtected))
    {
        m_downButtonClickRevoker = spinUp.Click(winrt::auto_revoke, { this, &NumberBox::OnSpinUpClick });

        // Do localization for the up button
        if (winrt::AutomationProperties::GetName(spinUp).empty())
        {
            const auto spinUpName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxUpSpinButtonName);
            winrt::AutomationProperties::SetName(spinUp, spinUpName);
        }
    }

    m_textBox.set([this, controlProtected]() {
        const auto textBox = GetTemplateChildT<winrt::TextBox>(c_tabViewTextBoxName, controlProtected);
        if (textBox)
        {
            m_textBoxLostFocusRevoker = textBox.LostFocus(winrt::auto_revoke, { this, &NumberBox::OnTextBoxLostFocus });
            m_textBoxKeyUpRevoker = textBox.KeyUp(winrt::auto_revoke, { this, &NumberBox::OnNumberBoxKeyUp });
        }
        return textBox;
    }());

    // Initializing precision formatter. This formatter works neutrally to protect against floating point imprecision resulting from stepping/calc
    m_stepPrecisionFormatter.FractionDigits(0);
    m_stepPrecisionFormatter.IntegerDigits(1);
    m_stepPrecisionFormatter.NumberRounder(nullptr);
    m_stepPrecisionRounder.RoundingAlgorithm(winrt::RoundingAlgorithm::RoundHalfAwayFromZero);

    SetSpinButtonVisualState();
    UpdateTextToValue();
}

void NumberBox::OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const auto oldValue = unbox_value<double>(args.OldValue());

    CoerceValue();

    auto newValue = Value();

    if (std::isnan(newValue) && BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
    {
        // In the validation case, we don't consider NaN to be valid.
        newValue = oldValue;
        Value(newValue);
    }

    if (newValue != oldValue)
    {
        // Fire ValueChanged event
        const auto valueChangedArgs = winrt::make_self<NumberBoxValueChangedEventArgs>(oldValue, newValue);
        m_valueChangedEventSource(*this, *valueChangedArgs);

        // Fire value property change for UIA
        if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).as<winrt::NumberBoxAutomationPeer>())
        {
            winrt::get_self<NumberBoxAutomationPeer>(peer)->RaiseValueChangedEvent(oldValue, newValue);
        }
    }

    UpdateTextToValue();
}

void NumberBox::OnMinimumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    CoerceValue();
}

void NumberBox::OnMaximumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    CoerceValue();
}

void NumberBox::OnNumberFormatterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // Update text with new formatting
    UpdateTextToValue();
}

void NumberBox::ValidateNumberFormatter(winrt::INumberFormatter2 value)
{
    // NumberFormatter also needs to be an INumberParser
    if (!value.try_as<winrt::INumberParser>())
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

void NumberBox::OnSpinButtonPlacementModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SetSpinButtonVisualState();
}

void NumberBox::OnTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto&& textBox = m_textBox.get())
    {
        textBox.Text(Text());
    }
}

void NumberBox::OnBasicValidationModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ValidateInput();
}

void NumberBox::OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    ValidateInput();
}

void NumberBox::CoerceValue()
{
    // Validate that the value is in bounds
    const auto value = Value();
    if (!std::isnan(value) && !IsInBounds(value) && BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
    {
        // Coerce value to be within range
        const auto max = Maximum();
        if (value > max)
        {
            Value(max);
        }
        else
        {
            Value(Minimum());
        }
    }
}

void NumberBox::ValidateInput()
{
    // Validate the content of the inner textbox
    if (auto&& textBox = m_textBox.get())
    {
        const auto text = textBox.Text();
        
        // Handles empty TextBox case, set text to current value
        if (text.empty())
        {
            UpdateTextToValue();
        }
        else
        {
            // Setting NumberFormatter to something that isn't an INumberParser will throw an exception, so this should be safe
            const auto numberParser = NumberFormatter().as<winrt::INumberParser>();
            const auto parsedNum = numberParser.ParseDouble(text);

            if (!parsedNum)
            {
                if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
                {
                    // Override text to current value
                    UpdateTextToValue();
                }
            }
            else
            {
                Value(parsedNum.Value());
            }
        }
    }
}

void NumberBox::OnSpinDownClick(winrt::IInspectable const&  sender, winrt::RoutedEventArgs const& args)
{
    StepValueDown();
}

void NumberBox::OnSpinUpClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    StepValueUp();
}

void NumberBox::OnNumberBoxKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    switch (args.OriginalKey())
    {
        case winrt::VirtualKey::Enter:
        case winrt::VirtualKey::GamepadA:
            ValidateInput();
            break;

        case winrt::VirtualKey::Escape:
        case winrt::VirtualKey::GamepadB:
            UpdateTextToValue();
            break;

        case winrt::VirtualKey::Up:
            StepValueUp();
            break;

        case winrt::VirtualKey::Down:
            StepValueDown();
            break;
    }
}

void NumberBox::OnScroll(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    if (HyperScrollEnabled())
    {
        const auto delta = args.GetCurrentPoint(*this).Properties().MouseWheelDelta();
        if (delta > 0)
        {
            StepValueUp();
        }
        else if (delta < 0)
        {
            StepValueDown();
        }
    }
}

void NumberBox::StepValue(bool isPositive)
{
    auto newVal = Value();

    if (isPositive)
    {
        newVal += StepFrequency();
    }
    else
    {
        newVal -= StepFrequency();
    }

    if (WrapEnabled())
    {
        const auto max = Maximum();
        const auto min = Minimum();

        if (newVal > max)
        {
            newVal = min;
        }
        else if (newVal < min)
        {
            newVal = max;
        }
    }

    // Safeguard for floating point imprecision errors
    m_stepPrecisionRounder.SignificantDigits(ComputePrecisionRounderSigDigits(newVal));
    newVal = m_stepPrecisionRounder.RoundDouble(newVal);

    // Update Text and Revalidate new value
    Value(newVal);
}

// Computes the number of significant digits that precision rounder should use. This helps to prevent floating point imprecision errors. 
int NumberBox::ComputePrecisionRounderSigDigits(double newVal)
{
    const auto oldVal = Value();

    // Run formatter on both values to discard trailing and leading 0's.
    const auto formattedVal = wstring_view(m_stepPrecisionFormatter.Format(oldVal));
    const auto formattedStep = wstring_view(m_stepPrecisionFormatter.Format(StepFrequency()));
    const auto formattedNew = wstring_view(m_stepPrecisionFormatter.Format(newVal));

    // Get size of only decimal portion of both old numbers. 
    const auto oldValSig = static_cast<int>(formattedVal.substr(formattedVal.find_first_of('.') + 1).size());
    const auto stepSig = static_cast<int>(formattedStep.substr(formattedStep.find_first_of('.') + 1).size());

    // Pick bigger of two decimal sigDigits
    auto result = std::max(oldValSig, stepSig);

    // append # of integer digits from new value
    result += (int)formattedNew.substr(0, formattedNew.find_first_of('.')).size();
    return result;
}

// Runs formatter and updates TextBox to it's value property, run on construction if Value != 0
void NumberBox::UpdateTextToValue()
{
    if (auto&& textBox = m_textBox.get())
    {
        const auto formattedValue = NumberFormatter().FormatDouble(Value());
        textBox.Text(formattedValue);
    }
}

// Enables or Disables Spin Buttons
void NumberBox::SetSpinButtonVisualState()
{
    if (SpinButtonPlacementMode() == winrt::NumberBoxSpinButtonPlacementMode::Inline)
    {
        winrt::VisualStateManager::GoToState(*this, L"SpinButtonsVisible", false);
    }
    else if (SpinButtonPlacementMode() == winrt::NumberBoxSpinButtonPlacementMode::Hidden)
    {
        winrt::VisualStateManager::GoToState(*this, L"SpinButtonsCollapsed", false);
    }
}

bool NumberBox::IsInBounds(double value)
{
    return (value >= Minimum() && value <= Maximum());
}

