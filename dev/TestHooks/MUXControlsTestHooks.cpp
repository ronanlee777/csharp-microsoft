﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MUXControlsTestHooksFactory.h"

#ifdef SCROLLER_INCLUDED
#include "ScrollerTrace.h"
#include "ScrollViewerTrace.h"
#endif

#ifdef SWIPECONTROL_INCLUDED
#include "SwipeControlTrace.h"
#endif

#ifdef COMMANDBARFLYOUT_INCLUDED
#include "CommandBarFlyoutTrace.h"
#endif

#ifdef REPEATER_INCLUDED
#include "RepeaterTrace.h"
#endif

/*static*/
UCHAR MUXControlsTestHooks::GetLoggingLevelForType(const wstring_view& type)
{
    if (type.empty())
    {
        return m_globalLoggingLevel;
    }
    else
    {
        auto iterator = m_typeLoggingLevels.find(type);

        if (iterator != m_typeLoggingLevels.end())
        {
            return iterator->second;
        }
        else
        {
            return WINEVENT_LEVEL_NONE;
        }
    }
}

UCHAR MUXControlsTestHooks::GetLoggingLevelForInstance(const winrt::IInspectable& sender)
{
    if (sender)
    {
        auto iterator = m_instanceLoggingLevels.find(sender);

        if (iterator != m_instanceLoggingLevels.end())
        {
            return iterator->second;
        }
        else
        {
            return WINEVENT_LEVEL_NONE;
        }
    }
    else
    {
        return m_globalLoggingLevel;
    }
}

void MUXControlsTestHooks::SetOutputDebugStringLevelForTypeImpl(const wstring_view& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
#ifdef SCROLLER_INCLUDED
    if (type == L"Scroller" || type.empty())
    {
        ScrollerTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        ScrollerTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
    if (type == L"ScrollViewer" || type.empty())
    {
        ScrollViewerTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        ScrollViewerTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef SWIPECONTROL_INCLUDED
    if (type == L"SwipeControl" || type.empty())
    {
        SwipeControlTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        SwipeControlTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef COMMANDBARFLYOUT_INCLUDED
    if (type == L"CommandBarFlyout" || type.empty())
    {
        CommandBarFlyoutTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        CommandBarFlyoutTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef REPEATER_INCLUDED
    if (type == L"Repeater" || type.empty())
    {
        RepeaterTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
    }
#endif
}

void MUXControlsTestHooks::SetLoggingLevelForTypeImpl(const wstring_view& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    UCHAR loggingProviderLevel = WINEVENT_LEVEL_NONE;

    if (isLoggingVerboseLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_VERBOSE;
    }
    else if (isLoggingInfoLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_INFO;
    }

    if (type.empty())
    {
        m_globalLoggingLevel = loggingProviderLevel;
    }
    else
    {
        auto iterator = m_typeLoggingLevels.find(type);

        if (iterator != m_typeLoggingLevels.end())
        {
            if (loggingProviderLevel == WINEVENT_LEVEL_NONE)
            {
                m_typeLoggingLevels.erase(iterator);
            }
            else
            {
                iterator->second = loggingProviderLevel;
            }
        }
        else if (loggingProviderLevel != WINEVENT_LEVEL_NONE)
        {
            m_typeLoggingLevels.emplace(type, loggingProviderLevel);
        }
    }
}

void MUXControlsTestHooks::SetLoggingLevelForInstanceImpl(const winrt::IInspectable& sender, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    UCHAR loggingProviderLevel = WINEVENT_LEVEL_NONE;

    if (isLoggingVerboseLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_VERBOSE;
    }
    else if (isLoggingInfoLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_INFO;
    }

    if (sender)
    {
        auto iterator = m_instanceLoggingLevels.find(sender);

        if (iterator != m_instanceLoggingLevels.end())
        {
            if (loggingProviderLevel == WINEVENT_LEVEL_NONE)
            {
                m_instanceLoggingLevels.erase(iterator);
            }
            else
            {
                iterator->second = loggingProviderLevel;
            }
        }
        else if (loggingProviderLevel != WINEVENT_LEVEL_NONE)
        {
            m_instanceLoggingLevels.emplace(sender, loggingProviderLevel);
        }
    }
    else
    {
        m_globalLoggingLevel = loggingProviderLevel;
    }
}

void MUXControlsTestHooks::LogMessage(const winrt::IInspectable& sender, const wstring_view& message, bool isVerboseLevel)
{
    com_ptr<MUXControlsTestHooksLoggingMessageEventArgs> loggingMessageEventArgs = winrt::make_self<MUXControlsTestHooksLoggingMessageEventArgs>();

    loggingMessageEventArgs->SetMessage(message);
    loggingMessageEventArgs->SetIsVerboseLevel(isVerboseLevel);
    m_loggingMessageEventSource(sender, *loggingMessageEventArgs);
}

winrt::event_token MUXControlsTestHooks::LoggingMessageImpl(
    winrt::TypedEventHandler<winrt::IInspectable, winrt::MUXControlsTestHooksLoggingMessageEventArgs> const& value)
{
    return m_loggingMessageEventSource.add(value);
}

void MUXControlsTestHooks::LoggingMessageImpl(winrt::event_token const& token)
{
    m_loggingMessageEventSource.remove(token);
}
