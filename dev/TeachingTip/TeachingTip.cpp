﻿#include "pch.h"
#include "common.h"
#include "TeachingTip.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "TeachingTipClosingEventArgs.h"
#include "TeachingTipClosedEventArgs.h"
#include "TeachingTipTestHooks.h"
#include "TeachingTipAutomationPeer.h"

TeachingTip::TeachingTip()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TeachingTip);
    SetDefaultStyleKey(this);
    EnsureProperties();
    SetValue(s_TemplateSettingsProperty, winrt::make<::TeachingTipTemplateSettings>());
}

winrt::AutomationPeer TeachingTip::OnCreateAutomationPeer()
{
    return winrt::make<TeachingTipAutomationPeer>(*this);
}

void TeachingTip::OnApplyTemplate()
{
    m_effectiveViewportChangedRevoker.revoke();
    m_contentSizeChangedRevoker.revoke();
    m_closeButtonClickedRevoker.revoke();
    m_alternateCloseButtonClickedRevoker.revoke();
    m_actionButtonClickedRevoker.revoke();

    winrt::IControlProtected controlProtected{ *this };

    m_container.set(GetTemplateChildT<winrt::Border>(s_containerName, controlProtected));
    m_rootElement.set(m_container.get().Child());
    m_beakOcclusionGrid.set(GetTemplateChildT<winrt::Grid>(s_beakOcclusionGridName, controlProtected));
    m_contentRootGrid.set(GetTemplateChildT<winrt::Grid>(s_contentRootGridName, controlProtected));
    m_nonBleedingContentRootGrid.set(GetTemplateChildT<winrt::Grid>(s_nonBleedingContentRootGridName, controlProtected));
    m_bleedingImageContentBorder.set(GetTemplateChildT<winrt::Border>(s_bleedingImageBorderName, controlProtected));
    m_iconBorder.set(GetTemplateChildT<winrt::Border>(s_iconBorderName, controlProtected));
    m_actionButton.set(GetTemplateChildT<winrt::Button>(s_actionButtonName, controlProtected));
    m_alternateCloseButton.set(GetTemplateChildT<winrt::Button>(s_alternateCloseButtonName, controlProtected));
    m_closeButton.set(GetTemplateChildT<winrt::Button>(s_closeButtonName, controlProtected));
    m_beakEdgeBorder.set(GetTemplateChildT<winrt::Grid>(s_beakEdgeBorderName, controlProtected));
    m_beakPolygon.set(GetTemplateChildT<winrt::Polygon>(s_beakPolygonName, controlProtected));

    if (auto && container = m_container.get())
    {
        container.Child(nullptr);
    }

    if (auto&& beakOcclusionGrid = m_beakOcclusionGrid.get())
    {
        m_contentSizeChangedRevoker = beakOcclusionGrid.SizeChanged(winrt::auto_revoke, {
            [this](auto const&, auto const&)
            {
                UpdateSizeBasedTemplateSettings();
                // Reset the currentEffectivePlacementMode so that the beak will be updated for the new size as well.
                m_currentEffectivePlacementMode = winrt::TeachingTipPlacementMode::Auto;
                TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
                if (IsOpen())
                {
                    PositionPopup();
                }
                {
                    auto&& beakOcclusionGrid = m_beakOcclusionGrid.get();
                    if (auto&& expandAnimation = m_expandAnimation.get())
                    {
                        expandAnimation.SetScalarParameter(L"Width", static_cast<float>(beakOcclusionGrid.ActualWidth()));
                        expandAnimation.SetScalarParameter(L"Height", static_cast<float>(beakOcclusionGrid.ActualHeight()));
                    }
                    if (auto&& contractAnimation = m_contractAnimation.get())
                    {
                        contractAnimation.SetScalarParameter(L"Width", static_cast<float>(beakOcclusionGrid.ActualWidth()));
                        contractAnimation.SetScalarParameter(L"Height", static_cast<float>(beakOcclusionGrid.ActualHeight()));
                    }
                }
            }
        });
    }

    if (auto&& closeButton = m_closeButton.get())
    {
        m_closeButtonClickedRevoker = closeButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnCloseButtonClicked });
    }
    if (auto&& alternateCloseButton = m_alternateCloseButton.get())
    {
        winrt::AutomationProperties::SetName(alternateCloseButton, ResourceAccessor::GetLocalizedStringResource(SR_TeachingTipAlternateCloseButtonName));
        m_alternateCloseButtonClickedRevoker = alternateCloseButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnCloseButtonClicked });
    }

    if (auto&& actionButton = m_actionButton.get())
    {
        m_actionButtonClickedRevoker = actionButton.Click(winrt::auto_revoke, {this, &TeachingTip::OnActionButtonClicked });
    }
    UpdateButtonsState();
    OnIconSourceChanged();
    EstablishShadows();

    m_isTemplateApplied = true;
}

void TeachingTip::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsOpenProperty)
    {
        OnIsOpenChanged();
    }
    else if (property == s_ActionButtonTextProperty ||
        property == s_CloseButtonKindProperty  ||
        property == s_CloseButtonTextProperty)
    {
        UpdateButtonsState();
    }
    else if (property == s_TargetOffsetProperty)
    {
        OnTargetOffsetChanged();
    }
    else if (property == s_IsLightDismissEnabledProperty)
    {
        OnIsLightDismissEnabledChanged();
    }
    else if (property == s_PlacementProperty)
    {
        if (IsOpen())
        {
            PositionPopup();
        }
    }
    else if (property == s_BleedingImagePlacementProperty)
    {
        OnBleedingImagePlacementChanged();
    }
    else if (property == s_IconSourceProperty)
    {
        OnIconSourceChanged();
    }
}

