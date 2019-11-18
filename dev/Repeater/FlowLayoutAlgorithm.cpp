﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include <ItemsRepeater.common.h>
#include "FlowLayoutAlgorithm.h"
#include "VirtualizingLayoutContext.h"

void FlowLayoutAlgorithm::InitializeForContext(
    const winrt::VirtualizingLayoutContext& context,
    IFlowLayoutAlgorithmDelegates* callbacks)
{
    m_algorithmCallbacks = callbacks;
    m_context.set(context);
    m_elementManager.SetContext(context);
}

void FlowLayoutAlgorithm::UninitializeForContext(const winrt::VirtualizingLayoutContext& context)
{
    if (IsVirtualizingContext())
    {
        // This layout is about to be detached. Let go of all elements
        // being held and remove the layout state from the context.
        m_elementManager.ClearRealizedRange();
    }
    context.LayoutStateCore(nullptr);
}

winrt::Size FlowLayoutAlgorithm::Measure(
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context,
    bool isWrapping,
    double minItemSpacing,
    double lineSpacing,
    unsigned int maxItemsPerLine,
    const ScrollOrientation& orientation,
    const wstring_view& layoutId)
{
    SetScrollOrientation(orientation);

    // If minor size is infinity, there is only one line and no need to align that line.
    m_scrollOrientationSameAsFlow = availableSize.*Minor() == std::numeric_limits<float>::infinity();
    const auto realizationRect = RealizationRect();
    REPEATER_TRACE_INFO(L"%*s: \tMeasureLayout Realization(%.0f,%.0f,%.0f,%.0f)\n",
        winrt::get_self<VirtualizingLayoutContext>(context)->Indent(),
        layoutId.data(),
        realizationRect.X, realizationRect.Y, realizationRect.Width, realizationRect.Height);

    auto suggestedAnchorIndex = m_context.get().RecommendedAnchorIndex();
    if (m_elementManager.IsIndexValidInData(suggestedAnchorIndex))
    {
        auto anchorRealized = m_elementManager.IsDataIndexRealized(suggestedAnchorIndex);
        if (!anchorRealized)
        {
            MakeAnchor(m_context.get(), suggestedAnchorIndex, availableSize);
        }
    }

    m_elementManager.OnBeginMeasure(orientation);

    int anchorIndex = GetAnchorIndex(availableSize, isWrapping, minItemSpacing, layoutId);
    Generate(GenerateDirection::Forward, anchorIndex, availableSize, minItemSpacing, lineSpacing, maxItemsPerLine, layoutId);
    Generate(GenerateDirection::Backward, anchorIndex, availableSize, minItemSpacing, lineSpacing, maxItemsPerLine, layoutId);
    if (isWrapping && IsReflowRequired())
    {
        REPEATER_TRACE_INFO(L"%*s: \tReflow Pass \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data());
        auto firstElementBounds = m_elementManager.GetLayoutBoundsForRealizedIndex(0);
        firstElementBounds.*MinorStart() = 0;
        m_elementManager.SetLayoutBoundsForRealizedIndex(0, firstElementBounds);
        Generate(GenerateDirection::Forward, 0 /*anchorIndex*/, availableSize, minItemSpacing, lineSpacing, maxItemsPerLine, layoutId);
    }

    RaiseLineArranged();
    m_collectionChangePending = false;
    m_lastExtent = EstimateExtent(availableSize, layoutId);
    SetLayoutOrigin();

    return winrt::Size{ m_lastExtent.Width, m_lastExtent.Height };
}

winrt::Size FlowLayoutAlgorithm::Arrange(
    const winrt::Size& finalSize,
    const winrt::VirtualizingLayoutContext& context,
    bool isWrapping,
    FlowLayoutAlgorithm::LineAlignment lineAlignment,
    const wstring_view& layoutId)
{
    REPEATER_TRACE_INFO(L"%*s: \tArrangeLayout \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data());
    ArrangeVirtualizingLayout(finalSize, lineAlignment, isWrapping, layoutId);

    return winrt::Size
    {
        std::max(finalSize.Width, m_lastExtent.Width),
        std::max(finalSize.Height, m_lastExtent.Height)
    };
}

void FlowLayoutAlgorithm::MakeAnchor(
    const winrt::VirtualizingLayoutContext& context,
    int index,
    const winrt::Size& availableSize)
{
    m_elementManager.ClearRealizedRange();
    // FlowLayout requires that the anchor is the first element in the row.
    auto internalAnchor = m_algorithmCallbacks->Algorithm_GetAnchorForTargetElement(index, availableSize, context);
    MUX_ASSERT(internalAnchor.Index <= index);

    // No need to set the position of the anchor.
    // (0,0) is fine for now since the extent can
    // grow in any direction.
    for (int dataIndex = internalAnchor.Index; dataIndex < index + 1; ++dataIndex)
    {
        auto element = context.GetOrCreateElementAt(dataIndex, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
        element.Measure(m_algorithmCallbacks->Algorithm_GetMeasureSize(dataIndex, availableSize, context));
        m_elementManager.Add(element, dataIndex);
    }
}

void FlowLayoutAlgorithm::OnItemsSourceChanged(
    const winrt::IInspectable& source,
    winrt::NotifyCollectionChangedEventArgs const& args,
    const winrt::IVirtualizingLayoutContext& /*context*/)
{
    m_elementManager.DataSourceChanged(source, args);
    m_collectionChangePending = true;
}

winrt::Size FlowLayoutAlgorithm::MeasureElement(
    const winrt::UIElement& element,
    int index,
    const winrt::Size& availableSize,
    const winrt::VirtualizingLayoutContext& context)
{
    auto measureSize = m_algorithmCallbacks->Algorithm_GetMeasureSize(index, availableSize, context);
    element.Measure(measureSize);
    auto provisionalArrangeSize = m_algorithmCallbacks->Algorithm_GetProvisionalArrangeSize(index, measureSize, element.DesiredSize(), context);
    m_algorithmCallbacks->Algorithm_OnElementMeasured(element, index, availableSize, measureSize, element.DesiredSize(), provisionalArrangeSize, context);

    return provisionalArrangeSize;
}

#pragma region Measure related private methods

int FlowLayoutAlgorithm::GetAnchorIndex(
    const winrt::Size& availableSize,
    bool isWrapping,
    double minItemSpacing,
    const wstring_view& layoutId)
{
    int anchorIndex = -1;
    winrt::Point anchorPosition{};
    auto context = m_context.get();

    if (!IsVirtualizingContext())
    {
        // Non virtualizing host, start generating from the element 0
        anchorIndex = context.ItemCountCore() > 0 ? 0 : -1;
    }
    else
    {
        bool isRealizationWindowConnected = m_elementManager.IsWindowConnected(RealizationRect(), GetScrollOrientation(), m_scrollOrientationSameAsFlow);
        // Item spacing and size in non-virtualizing direction change can cause elements to reflow
        // and get a new column position. In that case we need the anchor to be positioned in the
        // correct column.
        bool needAnchorColumnRevaluation = isWrapping && (
            m_lastAvailableSize.*Minor() != availableSize.*Minor() ||
            m_lastItemSpacing != minItemSpacing ||
            m_collectionChangePending);

        const auto suggestedAnchorIndex = m_context.get().RecommendedAnchorIndex();

        const bool isAnchorSuggestionValid = suggestedAnchorIndex >= 0 &&
            m_elementManager.IsDataIndexRealized(suggestedAnchorIndex);

        if (isAnchorSuggestionValid)
        {
            REPEATER_TRACE_INFO(L"%*s: \tUsing suggested anchor %d\n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data(), suggestedAnchorIndex);
            anchorIndex = m_algorithmCallbacks->Algorithm_GetAnchorForTargetElement(
                suggestedAnchorIndex,
                availableSize,
                context).Index;

            if (m_elementManager.IsDataIndexRealized(anchorIndex))
            {
                auto anchorBounds = m_elementManager.GetLayoutBoundsForDataIndex(anchorIndex);
                if (needAnchorColumnRevaluation)
                {
                    // We were provided a valid anchor, but its position might be incorrect because for example it is in
                    // the wrong column. We do know that the anchor is the first element in the row, so we can force the minor position
                    // to start at 0.
                    anchorPosition = MinorMajorPoint(0, anchorBounds.*MajorStart());
                }
                else
                {
                    anchorPosition = winrt::Point(anchorBounds.X, anchorBounds.Y);
                }
            }
            else
            {
                // It is possible to end up in a situation during a collection change where GetAnchorForTargetElement returns an index
                // which is not in the realized range. Eg. insert one item at index 0 for a grid layout.
                // SuggestedAnchor will be 1 (used to be 0) and GetAnchorForTargetElement will return 0 (left most item in row). However 0 is not in the
                // realized range yet. In this case we realize the gap between the target anchor and the suggested anchor.
                int firstRealizedDataIndex = m_elementManager.GetDataIndexFromRealizedRangeIndex(0);
                MUX_ASSERT(anchorIndex < firstRealizedDataIndex);
                for (int i = firstRealizedDataIndex - 1; i >= anchorIndex; --i)
                {
                    m_elementManager.EnsureElementRealized(false /*forward*/, i, layoutId);
                }

                auto anchorBounds = m_elementManager.GetLayoutBoundsForDataIndex(suggestedAnchorIndex);
                anchorPosition = MinorMajorPoint(0, anchorBounds.*MajorStart());
            }
        }
        else if (needAnchorColumnRevaluation || !isRealizationWindowConnected)
        {
            if (needAnchorColumnRevaluation) { REPEATER_TRACE_INFO(L"%*s: \tNeedAnchorColumnReevaluation \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data()); }
            if (!isRealizationWindowConnected) { REPEATER_TRACE_INFO(L"%*s: \tDisconnected Window \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data()); }

            // The anchor is based on the realization window because a connected ItemsRepeater might intersect the realization window
            // but not the visible window. In that situation, we still need to produce a valid anchor.
            auto anchorInfo = m_algorithmCallbacks->Algorithm_GetAnchorForRealizationRect(availableSize, context);
            anchorIndex = anchorInfo.Index;
            anchorPosition = MinorMajorPoint(0, static_cast<float>(anchorInfo.Offset));
        }
        else
        {
            REPEATER_TRACE_INFO(L"%*s: \tConnected Window - picking first realized element as anchor \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data());
            // No suggestion - just pick first in realized range
            anchorIndex = m_elementManager.GetDataIndexFromRealizedRangeIndex(0);
            auto firstElementBounds = m_elementManager.GetLayoutBoundsForRealizedIndex(0);
            anchorPosition = winrt::Point(firstElementBounds.X, firstElementBounds.Y);
        }
    }

    REPEATER_TRACE_INFO(L"%*s: \tPicked anchor:%d \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data(), anchorIndex);
    MUX_ASSERT(anchorIndex == -1 || m_elementManager.IsIndexValidInData(anchorIndex));
    m_firstRealizedDataIndexInsideRealizationWindow = m_lastRealizedDataIndexInsideRealizationWindow = anchorIndex;
    if (m_elementManager.IsIndexValidInData(anchorIndex))
    {
        if (!m_elementManager.IsDataIndexRealized(anchorIndex))
        {
            // Disconnected, throw everything and create new anchor
            REPEATER_TRACE_INFO(L"%*s Disconnected Window - throwing away all realized elements \n", winrt::get_self<VirtualizingLayoutContext>(context)->Indent(), layoutId.data());
            m_elementManager.ClearRealizedRange();

            auto anchor = m_context.get().GetOrCreateElementAt(anchorIndex, winrt::ElementRealizationOptions::ForceCreate | winrt::ElementRealizationOptions::SuppressAutoRecycle);
            m_elementManager.Add(anchor, anchorIndex);
        }

        auto anchorElement = m_elementManager.GetRealizedElement(anchorIndex);
        auto desiredSize = MeasureElement(anchorElement, anchorIndex, availableSize, m_context.get());
        auto layoutBounds = winrt::Rect{ anchorPosition.X, anchorPosition.Y, desiredSize.Width, desiredSize.Height };
        m_elementManager.SetLayoutBoundsForDataIndex(anchorIndex, layoutBounds);

        REPEATER_TRACE_INFO(L"%*s: \tLayout bounds of anchor %d are (%.0f,%.0f,%.0f,%.0f). \n",
            winrt::get_self<VirtualizingLayoutContext>(context)->Indent(),
            layoutId.data(),
            anchorIndex,
            layoutBounds.X, layoutBounds.Y, layoutBounds.Width, layoutBounds.Height);
    }
    else
    {
        // Throw everything away
        REPEATER_TRACE_INFO(L"%*s \tAnchor index is not valid - throwing away all realized elements \n",
            winrt::get_self<VirtualizingLayoutContext>(context)->Indent(),
            layoutId.data());
        m_elementManager.ClearRealizedRange();
    }

    // TODO: Perhaps we can track changes in the property setter
    m_lastAvailableSize = availableSize;
    m_lastItemSpacing = minItemSpacing;

    return anchorIndex;
}


void FlowLayoutAlgorithm::Generate(
    GenerateDirection direction,
    int anchorIndex,
    const winrt::Size& availableSize,
    double minItemSpacing,
    double lineSpacing,
    unsigned int maxItemsPerLine,
    const wstring_view& layoutId)
{
    if (anchorIndex != -1)
    {
        int step = (direction == GenerateDirection::Forward) ? 1 : -1;

        REPEATER_TRACE_INFO(L"%*s: \tGenerating %ls from anchor %d. \n",
            winrt::get_self<VirtualizingLayoutContext>(m_context.get())->Indent(),
            layoutId.data(),
            direction == GenerateDirection::Forward ? L"forward" : L"backward",
            anchorIndex);

        int previousIndex = anchorIndex;
        int currentIndex = anchorIndex + step;
        auto anchorBounds = m_elementManager.GetLayoutBoundsForDataIndex(anchorIndex);
        float lineOffset = anchorBounds.*MajorStart();
        float lineMajorSize = anchorBounds.*MajorSize();
        unsigned int countInLine = 1;
        bool lineNeedsReposition = false;

        while (m_elementManager.IsIndexValidInData(currentIndex) &&
            ShouldContinueFillingUpSpace(previousIndex, direction))
        {
            // Ensure layout element.
            m_elementManager.EnsureElementRealized(direction == GenerateDirection::Forward, currentIndex, layoutId);
            auto currentElement = m_elementManager.GetRealizedElement(currentIndex);
            auto desiredSize = MeasureElement(currentElement, currentIndex, availableSize, m_context.get());

            // Lay it out.
            auto previousElement = m_elementManager.GetRealizedElement(previousIndex);
            winrt::Rect currentBounds = winrt::Rect{ 0, 0, desiredSize.Width, desiredSize.Height };
            auto previousElementBounds = m_elementManager.GetLayoutBoundsForDataIndex(previousIndex);

            if (direction == GenerateDirection::Forward)
            {
                double remainingSpace = availableSize.*Minor() - (previousElementBounds.*MinorStart() + previousElementBounds.*MinorSize() + minItemSpacing + desiredSize.*Minor());
                if (countInLine >= maxItemsPerLine || m_algorithmCallbacks->Algorithm_ShouldBreakLine(currentIndex, remainingSpace))
                {
                    // No more space in this row. wrap to next row.
                    currentBounds.*MinorStart() = 0;
                    currentBounds.*MajorStart() = previousElementBounds.*MajorStart() + lineMajorSize + static_cast<float>(lineSpacing);

                    if (lineNeedsReposition)
                    {
                        // reposition the previous line (countInLine items)
                        for (unsigned int i = 0; i < countInLine; i++)
                        {
                            auto dataIndex = currentIndex - 1 - i;
                            auto bounds = m_elementManager.GetLayoutBoundsForDataIndex(dataIndex);
                            bounds.*MajorSize() = lineMajorSize;
                            m_elementManager.SetLayoutBoundsForDataIndex(dataIndex, bounds);
                        }
                    }

                    // Setup for next line.
                    lineMajorSize = currentBounds.*MajorSize();
                    lineOffset = currentBounds.*MajorStart();
                    lineNeedsReposition = false;
                    countInLine = 1;
                }
                else
                {
                    // More space is available in this row.
                    currentBounds.*MinorStart() = previousElementBounds.*MinorStart() + previousElementBounds.*MinorSize() + static_cast<float>(minItemSpacing);
                    currentBounds.*MajorStart() = lineOffset;
                    lineMajorSize = std::max(lineMajorSize, currentBounds.*MajorSize());
                    lineNeedsReposition = previousElementBounds.*MajorSize() != currentBounds.*MajorSize();
                    countInLine++;
                }
            }
            else
            {
                // Backward
                double remainingSpace = previousElementBounds.*MinorStart() - (desiredSize.*Minor() + static_cast<float>(minItemSpacing));
                if (countInLine >= maxItemsPerLine || m_algorithmCallbacks->Algorithm_ShouldBreakLine(currentIndex, remainingSpace))
                {
                    // Does not fit, wrap to the previous row
                    const auto availableSizeMinor = availableSize.*Minor();
                    currentBounds.*MinorStart() = std::isfinite(availableSizeMinor) ? availableSizeMinor - desiredSize.*Minor() : 0.0f;
                    currentBounds.*MajorStart() = lineOffset - desiredSize.*Major() - static_cast<float>(lineSpacing);

                    if (lineNeedsReposition)
                    {
                        auto previousLineOffset = m_elementManager.GetLayoutBoundsForDataIndex(currentIndex + countInLine + 1).*MajorStart();
                        // reposition the previous line (countInLine items)
                        for (unsigned int i = 0; i < countInLine; i++)
                        {
                            auto dataIndex = currentIndex + 1 + (int)i;
                            if (dataIndex != anchorIndex)
                            {
                                auto bounds = m_elementManager.GetLayoutBoundsForDataIndex(dataIndex);
                                bounds.*MajorStart() = previousLineOffset - lineMajorSize - static_cast<float>(lineSpacing);
                                bounds.*MajorSize() = lineMajorSize;
                                m_elementManager.SetLayoutBoundsForDataIndex(dataIndex, bounds);
                                REPEATER_TRACE_INFO(L"%*s: \t Corrected Layout bounds of element %d are (%.0f,%.0f,%.0f,%.0f). \n",
                                    winrt::get_self<VirtualizingLayoutContext>(m_context.get())->Indent(),
                                    layoutId.data(),
                                    dataIndex,
                                    bounds.X, bounds.Y, bounds.Width, bounds.Height);
                            }
                        }
                    }

                    // Setup for next line.
                    lineMajorSize = currentBounds.*MajorSize();
                    lineOffset = currentBounds.*MajorStart();
                    lineNeedsReposition = false;
                    countInLine = 1;
                }
                else
                {
                    // Fits in this row. put it in the previous position
                    currentBounds.*MinorStart() = previousElementBounds.*MinorStart() - desiredSize.*Minor() - static_cast<float>(minItemSpacing);
                    currentBounds.*MajorStart() = lineOffset;
                    lineMajorSize = std::max(lineMajorSize, currentBounds.*MajorSize());
                    lineNeedsReposition = previousElementBounds.*MajorSize() != currentBounds.*MajorSize();
                    countInLine++;
                }
            }

            m_elementManager.SetLayoutBoundsForDataIndex(currentIndex, currentBounds);

            REPEATER_TRACE_INFO(L"%*s: \tLayout bounds of element %d are (%.0f,%.0f,%.0f,%.0f). \n",
                winrt::get_self<VirtualizingLayoutContext>(m_context.get())->Indent(),
                layoutId.data(),
                currentIndex,
                currentBounds.X, currentBounds.Y, currentBounds.Width, currentBounds.Height);
            previousIndex = currentIndex;
            currentIndex += step;
        }

        // If we did not reach the top or bottom of the extent, we realized one
        // extra item before we knew we were outside the realization window. Do not
        // account for that element in the indicies inside the realization window.
        if (direction == GenerateDirection::Forward)
        {
            int dataCount = m_context.get().ItemCount();
            m_lastRealizedDataIndexInsideRealizationWindow = previousIndex == dataCount - 1 ? dataCount - 1 : previousIndex - 1;
            m_lastRealizedDataIndexInsideRealizationWindow = std::max(0, m_lastRealizedDataIndexInsideRealizationWindow);
        }
        else
        {
            int dataCount = m_context.get().ItemCount();
            m_firstRealizedDataIndexInsideRealizationWindow = previousIndex == 0 ? 0 : previousIndex + 1;
            m_firstRealizedDataIndexInsideRealizationWindow = std::min(dataCount - 1, m_firstRealizedDataIndexInsideRealizationWindow);
        }

        m_elementManager.DiscardElementsOutsideWindow(direction == GenerateDirection::Forward, currentIndex);
    }
}

bool FlowLayoutAlgorithm::IsReflowRequired() const
{
    // If first element is realized and is not at the very beginning we need to reflow.
    return
        m_elementManager.GetRealizedElementCount() > 0 &&
        m_elementManager.GetDataIndexFromRealizedRangeIndex(0) == 0 &&
        m_elementManager.GetLayoutBoundsForRealizedIndex(0).*MinorStart() != 0;
}

bool FlowLayoutAlgorithm::ShouldContinueFillingUpSpace(
    int index,
    GenerateDirection direction)
{
    bool shouldContinue = false;
    if (!IsVirtualizingContext())
    {
        shouldContinue = true;
    }
    else
    {
        auto realizationRect = m_context.get().RealizationRect();
        auto elementBounds = m_elementManager.GetLayoutBoundsForDataIndex(index);

        auto elementMajorStart = elementBounds.*MajorStart();
        auto elementMajorEnd = MajorEnd(elementBounds);
        auto rectMajorStart = realizationRect.*MajorStart();
        auto rectMajorEnd = MajorEnd(realizationRect);

        auto elementMinorStart = elementBounds.*MinorStart();
        auto elementMinorEnd = MinorEnd(elementBounds);
        auto rectMinorStart = realizationRect.*MinorStart();
        auto rectMinorEnd = MinorEnd(realizationRect);

        // Ensure that both minor and major directions are taken into consideration so that if the scrolling direction
        // is the same as the flow direction we still stop at the end of the viewport rectangle.
        shouldContinue =
            (direction == GenerateDirection::Forward && elementMajorStart < rectMajorEnd && elementMinorStart < rectMinorEnd) ||
            (direction == GenerateDirection::Backward && elementMajorEnd > rectMajorStart&& elementMinorEnd > rectMinorStart);
    }

    return shouldContinue;
}

winrt::Rect FlowLayoutAlgorithm::EstimateExtent(const winrt::Size& availableSize, const wstring_view& layoutId)
{
    winrt::UIElement firstRealizedElement = nullptr;
    winrt::Rect firstBounds{};
    winrt::UIElement lastRealizedElement = nullptr;
    winrt::Rect lastBounds{};
    int firstDataIndex = -1;
    int lastDataIndex = -1;

    if (m_elementManager.GetRealizedElementCount() > 0)
    {
        firstRealizedElement = m_elementManager.GetAt(0);
        firstBounds = m_elementManager.GetLayoutBoundsForRealizedIndex(0);
        firstDataIndex = m_elementManager.GetDataIndexFromRealizedRangeIndex(0);

        int last = m_elementManager.GetRealizedElementCount() - 1;
        lastRealizedElement = m_elementManager.GetAt(last);
        lastDataIndex = m_elementManager.GetDataIndexFromRealizedRangeIndex(last);
        lastBounds = m_elementManager.GetLayoutBoundsForRealizedIndex(last);
    }

    winrt::Rect extent = m_algorithmCallbacks->Algorithm_GetExtent(
        availableSize,
        m_context.get(),
        firstRealizedElement,
        firstDataIndex,
        firstBounds,
        lastRealizedElement,
        lastDataIndex,
        lastBounds);

    REPEATER_TRACE_INFO(L"%*s Extent: (%.0f,%.0f,%.0f,%.0f). \n", winrt::get_self<VirtualizingLayoutContext>(m_context.get())->Indent(), layoutId.data(), extent.X, extent.Y, extent.Width, extent.Height);
    return extent;
}

void FlowLayoutAlgorithm::RaiseLineArranged()
{
    auto realizationRect = RealizationRect();
    if (realizationRect.Width != 0.0f || realizationRect.Height != 0.0f)
    {
        int realizedElementCount = m_elementManager.GetRealizedElementCount();
        if (realizedElementCount > 0)
        {
            MUX_ASSERT(m_firstRealizedDataIndexInsideRealizationWindow != -1 && m_lastRealizedDataIndexInsideRealizationWindow != -1);
            int countInLine = 0;
            auto previousElementBounds = m_elementManager.GetLayoutBoundsForDataIndex(m_firstRealizedDataIndexInsideRealizationWindow);
            auto currentLineOffset = previousElementBounds.*MajorStart();
            auto currentLineSize = previousElementBounds.*MajorSize();
            for (int currentDataIndex = m_firstRealizedDataIndexInsideRealizationWindow; currentDataIndex <= m_lastRealizedDataIndexInsideRealizationWindow; currentDataIndex++)
            {
                auto currentBounds = m_elementManager.GetLayoutBoundsForDataIndex(currentDataIndex);
                if (currentBounds.*MajorStart() != currentLineOffset)
                {
                    // Staring a new line
                    m_algorithmCallbacks->Algorithm_OnLineArranged(currentDataIndex - countInLine, countInLine, currentLineSize, m_context.get());
                    countInLine = 0;
                    currentLineOffset = currentBounds.*MajorStart();
                    currentLineSize = 0;
                }

                currentLineSize = std::max(static_cast<float>(currentLineSize), currentBounds.*MajorSize());
                countInLine++;
                previousElementBounds = currentBounds;
            }

            // Raise for the last line.
            m_algorithmCallbacks->Algorithm_OnLineArranged(m_lastRealizedDataIndexInsideRealizationWindow - countInLine + 1, countInLine, currentLineSize, m_context.get());
        }
    }
}

#pragma endregion

#pragma region Arrange related private methods

void FlowLayoutAlgorithm::ArrangeVirtualizingLayout(
    const winrt::Size& finalSize,
    FlowLayoutAlgorithm::LineAlignment lineAlignment,
    bool isWrapping,
    const wstring_view& layoutId)
{
    // Walk through the realized elements one line at a time and
    // align them, Then call element.Arrange with the arranged bounds.
    int realizedElementCount = m_elementManager.GetRealizedElementCount();
    if (realizedElementCount > 0)
    {
        int countInLine = 1;
        auto previousElementBounds = m_elementManager.GetLayoutBoundsForRealizedIndex(0);
        auto currentLineOffset = previousElementBounds.*MajorStart();
        auto spaceAtLineStart = previousElementBounds.*MinorStart();
        float spaceAtLineEnd = 0;
        float currentLineSize = previousElementBounds.*MajorSize();
        for (int i = 1; i < realizedElementCount; i++)
        {
            auto currentBounds = m_elementManager.GetLayoutBoundsForRealizedIndex(i);
            if (currentBounds.*MajorStart() != currentLineOffset)
            {
                spaceAtLineEnd = finalSize.*Minor() - previousElementBounds.*MinorStart() - previousElementBounds.*MinorSize();
                PerformLineAlignment(i - countInLine, countInLine, spaceAtLineStart, spaceAtLineEnd, currentLineSize, lineAlignment, isWrapping, finalSize, layoutId);
                spaceAtLineStart = currentBounds.*MinorStart();
                countInLine = 0;
                currentLineOffset = currentBounds.*MajorStart();
                currentLineSize = 0;
            }

            countInLine++; // for current element
            currentLineSize = std::max(currentLineSize, currentBounds.*MajorSize());
            previousElementBounds = currentBounds;
        }

        // Last line - potentially have a property to customize
        // aligning the last line or not.
        if (countInLine > 0)
        {
            float spaceAtEnd = finalSize.*Minor() - previousElementBounds.*MinorStart() - previousElementBounds.*MinorSize();
            PerformLineAlignment(realizedElementCount - countInLine, countInLine, spaceAtLineStart, spaceAtEnd, currentLineSize, lineAlignment, isWrapping, finalSize, layoutId);
        }
    }
}

// Align elements within a line. Note that this does not modify LayoutBounds. So if we get
// repeated measures, the LayoutBounds remain the same in each layout.
void FlowLayoutAlgorithm::PerformLineAlignment(
    int lineStartIndex,
    int countInLine,
    float spaceAtLineStart,
    float spaceAtLineEnd,
    float lineSize,
    FlowLayoutAlgorithm::LineAlignment lineAlignment,
    bool isWrapping,
    const winrt::Size& finalSize,
    const wstring_view& layoutId)
{
    for (int rangeIndex = lineStartIndex; rangeIndex < lineStartIndex + countInLine; ++rangeIndex)
    {
        auto bounds = m_elementManager.GetLayoutBoundsForRealizedIndex(rangeIndex);
        bounds.*MajorSize() = lineSize;

        if (!m_scrollOrientationSameAsFlow)
        {
            // Note: Space at start could potentially be negative
            if (spaceAtLineStart != 0 || spaceAtLineEnd != 0)
            {
                float totalSpace = spaceAtLineStart + spaceAtLineEnd;
                switch (lineAlignment)
                {
                case FlowLayoutAlgorithm::LineAlignment::Start:
                {
                    bounds.*MinorStart() -= spaceAtLineStart;
                    break;
                }

                case FlowLayoutAlgorithm::LineAlignment::End:
                {
                    bounds.*MinorStart() += spaceAtLineEnd;
                    break;
                }

                case FlowLayoutAlgorithm::LineAlignment::Center:
                {
                    bounds.*MinorStart() -= spaceAtLineStart;
                    bounds.*MinorStart() += totalSpace / 2;
                    break;
                }

                case FlowLayoutAlgorithm::LineAlignment::SpaceAround:
                {
                    float interItemSpace = countInLine >= 1 ? totalSpace / (countInLine * 2) : 0;
                    bounds.*MinorStart() -= spaceAtLineStart;
                    bounds.*MinorStart() += interItemSpace * ((rangeIndex - lineStartIndex + 1) * 2 - 1);
                    break;
                }

                case FlowLayoutAlgorithm::LineAlignment::SpaceBetween:
                {
                    float interItemSpace = countInLine > 1 ? totalSpace / (countInLine - 1) : 0;
                    bounds.*MinorStart() -= spaceAtLineStart;
                    bounds.*MinorStart() += interItemSpace * (rangeIndex - lineStartIndex);
                    break;
                }

                case FlowLayoutAlgorithm::LineAlignment::SpaceEvenly:
                {
                    float interItemSpace = countInLine >= 1 ? totalSpace / (countInLine + 1) : 0;
                    bounds.*MinorStart() -= spaceAtLineStart;
                    bounds.*MinorStart() += interItemSpace * (rangeIndex - lineStartIndex + 1);
                    break;
                }
                }
            }
        }

        bounds.X -= m_lastExtent.X;
        bounds.Y -= m_lastExtent.Y;

        if (!isWrapping)
        {
            bounds.*MinorSize() = std::max(bounds.*MinorSize(), finalSize.*Minor());
        }

        auto element = m_elementManager.GetAt(rangeIndex);

        REPEATER_TRACE_INFO(L"%*s: \tArranging element %d at (%.0f,%.0f,%.0f,%.0f). \n",
            winrt::get_self<VirtualizingLayoutContext>(m_context.get())->Indent(),
            layoutId.data(),
            m_elementManager.GetDataIndexFromRealizedRangeIndex(rangeIndex),
            bounds.X, bounds.Y, bounds.Width, bounds.Height);
        element.Arrange(bounds);
    }
}

#pragma endregion

#pragma region Layout Context Helpers

winrt::Rect FlowLayoutAlgorithm::RealizationRect()
{
    return IsVirtualizingContext() ?
        m_context.get().RealizationRect() :
        winrt::Rect{ 0, 0, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
}

void FlowLayoutAlgorithm::SetLayoutOrigin()
{
    if (IsVirtualizingContext())
    {
        m_context.get().LayoutOrigin(winrt::Point{ m_lastExtent.X, m_lastExtent.Y });
    }
    else
    {
        // Should have 0 origin for non-virtualizing layout since we always start from
        // the first item
        MUX_ASSERT(m_lastExtent.X == 0 && m_lastExtent.Y == 0);
    }
}

winrt::UIElement FlowLayoutAlgorithm::GetElementIfRealized(int dataIndex)
{
    if (m_elementManager.IsDataIndexRealized(dataIndex))
    {
        return m_elementManager.GetRealizedElement(dataIndex);
    }

    return  nullptr;
}

bool FlowLayoutAlgorithm::TryAddElement0(winrt::UIElement const& element)
{
    if (m_elementManager.GetRealizedElementCount() == 0)
    {
        m_elementManager.Add(element, 0);
        return true;
    }

    return false;
}

bool FlowLayoutAlgorithm::IsVirtualizingContext()
{
    if (m_context)
    {
        auto rect = m_context.get().RealizationRect();
        bool hasInfiniteSize = (rect.Height == std::numeric_limits<float>::infinity() || rect.Width == std::numeric_limits<float>::infinity());
        return !hasInfiniteSize;
    }
    return false;
}

#pragma endregion
