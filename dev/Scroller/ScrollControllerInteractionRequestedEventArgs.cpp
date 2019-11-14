﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerTrace.h"
#include "ScrollControllerInteractionRequestedEventArgs.h"

CppWinRTActivatableClassWithBasicFactory(ScrollControllerInteractionRequestedEventArgs)

ScrollControllerInteractionRequestedEventArgs::ScrollControllerInteractionRequestedEventArgs(
    const winrt::PointerPoint& pointerPoint)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::PointerPointToString(pointerPoint).c_str());

    m_pointerPoint = pointerPoint;
}

winrt::PointerPoint ScrollControllerInteractionRequestedEventArgs::PointerPoint() const
{
    return m_pointerPoint;
}

bool ScrollControllerInteractionRequestedEventArgs::Handled() const
{
    return m_handled;
}

void ScrollControllerInteractionRequestedEventArgs::Handled(bool handled)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, handled);
    m_handled = handled;
}