void TeachingTip::OnContentChanged(const winrt::IInspectable& oldContent, const winrt::IInspectable& newContent)
{
    if (newContent)
    {
        winrt::VisualStateManager::GoToState(*this, L"Content"sv, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NoContent"sv, false);
    }
}

// Playing a closing animation when the Teaching Tip is closed via light dismiss requires this work around.
// This is because there is no event that occurs when a popup is closing due to light dismiss so we have no way to intercept
// the close and play our animation first. To work around this we've created a second popup which has no content and sits
// underneath the teaching tip and is put into light dismiss mode instead of the primary popup. Then when this popup closes
// due to light dismiss we know we are supposed to close the primary popup as well. To ensure that this popup does not block
// interaction to the primary popup we need to make sure that the LightDismissIndicatorPopup is always opened first, so that
// it is Z ordered underneath the primary popup.
void TeachingTip::CreateLightDismissIndicatorPopup()
{
    if (!m_lightDismissIndicatorPopup)
    {
        auto popup = winrt::Popup();
        // A Popup needs contents to open, so set a child that doesn't do anything.
        auto grid = winrt::Grid();
        popup.Child(grid);

        m_lightDismissIndicatorPopup.set(popup);
    }
}

void TeachingTip::UpdateBeak()
{
    auto&& beakOcclusionGrid = m_beakOcclusionGrid.get();
    auto&& beakEdgeBorder = m_beakEdgeBorder.get();

    float height = static_cast<float>(beakOcclusionGrid.ActualHeight());
    float width = static_cast<float>(beakOcclusionGrid.ActualWidth());

    auto columnDefinitions = beakOcclusionGrid.ColumnDefinitions();
    auto rowDefinitions = beakOcclusionGrid.RowDefinitions();

    float firstColumnWidth = static_cast<float>(columnDefinitions.GetAt(0).ActualWidth());
    float secondColumnWidth = static_cast<float>(columnDefinitions.GetAt(1).ActualWidth());
    float nextToLastColumnWidth = static_cast<float>(columnDefinitions.GetAt(columnDefinitions.Size() - 2).ActualWidth());
    float lastColumnWidth = static_cast<float>(columnDefinitions.GetAt(columnDefinitions.Size() - 1).ActualWidth());

    float firstRowHeight = static_cast<float>(rowDefinitions.GetAt(0).ActualHeight());
    float secondRowHeight = static_cast<float>(rowDefinitions.GetAt(1).ActualHeight());
    float nextToLastRowHeight = static_cast<float>(rowDefinitions.GetAt(rowDefinitions.Size() - 2).ActualHeight());
    float lastRowHeight = static_cast<float>(rowDefinitions.GetAt(rowDefinitions.Size() - 1).ActualHeight());

    UpdateSizeBasedTemplateSettings();

    switch (m_currentEffectivePlacementMode)
    {
    // An effective placement of auto means the tip should not display a beak.
    case winrt::TeachingTipPlacementMode::Auto:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width / 2, height / 2, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Untargeted"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Top:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width / 2, height - lastRowHeight, 0.0f });
            beakEdgeBorder.CenterPoint({ (width / 2) - firstColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Top"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Bottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width / 2, firstRowHeight, 0.0f });
            beakEdgeBorder.CenterPoint({ (width / 2) - firstColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"Bottom"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Left:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width - lastColumnWidth, (height / 2), 0.0f });
            beakEdgeBorder.CenterPoint({ 0.0f, (height / 2) - firstRowHeight, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Left"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::Right:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ firstColumnWidth, height / 2, 0.0f });
            beakEdgeBorder.CenterPoint({ 0.0f, (height / 2) - firstRowHeight, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"Right"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ firstColumnWidth + secondColumnWidth + 1, height - lastRowHeight, 0.0f });
            beakEdgeBorder.CenterPoint({ secondColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"TopEdgeAlignedRight"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width - (nextToLastColumnWidth + lastColumnWidth + 1), height - lastRowHeight, 0.0f });
            beakEdgeBorder.CenterPoint({ width - (nextToLastColumnWidth + firstColumnWidth + lastColumnWidth), 0.0f, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"TopEdgeAlignedLeft"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ firstColumnWidth + secondColumnWidth + 1, firstRowHeight, 0.0f });
            beakEdgeBorder.CenterPoint({ secondColumnWidth, 0.0f, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"BottomEdgeAlignedRight"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width - (nextToLastColumnWidth + lastColumnWidth + 1), firstRowHeight, 0.0f });
            beakEdgeBorder.CenterPoint({ width - (nextToLastColumnWidth + firstColumnWidth + lastColumnWidth), 0.0f, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"BottomEdgeAlignedLeft"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width - lastColumnWidth,  height - (nextToLastRowHeight + lastRowHeight + 1), 0.0f });
            beakEdgeBorder.CenterPoint({ 0.0f,  height - (nextToLastRowHeight + firstRowHeight + lastRowHeight), 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"LeftEdgeAlignedTop"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ width - lastColumnWidth, (firstRowHeight + secondRowHeight + 1), 0.0f });
            beakEdgeBorder.CenterPoint({ 0.0f, secondRowHeight, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"LeftEdgeAlignedBottom"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ firstColumnWidth, height - (nextToLastRowHeight + lastRowHeight + 1), 0.0f });
            beakEdgeBorder.CenterPoint({ 0.0f, height - (nextToLastRowHeight + firstRowHeight + lastRowHeight), 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToTop();
        winrt::VisualStateManager::GoToState(*this, L"RightEdgeAlignedTop"sv, false);
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        if (SharedHelpers::IsRS5OrHigher())
        {
            beakOcclusionGrid.CenterPoint({ firstColumnWidth, (firstRowHeight + secondRowHeight + 1), 0.0f });
            beakEdgeBorder.CenterPoint({ 0.0f, secondRowHeight, 0.0f });
        }
        UpdateDynamicBleedingContentPlacementToBottom();
        winrt::VisualStateManager::GoToState(*this, L"RightEdgeAlignedBottom"sv, false);
        break;

    default:
        break;
    }
}

void TeachingTip::PositionPopup()
{
    if (m_target)
    {
        PositionTargetedPopup();
    }
    else
    {
        PositionUntargetedPopup();
    }
    TeachingTipTestHooks::NotifyOffsetChanged(*this);
}

void TeachingTip::PositionTargetedPopup()
{
    if (auto&& popup = m_popup.get())
    {
        auto placement = DetermineEffectivePlacement();
        auto offset = TargetOffset();

        auto&& beakOcclusionGrid = m_beakOcclusionGrid.get();
        double tipHeight = beakOcclusionGrid.ActualHeight();
        double tipWidth = beakOcclusionGrid.ActualWidth();

        // Depending on the effective placement mode of the tip we use a combination of the tip's size, the target's position within the app, the target's
        // size, and the target offset property to determine the appropriate vertical and horizontal offsets of the popup that the tip is contained in.
        switch (placement)
        {
        case winrt::TeachingTipPlacementMode::Top:
            popup.VerticalOffset(m_currentTargetBounds.Y - tipHeight - offset.Top);
            popup.HorizontalOffset((((m_currentTargetBounds.X * 2)  + m_currentTargetBounds.Width - tipWidth) / 2));
            break;

        case winrt::TeachingTipPlacementMode::Bottom:
            popup.VerticalOffset(m_currentTargetBounds.Y + m_currentTargetBounds.Height + offset.Bottom);
            popup.HorizontalOffset((((m_currentTargetBounds.X * 2) + m_currentTargetBounds.Width - tipWidth) / 2));
            break;

        case winrt::TeachingTipPlacementMode::Left:
            popup.VerticalOffset(((m_currentTargetBounds.Y * 2) + m_currentTargetBounds.Height - tipHeight) / 2);
            popup.HorizontalOffset(m_currentTargetBounds.X - tipWidth - offset.Left);
            break;

        case winrt::TeachingTipPlacementMode::Right:
            popup.VerticalOffset(((m_currentTargetBounds.Y * 2) + m_currentTargetBounds.Height - tipHeight) / 2);
            popup.HorizontalOffset(m_currentTargetBounds.X + m_currentTargetBounds.Width + offset.Right);
            break;

        case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
            popup.VerticalOffset(m_currentTargetBounds.Y - tipHeight - offset.Top);
            popup.HorizontalOffset(((((m_currentTargetBounds.X  * 2) + m_currentTargetBounds.Width) / 2) - MinimumTipEdgeToBeakCenter()));
            break;

        case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
            popup.VerticalOffset(m_currentTargetBounds.Y - tipHeight - offset.Top);
            popup.HorizontalOffset(((((m_currentTargetBounds.X  * 2) + m_currentTargetBounds.Width) / 2) - tipWidth + MinimumTipEdgeToBeakCenter()));
            break;

        case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
            popup.VerticalOffset(m_currentTargetBounds.Y + m_currentTargetBounds.Height + offset.Bottom);
            popup.HorizontalOffset(((((m_currentTargetBounds.X * 2) + m_currentTargetBounds.Width) / 2) - MinimumTipEdgeToBeakCenter()));
            break;

        case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
            popup.VerticalOffset(m_currentTargetBounds.Y + m_currentTargetBounds.Height + offset.Bottom);
            popup.HorizontalOffset(((((m_currentTargetBounds.X * 2) + m_currentTargetBounds.Width) / 2) - tipWidth + MinimumTipEdgeToBeakCenter()));
            break;

        case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
            popup.VerticalOffset((((m_currentTargetBounds.Y * 2) + m_currentTargetBounds.Height) / 2) - tipHeight + MinimumTipEdgeToBeakCenter());
            popup.HorizontalOffset(m_currentTargetBounds.X - tipWidth - offset.Left);
            break;

        case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
            popup.VerticalOffset((((m_currentTargetBounds.Y * 2) + m_currentTargetBounds.Height) / 2) - MinimumTipEdgeToBeakCenter());
            popup.HorizontalOffset(m_currentTargetBounds.X - tipWidth - offset.Left);
            break;

        case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
            popup.VerticalOffset((((m_currentTargetBounds.Y * 2) + m_currentTargetBounds.Height) / 2) - tipHeight + MinimumTipEdgeToBeakCenter());
            popup.HorizontalOffset(m_currentTargetBounds.X + m_currentTargetBounds.Width + offset.Right);
            break;

        case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
            popup.VerticalOffset((((m_currentTargetBounds.Y * 2) + m_currentTargetBounds.Height) / 2) - MinimumTipEdgeToBeakCenter());
            popup.HorizontalOffset(m_currentTargetBounds.X + m_currentTargetBounds.Width + offset.Right);
            break;

        default:
            MUX_FAIL_FAST();
        }

        if (placement != m_currentEffectivePlacementMode)
        {
            m_currentEffectivePlacementMode = placement;
            TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
            UpdateBeak();
        }
    }
}

void TeachingTip::PositionUntargetedPopup()
{
    auto windowBounds = m_useTestWindowBounds ? m_testWindowBounds : winrt::Window::Current().CoreWindow().Bounds();

    auto&& beakOcclusionGrid = m_beakOcclusionGrid.get();
    double finalTipHeight = beakOcclusionGrid.ActualHeight();
    double finalTipWidth = beakOcclusionGrid.ActualWidth();

    // An effective placement of auto indicates that no beak should be shown.
    m_currentEffectivePlacementMode = winrt::TeachingTipPlacementMode::Auto;
    TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
    UpdateBeak();

    auto offset = TargetOffset();

    // Depending on the effective placement mode of the tip we use a combination of the tip's size, the window's size, and the target
    // offset property to determine the appropriate vertical and horizontal offsets of the popup that the tip is contained in.
    auto&& popup = m_popup.get();
    switch (Placement())
    {
    case winrt::TeachingTipPlacementMode::Auto:
    case winrt::TeachingTipPlacementMode::Bottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height, finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Top:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Width, finalTipWidth, offset.Left, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::Left:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::Right:
        popup.VerticalOffset(UntargetedTipCenterPlacementOffset(windowBounds.Height, finalTipHeight, offset.Top, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width,finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width,finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height,finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipNearPlacementOffset(offset.Left));
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        popup.VerticalOffset(UntargetedTipNearPlacementOffset(offset.Top));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width, finalTipWidth, offset.Right));
        break;

    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        popup.VerticalOffset(UntargetedTipFarPlacementOffset(windowBounds.Height, finalTipHeight, offset.Bottom));
        popup.HorizontalOffset(UntargetedTipFarPlacementOffset(windowBounds.Width, finalTipWidth, offset.Right));
        break;

    default:
        MUX_FAIL_FAST();
    }
}

void TeachingTip::UpdateSizeBasedTemplateSettings()
{
    auto templateSettings = winrt::get_self<::TeachingTipTemplateSettings>(TemplateSettings());
    auto&& contentRootGrid = m_contentRootGrid.get();
    auto width = contentRootGrid.ActualWidth();
    auto height = contentRootGrid.ActualHeight();
    auto floatWidth = static_cast<float>(width);
    auto floatHeight = static_cast<float>(height);
    switch (m_currentEffectivePlacementMode)
    {
    case winrt::TeachingTipPlacementMode::Top:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Bottom:
        templateSettings->TopRightHighlightMargin(BottomPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Left:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Right:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::TopEdgeAlignedRight:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft:
        templateSettings->TopRightHighlightMargin(BottomEdgeAlignedLeftPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomEdgeAlignedLeftPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight:
        templateSettings->TopRightHighlightMargin(BottomEdgeAlignedRightPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(BottomEdgeAlignedRightPlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(LeftEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::RightEdgeAlignedTop:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(RightEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    case winrt::TeachingTipPlacementMode::Auto:
        templateSettings->TopRightHighlightMargin(OtherPlacementTopRightHighlightMargin(width, height));
        templateSettings->TopLeftHighlightMargin(TopEdgePlacementTopLeftHighlightMargin(width, height));
        break;
    }
}

void TeachingTip::UpdateButtonsState()
{
    hstring actionText = ActionButtonText();
    hstring closeText = CloseButtonText();
    switch (CloseButtonKind())
    {
    case winrt::TeachingTipCloseButtonKind::Auto:
        if (actionText.size() > 0 && closeText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"BothButtonsVisible"sv, false);
            winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
        }
        else if (actionText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"ActionButtonVisible"sv, false);
            winrt::VisualStateManager::GoToState(*this, L"HeaderCloseButton"sv, false);
        }
        else if (closeText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"CloseButtonVisible"sv, false);
            winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, L"NoButtonsVisible"sv, false);
            winrt::VisualStateManager::GoToState(*this, L"HeaderCloseButton"sv, false);
        }
        break;
    case winrt::TeachingTipCloseButtonKind::Header:
        winrt::VisualStateManager::GoToState(*this, L"HeaderCloseButton"sv, false);
        if (actionText.size() > 0 && closeText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"BothButtonsVisible"sv, false);
        }
        else if (actionText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"ActionButtonVisible"sv, false);
        }
        else if (closeText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"CloseButtonVisible"sv, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, L"NoButtonsVisible"sv, false);
        }
        break;
    case winrt::TeachingTipCloseButtonKind::Footer:
        winrt::VisualStateManager::GoToState(*this, L"FooterCloseButton"sv, false);
        if (actionText.size() > 0 && closeText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"BothButtonsVisible"sv, false);
        }
        else if (actionText.size() > 0)
        {
            if (IsLightDismissEnabled())
            {
                winrt::VisualStateManager::GoToState(*this, L"ActionButtonVisible"sv, false);
            }
            else
            {
                // Without light dismiss we require that at least one close button be shown at all times.
                winrt::VisualStateManager::GoToState(*this, L"BothButtonsVisible"sv, false);
            }
        }
        else if (closeText.size() > 0)
        {
            winrt::VisualStateManager::GoToState(*this, L"CloseButtonVisible"sv, false);
        }
        else
        {
            if (IsLightDismissEnabled())
            {
                winrt::VisualStateManager::GoToState(*this, L"NoButtonsVisible"sv, false);
            }
            else
            {
                // We require that at least one close button be shown at all times.
                winrt::VisualStateManager::GoToState(*this, L"CloseButtonVisible"sv, false);
            }
        }
        break;
    }
}

void TeachingTip::UpdateDynamicBleedingContentPlacementToTop()
{
    if (BleedingImagePlacement() == winrt::TeachingTipBleedingImagePlacementMode::Auto)
    {
        winrt::VisualStateManager::GoToState(*this, L"BleedingContentTop"sv, false);
        if (m_currentBleedingEffectivePlacementMode != winrt::TeachingTipBleedingImagePlacementMode::Top)
        {
            m_currentBleedingEffectivePlacementMode = winrt::TeachingTipBleedingImagePlacementMode::Top;
            TeachingTipTestHooks::NotifyEffectiveBleedingPlacementChanged(*this);
        }
    }
}

void TeachingTip::UpdateDynamicBleedingContentPlacementToBottom()
{
    if (BleedingImagePlacement() == winrt::TeachingTipBleedingImagePlacementMode::Auto)
    {
        winrt::VisualStateManager::GoToState(*this, L"BleedingContentBottom"sv, false);
        if (m_currentBleedingEffectivePlacementMode != winrt::TeachingTipBleedingImagePlacementMode::Bottom)
        {
            m_currentBleedingEffectivePlacementMode = winrt::TeachingTipBleedingImagePlacementMode::Bottom;
            TeachingTipTestHooks::NotifyEffectiveBleedingPlacementChanged(*this);
        }
    }
}

void TeachingTip::OnIsOpenChanged()
{
    if (IsOpen())
    {
        //Reset the close reason to the default value of programmatic.
        m_lastCloseReason = winrt::TeachingTipCloseReason::Programmatic;

        m_currentBounds = this->TransformToVisual(nullptr).TransformBounds({
            0.0,
            0.0,
            static_cast<float>(this->ActualWidth()),
            static_cast<float>(this->ActualHeight())
            });

        if (auto&& target = m_target.get())
        {
            SetViewportChangedEvent();
            m_currentTargetBounds = target.TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualWidth()),
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualHeight())
                });
        }

        if (!m_lightDismissIndicatorPopup)
        {
            CreateLightDismissIndicatorPopup();
        }
        OnIsLightDismissEnabledChanged();

        if (!m_contractAnimation)
        {
            CreateContractAnimation();
        }
        if (!m_expandAnimation)
        {
            CreateExpandAnimation();
        }

        // We are about to begin the process of trying to open the teaching tip, so notify that we are no longer idle.
        if (m_isIdle)
        {
            m_isIdle = false;
            TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
        }

        if (!m_isTemplateApplied)
        {
            this->ApplyTemplate();
        }

        if (!m_popup)
        {
            auto popup = winrt::Popup();
            m_popupOpenedRevoker = popup.Opened(winrt::auto_revoke, { this, &TeachingTip::OnPopupOpened });
            m_popupClosedRevoker = popup.Closed(winrt::auto_revoke, { this, &TeachingTip::OnPopupClosed });
            m_popup.set(popup);
        }

        auto&& popup = m_popup.get();
        if (!popup.IsOpen())
        {
            popup.Child(m_rootElement.get());
            m_lightDismissIndicatorPopup.get().IsOpen(true);
            popup.IsOpen(true);
        }
        else
        {
            // We have become Open but our popup was already open. This can happen when a close is canceled by the closing event, so make sure the idle status is correct.
            if (!m_isIdle && !m_isExpandAnimationPlaying && !m_isContractAnimationPlaying)
            {
                m_isIdle = true;
                TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
            }
        }
    }
    else
    {
        if (auto&& popup = m_popup.get())
        {
            if (popup.IsOpen())
            {
                // We are about to begin the process of trying to close the teaching tip, so notify that we are no longer idle.
                if (m_isIdle)
                {
                    m_isIdle = false;
                    TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
                }
                RaiseClosingEvent();
            }
            else
            {
                // We have become not Open but our popup was already not open. Lets make sure the idle status is correct.
                if (!m_isIdle && !m_isExpandAnimationPlaying && !m_isContractAnimationPlaying)
                {
                    m_isIdle = true;
                    TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
                }
            }
        }

        m_currentEffectivePlacementMode = winrt::TeachingTipPlacementMode::Auto;
        TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
    }
    TeachingTipTestHooks::NotifyOpenedStatusChanged(*this);
}

void TeachingTip::OnIconSourceChanged()
{
    auto templateSettings = winrt::get_self<::TeachingTipTemplateSettings>(TemplateSettings());
    if (auto source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        winrt::VisualStateManager::GoToState(*this, L"Icon"sv, false);
    }
    else
    {
        templateSettings->IconElement(nullptr);
        winrt::VisualStateManager::GoToState(*this, L"NoIcon"sv, false);
    }
}

void TeachingTip::OnTargetOffsetChanged()
{
    if (IsOpen())
    {
        PositionPopup();
    }
}

void TeachingTip::OnIsLightDismissEnabledChanged()
{
    if (IsLightDismissEnabled())
    {
        winrt::VisualStateManager::GoToState(*this, L"LightDismiss"sv, false);
        if (auto&& lightDismissIndicatorPopup = m_lightDismissIndicatorPopup.get())
        {
            lightDismissIndicatorPopup.IsLightDismissEnabled(true);
            m_lightDismissIndicatorPopupClosedRevoker = lightDismissIndicatorPopup.Closed(winrt::auto_revoke, { this, &TeachingTip::OnLightDismissIndicatorPopupClosed });
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NormalDismiss"sv, false);
        if (auto&& lightDismissIndicatorPopup = m_lightDismissIndicatorPopup.get())
        {
            lightDismissIndicatorPopup.IsLightDismissEnabled(false);
        }
        m_lightDismissIndicatorPopupClosedRevoker.revoke();
    }
}

void TeachingTip::OnBleedingImagePlacementChanged()
{
    switch (BleedingImagePlacement())
    {
    case winrt::TeachingTipBleedingImagePlacementMode::Auto:
        break;
    case winrt::TeachingTipBleedingImagePlacementMode::Top:
        winrt::VisualStateManager::GoToState(*this, L"BleedingContentTop"sv, false);
        if (m_currentBleedingEffectivePlacementMode != winrt::TeachingTipBleedingImagePlacementMode::Top)
        {
            m_currentBleedingEffectivePlacementMode = winrt::TeachingTipBleedingImagePlacementMode::Top;
            TeachingTipTestHooks::NotifyEffectiveBleedingPlacementChanged(*this);
        }
        break;
    case winrt::TeachingTipBleedingImagePlacementMode::Bottom:
        winrt::VisualStateManager::GoToState(*this, L"BleedingContentBottom"sv, false);
        if (m_currentBleedingEffectivePlacementMode != winrt::TeachingTipBleedingImagePlacementMode::Bottom)
        {
            m_currentBleedingEffectivePlacementMode = winrt::TeachingTipBleedingImagePlacementMode::Bottom;
            TeachingTipTestHooks::NotifyEffectiveBleedingPlacementChanged(*this);
        }
        break;
    }

    // Setting m_currentEffectivePlacementMode to auto ensures that the next time position popup is called we'll rerun the DetermineEffectivePlacement
    // alogorithm. If we did not do this and the popup was opened the algorithm would maintain the current effective placement mode, which we don't want
    // since the bleeding image placement contributes to the choice of tip placement mode.
    m_currentEffectivePlacementMode = winrt::TeachingTipPlacementMode::Auto;
    TeachingTipTestHooks::NotifyEffectivePlacementChanged(*this);
    if (IsOpen())
    {
        PositionPopup();
    }
}

void TeachingTip::OnCloseButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    m_closeButtonClickEventSource(*this, nullptr);
    m_lastCloseReason = winrt::TeachingTipCloseReason::CloseButton;
    IsOpen(false);
}

void TeachingTip::OnActionButtonClicked(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    m_actionButtonClickEventSource(*this, nullptr);
}

void TeachingTip::OnPopupOpened(const winrt::IInspectable&, const winrt::IInspectable&)
{
    StartExpandToOpen();
}

void TeachingTip::OnPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&)
{
    m_lightDismissIndicatorPopup.get().IsOpen(false);
    m_popup.get().Child(nullptr);
    auto myArgs = winrt::make_self<TeachingTipClosedEventArgs>();
    myArgs->Reason(m_lastCloseReason);
    m_closedEventSource(*this, *myArgs);
}

void TeachingTip::OnLightDismissIndicatorPopupClosed(const winrt::IInspectable&, const winrt::IInspectable&)
{
    if (IsOpen())
    {
        m_lastCloseReason = winrt::TeachingTipCloseReason::LightDismiss;
    }
    IsOpen(false);
}

void TeachingTip::OnBeakOcclusionGridLoaded(const winrt::IInspectable&, const winrt::IInspectable&)
{
    StartExpandToOpen();
    m_beakOcclusionGridLoadedRevoker.revoke();
}


void TeachingTip::RaiseClosingEvent()
{
    auto args = winrt::make_self<TeachingTipClosingEventArgs>();
    args->Reason(m_lastCloseReason);

    com_ptr<TeachingTip> strongThis = get_strong();
    winrt::DeferralCompletedHandler instance{ [strongThis, args]()
        {
            strongThis->CheckThread();
            if (!args->Cancel())
            {
                strongThis->ClosePopupWithAnimationIfAvailable();
            }
            else
            {
                // The developer has changed the Cancel property to true, indicating that they wish to Cancel the
                // closing of this tip, so we need to revert the IsOpen property to true.
                strongThis->IsOpen(true);
            }
        }
    };

    args->SetDeferral(instance);

    args->IncrementDeferralCount();
    m_closingEventSource(*this, *args);
    args->DecrementDeferralCount();
}

void TeachingTip::ClosePopupWithAnimationIfAvailable()
{
    if (m_popup && m_popup.get().IsOpen())
    {
        if (SharedHelpers::IsRS5OrHigher())
        {
            StartContractToClose();
        }
        else
        {
            ClosePopup();
        }

        // Under normal circumstances we would have launched an animation just now, if we did not then we should make sure
        // that the idle state is correct.
        if (!m_isContractAnimationPlaying && !m_isIdle && !m_isExpandAnimationPlaying)
        {
            m_isIdle = true;
            TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
        }
    }
}

void TeachingTip::ClosePopup()
{
    if (auto&& popup = m_popup.get())
    {
        popup.IsOpen(false);
    }
    if (auto&& lightDismissIndicatorPopup = m_lightDismissIndicatorPopup.get())
    {
        lightDismissIndicatorPopup.IsOpen(false);
    }
    if (auto && beakOcclusionGrid = m_beakOcclusionGrid.get())
    {
        if (SharedHelpers::IsRS5OrHigher())
        {
            // A previous close animation may have left the rootGrid's scale at a very small value and if this teaching tip
            // is shown again then its text would be rasterized at this small scale and blown up ~20x. To fix this we have to
            // reset the scale after the popup has closed so that if the teaching tip is reshown the render pass does not use the
            // small scale.
            beakOcclusionGrid.Scale({ 1.0f,1.0f,1.0f });
        }
    }
}

void TeachingTip::SetTarget(const winrt::FrameworkElement& element)
{
    m_targetLayoutUpdatedRevoker.revoke();
    m_targetEffectiveViewportChangedRevoker.revoke();

    m_target.set(element);

    if (IsOpen())
    {
        if (element)
        {
            m_currentTargetBounds = element.TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(element.as<winrt::FrameworkElement>().ActualWidth()),
                static_cast<float>(element.as<winrt::FrameworkElement>().ActualHeight())
            });
        }
        SetViewportChangedEvent();
        PositionPopup();
    }
}

void TeachingTip::SetViewportChangedEvent()
{
    if (m_tipFollowsTarget)
    {
        if (auto targetAsFE = m_target.get())
        {
            // EffectiveViewPortChanged is only available on RS5 and higher.
            if (SharedHelpers::IsRS5OrHigher())
            {
                m_targetEffectiveViewportChangedRevoker = targetAsFE.EffectiveViewportChanged(winrt::auto_revoke, { this, &TeachingTip::TargetLayoutUpdated });
                m_effectiveViewportChangedRevoker = this->EffectiveViewportChanged(winrt::auto_revoke, { this, &TeachingTip::TargetLayoutUpdated });
            }
            else
            {
                m_targetLayoutUpdatedRevoker = targetAsFE.LayoutUpdated(winrt::auto_revoke, { this, &TeachingTip::TargetLayoutUpdated });
            }
        }
    }
}

void TeachingTip::RevokeViewportChangedEvent()
{
    m_targetEffectiveViewportChangedRevoker.revoke();
    m_effectiveViewportChangedRevoker.revoke();
    m_targetLayoutUpdatedRevoker.revoke();
}

void TeachingTip::TargetLayoutUpdated(const winrt::IInspectable&, const winrt::IInspectable&)
{
    if (IsOpen())
    {
        if (auto&& target = m_target.get())
        {
            auto newTargetBounds = target.TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualWidth()),
                static_cast<float>(target.as<winrt::FrameworkElement>().ActualHeight())
            });

            auto newCurrentBounds = this->TransformToVisual(nullptr).TransformBounds({
                0.0,
                0.0,
                static_cast<float>(this->ActualWidth()),
                static_cast<float>(this->ActualHeight())
            });

            if (newTargetBounds != m_currentTargetBounds || newCurrentBounds != m_currentBounds)
            {
                m_currentBounds = newCurrentBounds;
                m_currentTargetBounds = newTargetBounds;
                PositionPopup();
            }
        }
    }
}

void TeachingTip::CreateExpandAnimation()
{
    auto compositor = winrt::Window::Current().Compositor();
    if (!m_expandEasingFunction)
    {
        m_expandEasingFunction.set(compositor.CreateCubicBezierEasingFunction(s_expandAnimationEasingCurveControlPoint1, s_expandAnimationEasingCurveControlPoint2));
    }
    auto expandAnimation = compositor.CreateVector3KeyFrameAnimation();
    if (auto&& beakOcclusionGrid = m_beakOcclusionGrid.get())
    {
        expandAnimation.SetScalarParameter(L"Width", static_cast<float>(beakOcclusionGrid.ActualWidth()));
        expandAnimation.SetScalarParameter(L"Height", static_cast<float>(beakOcclusionGrid.ActualHeight()));
    }
    else
    {
        expandAnimation.SetScalarParameter(L"Width", s_defaultTipHeightAndWidth);
        expandAnimation.SetScalarParameter(L"Height", s_defaultTipHeightAndWidth);
    }

    auto&& expandEasingFunction = m_expandEasingFunction.get();
    expandAnimation.InsertExpressionKeyFrame(0.0f, L"Vector3(Min(0.01, 20.0 / Width), Min(0.01, 20.0 / Height), 1.0)");
    expandAnimation.InsertKeyFrame(1.0f, { 1.0f, 1.0f, 1.0f }, expandEasingFunction);
    expandAnimation.Duration(m_expandAnimationDuration);
    expandAnimation.Target(s_scaleTargetName);
    m_expandAnimation.set(expandAnimation);

    auto expandElevationAnimation = compositor.CreateVector3KeyFrameAnimation();
    expandElevationAnimation.InsertExpressionKeyFrame(1.0f, L"Vector3(this.Target.Translation.X, this.Target.Translation.Y, contentElevation)", expandEasingFunction);
    expandElevationAnimation.SetScalarParameter(L"contentElevation", m_contentElevation);
    expandElevationAnimation.Duration(m_expandAnimationDuration);
    expandElevationAnimation.Target(s_translationTargetName);
    m_expandElevationAnimation.set(expandElevationAnimation);
}

void TeachingTip::CreateContractAnimation()
{
    auto compositor = winrt::Window::Current().Compositor();
    if (!m_contractEasingFunction)
    {
        m_contractEasingFunction.set(compositor.CreateCubicBezierEasingFunction(s_contractAnimationEasingCurveControlPoint1, s_contractAnimationEasingCurveControlPoint2));
    }

    auto contractAnimation = compositor.CreateVector3KeyFrameAnimation();
    if (auto&& beakOcclusionGrid = m_beakOcclusionGrid.get())
    {
        contractAnimation.SetScalarParameter(L"Width", static_cast<float>(beakOcclusionGrid.ActualWidth()));
        contractAnimation.SetScalarParameter(L"Height", static_cast<float>(beakOcclusionGrid.ActualHeight()));
    }
    else
    {
        contractAnimation.SetScalarParameter(L"Width", s_defaultTipHeightAndWidth);
        contractAnimation.SetScalarParameter(L"Height", s_defaultTipHeightAndWidth);
    }

    auto&& contractEasingFunction = m_contractEasingFunction.get();
    contractAnimation.InsertKeyFrame(0.0f, { 1.0f, 1.0f, 1.0f });
    contractAnimation.InsertExpressionKeyFrame(1.0f, L"Vector3(20.0 / Width, 20.0 / Height, 1.0)", contractEasingFunction);
    contractAnimation.Duration(m_contractAnimationDuration);
    contractAnimation.Target(s_scaleTargetName);
    m_contractAnimation.set(contractAnimation);

    auto contractElevationAnimation = compositor.CreateVector3KeyFrameAnimation();
    contractElevationAnimation.InsertExpressionKeyFrame(1.0f, L"Vector3(this.Target.Translation.X, this.Target.Translation.Y, 0.0f)", contractEasingFunction);
    contractElevationAnimation.Duration(m_contractAnimationDuration);
    contractElevationAnimation.Target(s_translationTargetName);
    m_contractElevationAnimation.set(contractElevationAnimation);
}

void TeachingTip::StartExpandToOpen()
{
    // The contract and expand animations currently use facade's which were not availible pre RS5.
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (!m_expandAnimation)
        {
            CreateExpandAnimation();
        }
        auto scopedBatch = winrt::Window::Current().Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        auto&& expandAnimation = m_expandAnimation.get();
        if (auto&& beakOcclusionGrid = m_beakOcclusionGrid.get())
        {
            beakOcclusionGrid.StartAnimation(expandAnimation);
            m_isExpandAnimationPlaying = true;
        }
        if (auto&& contentRootGrid = m_contentRootGrid.get())
        {
            contentRootGrid.StartAnimation(m_expandElevationAnimation.get());
            m_isExpandAnimationPlaying = true;
        }
        if (auto&& beakEdgeBorder = m_beakEdgeBorder.get())
        {
            beakEdgeBorder.StartAnimation(expandAnimation);
            m_isExpandAnimationPlaying = true;
        }
        scopedBatch.End();

        auto strongThis = get_strong();
        scopedBatch.Completed([strongThis](auto, auto)
        {
            strongThis->m_isExpandAnimationPlaying = false;
            if (!strongThis->m_isContractAnimationPlaying && !strongThis->m_isIdle)
            {
                strongThis->m_isIdle = true;
                TeachingTipTestHooks::NotifyIdleStatusChanged(*strongThis);
            }
        });
    }

    // Under normal circumstances we would have launched an animation just now, if we did not then we should make sure that the idle state is correct
    if (!m_isExpandAnimationPlaying && !m_isIdle && !m_isContractAnimationPlaying)
    {
        m_isIdle = true;
        TeachingTipTestHooks::NotifyIdleStatusChanged(*this);
    }
}

void TeachingTip::StartContractToClose()
{
    // The contract and expand animations currently use facade's which were not availible pre RS5.
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (!m_contractAnimation)
        {
            CreateContractAnimation();
        }

        auto scopedBatch = winrt::Window::Current().Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        auto&& contractAnimation = m_contractAnimation.get();
        if (auto&& beakOcclusionGrid = m_beakOcclusionGrid.get())
        {
            beakOcclusionGrid.StartAnimation(contractAnimation);
            m_isContractAnimationPlaying = true;
        }
        if (auto&& contentRootGrid = m_contentRootGrid.get())
        {
            contentRootGrid.StartAnimation(m_contractElevationAnimation.get());
            m_isContractAnimationPlaying = true;
        }
        if (auto&& beakEdgeBorder = m_beakEdgeBorder.get())
        {
            beakEdgeBorder.StartAnimation(contractAnimation);
            m_isContractAnimationPlaying = true;
        }
        scopedBatch.End();

        auto strongThis = get_strong();
        scopedBatch.Completed([strongThis](auto, auto)
        {
            strongThis->m_isContractAnimationPlaying = false;
            strongThis->ClosePopup();
            if (!strongThis->m_isExpandAnimationPlaying && !strongThis->m_isIdle)
            {
                strongThis->m_isIdle = true;
                TeachingTipTestHooks::NotifyIdleStatusChanged(*strongThis);
            }
        });
    }
}

winrt::TeachingTipPlacementMode TeachingTip::DetermineEffectivePlacement()
{
    auto placement = Placement();
    if (placement != winrt::TeachingTipPlacementMode::Auto)
    {
        return placement;
    }
    else
    {
        if (IsOpen() && m_currentEffectivePlacementMode != winrt::TeachingTipPlacementMode::Auto)
        {
            return m_currentEffectivePlacementMode;
        }
        if (m_target)
        {
            bool topCenterAvailable = true;
            bool topLeftAvailable = true;
            bool topRightAvailable = true;
            bool bottomCenterAvailable = true;
            bool bottomLeftAvailable = true;
            bool bottomRightAvailable = true;
            bool rightCenterAvailable = true;
            bool rightTopAvailable = true;
            bool rightBottomAvailable = true;
            bool leftCenterAvailable = true;
            bool leftTopAvailable = true;
            bool leftBottomAvailable = true;

            auto windowBounds = m_useTestWindowBounds ? m_testWindowBounds : winrt::Window::Current().CoreWindow().Bounds();
            auto targetBounds = m_currentTargetBounds;
            if (m_useTestWindowBounds)
            {
                targetBounds.X -= windowBounds.X;
                targetBounds.Y -= windowBounds.Y;
            }

            auto&& beakOcclusionGrid = m_beakOcclusionGrid.get();
            double contentHeight = beakOcclusionGrid.ActualHeight();
            double contentWidth = beakOcclusionGrid.ActualWidth();
            double tipHeight = contentHeight + BeakShortSideLength();
            double tipWidth = contentWidth + BeakShortSideLength();

            if (BleedingImageContent())
            {
                if (m_bleedingImageContentBorder.get().ActualHeight() > m_nonBleedingContentRootGrid.get().ActualHeight() - BeakLongSideActualLength())
                {
                    leftCenterAvailable = false;
                    rightCenterAvailable = false;
                }

                switch(BleedingImagePlacement())
                {
                case winrt::TeachingTipBleedingImagePlacementMode::Bottom:
                    topCenterAvailable = false;
                    topRightAvailable = false;
                    topLeftAvailable = false;
                    rightTopAvailable = false;
                    leftTopAvailable = false;
                    break;
                case winrt::TeachingTipBleedingImagePlacementMode::Top:
                    bottomCenterAvailable = false;
                    bottomLeftAvailable = false;
                    bottomRightAvailable = false;
                    rightBottomAvailable = false;
                    leftBottomAvailable = false;
                    break;
                }
            }

            // If the left edge of the target is past the right edge of the window.
            if (targetBounds.X > windowBounds.Width)
            {
                leftBottomAvailable = false;
                leftCenterAvailable = false;
                leftTopAvailable = false;
            }
            // If the right edge of the target is before the left edge of the window.
            if (targetBounds.X + targetBounds.Width < 0)
            {
                rightBottomAvailable = false;
                rightCenterAvailable = false;
                rightTopAvailable = false;
            }
            // If the top edge of the target is below the bottom edge of the window.
            if (targetBounds.Y > windowBounds.Height)
            {
                topLeftAvailable = false;
                topCenterAvailable = false;
                topRightAvailable = false;
            }
            // If the bottom edge of the target is above the edge of the window.
            if (targetBounds.Y + targetBounds.Height < 0)
            {
                bottomLeftAvailable = false;
                bottomCenterAvailable = false;
                bottomRightAvailable = false;
            }

            // If the horizontal midpoint is out of the window.
            if (targetBounds.X + (targetBounds.Width / 2) < MinimumTipEdgeToBeakCenter() ||
                targetBounds.X + (targetBounds.Width / 2) > windowBounds.Width - MinimumTipEdgeToBeakCenter())
            {
                topLeftAvailable = false;
                topCenterAvailable = false;
                topRightAvailable = false;
                bottomLeftAvailable = false;
                bottomCenterAvailable = false;
                bottomRightAvailable = false;
            }

            // If the vertical midpoint is out of the window.
            if (targetBounds.Y + (targetBounds.Height / 2) < MinimumTipEdgeToBeakCenter() ||
                targetBounds.Y + (targetBounds.Height / 2) > windowBounds.Height - MinimumTipEdgeToBeakCenter())
            {
                leftBottomAvailable = false;
                leftCenterAvailable = false;
                leftTopAvailable = false;
                rightBottomAvailable = false;
                rightCenterAvailable = false;
                rightTopAvailable = false;
            }

            // If the tip is too tall to fit between the top of the target and the top edge of the window.
            if (tipHeight > targetBounds.Y)
            {
                topCenterAvailable = false;
                topRightAvailable = false;
                topLeftAvailable = false;
            }
            // If the tip is too tall to fit between the center of the target and the top edge of the window.
            if (contentHeight - MinimumTipEdgeToBeakCenter() > targetBounds.Y + (targetBounds.Height / 2))
            {
                rightTopAvailable = false;
                leftTopAvailable = false;
            }
            // If the tip is too tall to fit in the window when the beak is centered vertically on the target and the tip.
            if (contentHeight / 2 > targetBounds.Y + targetBounds.Height / 2 ||
                contentHeight / 2 > (windowBounds.Height - (targetBounds.Height + targetBounds.Y) + (targetBounds.Height / 2)))
            {
                rightCenterAvailable = false;
                leftCenterAvailable = false;
            }
            // If the tip is too tall to fit between the center of the target and the bottom edge of the window.
            if (contentHeight - MinimumTipEdgeToBeakCenter() > windowBounds.Height - (targetBounds.Y + (targetBounds.Height / 2)))
            {
                rightBottomAvailable = false;
                leftBottomAvailable = false;
            }
            // If the tip is too tall to fit between the bottom of the target and the bottom edge of the window.
            if (tipHeight > windowBounds.Height - (targetBounds.Height + targetBounds.Y))
            {
                bottomCenterAvailable = false;
                bottomLeftAvailable = false;
                bottomRightAvailable = false;
            }

            // If the tip is too wide to fit between the left edge of the target and the left edge of the window.
            if (tipWidth > targetBounds.X)
            {
                leftCenterAvailable = false;
                leftTopAvailable = false;
                leftBottomAvailable = false;
            }
            // If the tip is too wide to fit between the center of the target and the left edge of the window.
            if (contentWidth - MinimumTipEdgeToBeakCenter() > targetBounds.X + (targetBounds.Width / 2))
            {
                topLeftAvailable = false;
                bottomLeftAvailable = false;
            }
            // If the tip is too wide to fit in the window when the beak is centerd horizontally on the target and the tip.
            if (contentWidth / 2 > targetBounds.X + targetBounds.Width / 2 ||
                contentWidth / 2 > (windowBounds.Width - (targetBounds.Width + targetBounds.X) + (targetBounds.Width / 2)))
            {
                topCenterAvailable = false;
                bottomCenterAvailable = false;
            }
            // If the tip is too wide to fit between the center of the target and the right edge of the window.
            if (contentWidth - MinimumTipEdgeToBeakCenter() > windowBounds.Width - (targetBounds.X + (targetBounds.Width / 2)))
            {
                topRightAvailable = false;
                bottomRightAvailable = false;
            }
            // If the tip is too wide to fit between the right edge of the target and the right edge of the window.
            if (tipWidth > windowBounds.Width - (targetBounds.Width + targetBounds.X))
            {
                rightCenterAvailable = false;
                rightTopAvailable = false;
                rightBottomAvailable = false;
            }


            if (topCenterAvailable)
            {
                return winrt::TeachingTipPlacementMode::Top;
            }
            else if (bottomCenterAvailable)
            {
                return winrt::TeachingTipPlacementMode::Bottom;
            }
            else if (rightCenterAvailable)
            {
                return winrt::TeachingTipPlacementMode::Right;
            }
            else if (leftCenterAvailable)
            {
                return winrt::TeachingTipPlacementMode::Left;
            }
            else if (topRightAvailable)
            {
                return winrt::TeachingTipPlacementMode::TopEdgeAlignedRight;
            }
            else if (topLeftAvailable)
            {
                return winrt::TeachingTipPlacementMode::TopEdgeAlignedLeft;
            }
            else if (bottomRightAvailable)
            {
                return winrt::TeachingTipPlacementMode::BottomEdgeAlignedRight;
            }
            else if (bottomLeftAvailable)
            {
                return winrt::TeachingTipPlacementMode::BottomEdgeAlignedLeft;
            }
            else if (rightTopAvailable)
            {
                return winrt::TeachingTipPlacementMode::RightEdgeAlignedTop;
            }
            else if (rightBottomAvailable)
            {
                return winrt::TeachingTipPlacementMode::RightEdgeAlignedBottom;
            }
            else if (leftTopAvailable)
            {
                return winrt::TeachingTipPlacementMode::LeftEdgeAlignedTop;
            }
            else if (leftBottomAvailable)
            {
                return winrt::TeachingTipPlacementMode::LeftEdgeAlignedBottom;
            }
        }
    }
    // The teaching tip wont fit anywhere, just return top.
    return winrt::TeachingTipPlacementMode::Top;
}


void TeachingTip::EstablishShadows()
{
#ifdef USE_INSIDER_SDK
#ifdef BEAK_SHADOW
#ifdef _DEBUG
    if (winrt::IUIElement10 beakPolygon_uiElement10 = m_contentRootGrid.get())
    {
        if (m_tipShadow)
        {
            if (!beakPolygon_uiElement10.Shadow())
            {
                // This facilitates an experiment around faking a proper beak shadow, shadows are expensive though so we don't want it present for release builds.
                auto beakShadow = winrt::Windows::UI::Xaml::Media::ThemeShadow{};
                beakShadow.Receivers().Append(m_target.get());
                beakPolygon_uiElement10.Shadow(beakShadow);
                auto&& beakPolygon = m_beakPolygon.get();
                auto&& beakPolygonTranslation = beakPolygon.Translation();
                beakPolygon.Translation({ beakPolygonTranslation.x, beakPolygonTranslation.y, m_beakElevation });
            }
        }
        else
        {
            beakPolygon_uiElement10.Shadow(nullptr);
        }
    }
#endif
#endif
    if (winrt::IUIElement10 m_contentRootGrid_uiElement10 = m_contentRootGrid.get())
    {
        if (m_tipShouldHaveShadow)
        {
            if (!m_contentRootGrid_uiElement10.Shadow())
            {
                m_contentRootGrid_uiElement10.Shadow(winrt::ThemeShadow{});
                auto contentRootGrid = m_contentRootGrid.get();
                auto contentRootGridTranslation = contentRootGrid.Translation();
                contentRootGrid.Translation({ contentRootGridTranslation.x, contentRootGridTranslation.y, m_contentElevation });
            }
        }
        else
        {
            m_contentRootGrid_uiElement10.Shadow(nullptr);
        }
    }
#endif
}

void TeachingTip::OnPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.Property() == s_AttachProperty)
    {
        OnAttachPropertyChanged(sender, args);
    }
    else
    {
        winrt::get_self<TeachingTip>(sender.as<winrt::TeachingTip>())->OnPropertyChanged(args);
    }
}

void TeachingTip::OnAttachPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto oldTip = unbox_value<winrt::TeachingTip>(args.OldValue());
    auto newTip = unbox_value<winrt::TeachingTip>(args.NewValue());

    if (oldTip == newTip)
    {
        return;
    }

    winrt::TeachingTip::SetAttach(nullptr, oldTip);
    winrt::TeachingTip::SetAttach(sender.try_as<winrt::UIElement>(), newTip);
}

void TeachingTip::SetAttach(const winrt::UIElement& element, const winrt::TeachingTip& teachingTip)
{
    MUX_ASSERT(teachingTip);
    auto tip = winrt::get_self<TeachingTip>(teachingTip);
    tip->SetTarget(element.as<winrt::FrameworkElement>());
}

winrt::TeachingTip TeachingTip::GetAttach(const winrt::UIElement& element)
{
    return unbox_value<winrt::TeachingTip>(element.GetValue(s_AttachProperty));
}

////////////////
// Test Hooks //
////////////////
void TeachingTip::SetExpandEasingFunction(const winrt::CompositionEasingFunction& easingFunction)
{
    m_expandEasingFunction.set(easingFunction);
    CreateExpandAnimation();
}

void TeachingTip::SetContractEasingFunction(const winrt::CompositionEasingFunction& easingFunction)
{
    m_contractEasingFunction.set(easingFunction);
    CreateContractAnimation();
}

void TeachingTip::SetTipShouldHaveShadow(bool tipShouldHaveShadow)
{
    if (m_tipShouldHaveShadow != tipShouldHaveShadow)
    {
        m_tipShouldHaveShadow = tipShouldHaveShadow;
        EstablishShadows();
    }
}

void TeachingTip::SetContentElevation(float elevation)
{
    m_contentElevation = elevation;
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (auto&& beakOcclusionGrid = m_beakOcclusionGrid.get())
        {
            auto beakOcclusionGridTranslation = beakOcclusionGrid.Translation();
            m_contentRootGrid.get().Translation({ beakOcclusionGridTranslation.x, beakOcclusionGridTranslation.y, m_contentElevation });
        }
        if (m_expandElevationAnimation)
        {
            m_expandElevationAnimation.get().SetScalarParameter(L"contentElevation", m_contentElevation);
        }
    }
}

void TeachingTip::SetBeakElevation(float elevation)
{
    m_beakElevation = elevation;
    if (SharedHelpers::IsRS5OrHigher() && m_beakPolygon)
    {
        if (auto && beakPolygon = m_beakPolygon.get())
        {
            auto beakPolygonTranslation = beakPolygon.Translation();
            beakPolygon.Translation({ beakPolygonTranslation.x, beakPolygonTranslation.y, m_beakElevation });
        }
    }
}

void TeachingTip::SetUseTestWindowBounds(bool useTestWindowBounds)
{
    m_useTestWindowBounds = useTestWindowBounds;
}

void TeachingTip::SetTestWindowBounds(const winrt::Rect& testWindowBounds)
{
    m_testWindowBounds = testWindowBounds;
}

void TeachingTip::SetTipFollowsTarget(bool tipFollowsTarget)
{
    if (m_tipFollowsTarget != tipFollowsTarget)
    {
        m_tipFollowsTarget = tipFollowsTarget;
        if (tipFollowsTarget)
        {
            SetViewportChangedEvent();
        }
        else
        {
            RevokeViewportChangedEvent();
        }
    }
}

void TeachingTip::SetExpandAnimationDuration(const winrt::TimeSpan& expandAnimationDuration)
{
    m_expandAnimationDuration = expandAnimationDuration;
    if (auto&& expandAnimation = m_expandAnimation.get())
    {
        expandAnimation.Duration(m_expandAnimationDuration);
    }
    if (auto&& expandElevationAnimation = m_expandElevationAnimation.get())
    {
        expandElevationAnimation.Duration(m_expandAnimationDuration);
    }
}

void TeachingTip::SetContractAnimationDuration(const winrt::TimeSpan& contractAnimationDuration)
{
    m_contractAnimationDuration = contractAnimationDuration;
    if (auto&& contractAnimation = m_contractAnimation.get())
    {
        contractAnimation.Duration(m_contractAnimationDuration);
    }
    if (auto&& contractElevationAnimation = m_contractElevationAnimation.get())
    {
        contractElevationAnimation.Duration(m_contractAnimationDuration);
    }
}

bool TeachingTip::GetIsIdle()
{
    return m_isIdle;
}

winrt::TeachingTipPlacementMode TeachingTip::GetEffectivePlacement()
{
    return m_currentEffectivePlacementMode;
}

winrt::TeachingTipBleedingImagePlacementMode TeachingTip::GetEffectiveBleedingPlacement()
{
    return m_currentBleedingEffectivePlacementMode;
}

double TeachingTip::GetHorizontalOffset()
{
    if (auto&& popup = m_popup.get())
    {
        return popup.HorizontalOffset();
    }
    return 0.0;
}

double TeachingTip::GetVerticalOffset()
{
    if (auto&& popup = m_popup.get())
    {
        return popup.VerticalOffset();
    }
    return 0.0;
}
