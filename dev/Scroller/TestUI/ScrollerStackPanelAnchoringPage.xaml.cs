﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ScrollerAnchorRequestedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerAnchorRequestedEventArgs;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;

using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerViewChangeResult = Microsoft.UI.Private.Controls.ScrollerViewChangeResult;
using ScrollerTestHooksAnchorEvaluatedEventArgs = Microsoft.UI.Private.Controls.ScrollerTestHooksAnchorEvaluatedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerStackPanelAnchoringPage : TestPage
    {
        private DispatcherTimer timer = new DispatcherTimer();
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private List<QueuedOperation> lstQueuedOperations = new List<QueuedOperation>();
        private List<QueuedOperation> lstTriggeredOperations = new List<QueuedOperation>();
        private Border currentAnchor = null;
        private UIElement anchorElement = null;
        private SolidColorBrush chartreuseBrush = new SolidColorBrush(Colors.Chartreuse);
        private SolidColorBrush blanchedAlmondBrush = new SolidColorBrush(Colors.BlanchedAlmond);
        private SolidColorBrush orangeBrush = new SolidColorBrush(Colors.Orange);
        private int operationCount = 0;
        private double lastScrollerOffset = 0.0;

        public ScrollerStackPanelAnchoringPage()
        {
            InitializeComponent();

            Loaded += ScrollerStackPanelAnchoringPage_Loaded;

            scroller.SizeChanged += Scroller_SizeChanged;

            timer.Interval = new TimeSpan(0, 0, 2 /*sec*/);
            timer.Tick += Timer_Tick;

            Insert(0 /*newIndex*/, 16 /*newCount*/);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            UpdateRaiseAnchorNotifications(raiseAnchorNotifications: false);

            base.OnNavigatedFrom(e);
        }

        private void ScrollerStackPanelAnchoringPage_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                if (chkLogScrollerMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                UpdateRaiseAnchorNotifications(raiseAnchorNotifications: true);

                UpdateHorizontalAnchorRatio();
                UpdateVerticalAnchorRatio();

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    scroller.ExtentChanged += Scroller_ExtentChanged;
                    scroller.StateChanged += Scroller_StateChanged;
                }
                scroller.AnchorRequested += Scroller_AnchorRequested;
                scroller.ViewChanged += Scroller_ViewChanged;
                scroller.ScrollCompleted += Scroller_ScrollCompleted;
                scroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            cnsAnchorPoint.Width = scroller.Width;
            cnsAnchorPoint.Height = scroller.Height;
        }

        private void Timer_Tick(object sender, object e)
        {
            timer.Stop();
            ExecuteQueuedOperations();
        }

        private void BtnInsert_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Insert(int.Parse(txtNewStartIndex.Text), int.Parse(txtNewCount.Text));
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Insert, 0, 0, int.Parse(txtNewStartIndex.Text), int.Parse(txtNewCount.Text)));
                        AppendAsyncEventMessage("Queued Insert NewIndex=" + txtNewStartIndex.Text + ", NewCount=" + txtNewCount.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Insert, 0, 0, int.Parse(txtNewStartIndex.Text), int.Parse(txtNewCount.Text)));
                        AppendAsyncEventMessage("Triggerable Insert NewIndex=" + txtNewStartIndex.Text + ", NewCount=" + txtNewCount.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnRemove_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Remove(int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text));
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Remove, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, 0));
                        AppendAsyncEventMessage("Queued Remove OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Remove, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, 0));
                        AppendAsyncEventMessage("Triggerable Remove OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnReplace_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Replace(int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), int.Parse(txtNewCount.Text));
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Replace, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, int.Parse(txtNewCount.Text)));
                        AppendAsyncEventMessage("Queued Replace OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text + ", NewCount=" + txtNewCount.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Replace, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, int.Parse(txtNewCount.Text)));
                        AppendAsyncEventMessage("Triggerable Replace OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text + ", NewCount=" + txtNewCount.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnShrink_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Shrink(int.Parse(txtIndex.Text), 1);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Shrink, 0, 0, int.Parse(txtIndex.Text), 1));
                        AppendAsyncEventMessage("Queued Shrink Index=" + txtIndex.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Shrink, 0, 0, int.Parse(txtIndex.Text), 1));
                        AppendAsyncEventMessage("Triggerable Shrink Index=" + txtIndex.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnShrinkMore_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Shrink(int.Parse(txtIndex.Text), 4);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Shrink, 0, 0, int.Parse(txtIndex.Text), 4));
                        AppendAsyncEventMessage("Queued Shrink More Index=" + txtIndex.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Shrink, 0, 0, int.Parse(txtIndex.Text), 4));
                        AppendAsyncEventMessage("Triggerable Shrink More Index=" + txtIndex.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnExpand_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Expand(int.Parse(txtIndex.Text), 1);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Expand, 0, 0, int.Parse(txtIndex.Text), 1));
                        AppendAsyncEventMessage("Queued Expand Index=" + txtIndex.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Expand, 0, 0, int.Parse(txtIndex.Text), 1));
                        AppendAsyncEventMessage("Triggerable Expand Index=" + txtIndex.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnExpandMore_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        Expand(int.Parse(txtIndex.Text), 4);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Expand, 0, 0, int.Parse(txtIndex.Text), 4));
                        AppendAsyncEventMessage("Queued Expand Index=" + txtIndex.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Expand, 0, 0, int.Parse(txtIndex.Text), 4));
                        AppendAsyncEventMessage("Triggerable Expand Index=" + txtIndex.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void ExecuteQueuedOperations()
        {
            try
            {
                while (lstQueuedOperations.Count > 0)
                {
                    QueuedOperation qo = lstQueuedOperations[0];

                    switch (qo.Type)
                    {
                        case QueuedOperationType.Insert:
                            AppendAsyncEventMessage("Unqueuing Insert NewIndex=" + qo.NewIndex + ", NewCount=" + qo.NewCount);
                            Insert(qo.NewIndex, qo.NewCount);
                            break;
                        case QueuedOperationType.Remove:
                            AppendAsyncEventMessage("Unqueuing Remove OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount);
                            Remove(qo.OldIndex, qo.OldCount);
                            break;
                        case QueuedOperationType.Replace:
                            AppendAsyncEventMessage("Unqueuing Replace OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount + ", NewCount=" + qo.NewCount);
                            Replace(qo.OldIndex, qo.OldCount, qo.NewCount);
                            break;
                        case QueuedOperationType.Shrink:
                            AppendAsyncEventMessage("Unqueuing Shrink Index=" + qo.NewIndex + ", Amount=" + qo.NewCount * 20);
                            Shrink(qo.NewIndex, qo.NewCount);
                            break;
                        case QueuedOperationType.Expand:
                            AppendAsyncEventMessage("Unqueuing Expand Index=" + qo.NewIndex + ", Amount=" + qo.NewCount * 20);
                            Expand(qo.NewIndex, qo.NewCount);
                            break;
                    }

                    lstQueuedOperations.RemoveAt(0);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void ExecuteTriggerableOperations()
        {
            try
            {
                while (lstTriggeredOperations.Count > 0)
                {
                    QueuedOperation qo = lstTriggeredOperations[0];

                    switch (qo.Type)
                    {
                        case QueuedOperationType.Insert:
                            AppendAsyncEventMessage("Triggering Insert NewIndex=" + qo.NewIndex + ", NewCount=" + qo.NewCount);
                            Insert(qo.NewIndex, qo.NewCount);
                            break;
                        case QueuedOperationType.Remove:
                            AppendAsyncEventMessage("Triggering Remove OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount);
                            Remove(qo.OldIndex, qo.OldCount);
                            break;
                        case QueuedOperationType.Replace:
                            AppendAsyncEventMessage("Triggering Replace OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount + ", NewCount=" + qo.NewCount);
                            Replace(qo.OldIndex, qo.OldCount, qo.NewCount);
                            break;
                        case QueuedOperationType.Shrink:
                            AppendAsyncEventMessage("Triggering Shrink Index=" + qo.NewIndex + ", Amount=" + qo.NewCount * 20);
                            Shrink(qo.NewIndex, qo.NewCount);
                            break;
                        case QueuedOperationType.Expand:
                            AppendAsyncEventMessage("Triggering Expand Index=" + qo.NewIndex + ", Amount=" + qo.NewCount * 20);
                            Expand(qo.NewIndex, qo.NewCount);
                            break;
                    }

                    lstTriggeredOperations.RemoveAt(0);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Insert(int newIndex, int newCount)
        {
            if (newIndex < 0 || newIndex > stackPanel.Children.Count || newCount <= 0)
            {
                throw new ArgumentException();
            }

            for (int i = 0; i < newCount; i++)
            {
                TextBlock textBlock = new TextBlock();
                textBlock.Text = "TB#" + stackPanel.Children.Count + "_" + operationCount;
                textBlock.Name = "textBlock" + stackPanel.Children.Count + "_" + operationCount;
                textBlock.HorizontalAlignment = HorizontalAlignment.Center;
                textBlock.VerticalAlignment = VerticalAlignment.Center;

                Border border = new Border();
                border.Name = "border" + stackPanel.Children.Count + "_" + operationCount;
                border.BorderThickness = border.Margin = new Thickness(3);
                border.BorderBrush = chartreuseBrush;
                border.Background = blanchedAlmondBrush;
                if (chkHorizontalOrientation.IsChecked == true)
                {
                    border.Width = 120;
                    border.Height = 170;
                }
                else
                {
                    border.Width = 170;
                    border.Height = 120;
                }
                border.Child = textBlock;

                stackPanel.Children.Insert(newIndex + i, border);
            }

            operationCount++;
        }

        private void Remove(int oldIndex, int oldCount)
        {
            if (oldIndex < 0 || oldIndex > stackPanel.Children.Count - oldCount || oldCount <= 0)
            {
                throw new ArgumentException();
            }

            bool isAnchorRemoved = false;

            for (int i = 0; i < oldCount; i++)
            {
                if (!isAnchorRemoved && anchorElement == stackPanel.Children[oldIndex])
                {
                    isAnchorRemoved = true;
                }
                stackPanel.Children.RemoveAt(oldIndex);
            }

            if (isAnchorRemoved)
            {
                BtnSetAnchorElement_Click(null, null);
            }

            operationCount++;
        }

        private void Replace(int oldIndex, int oldCount, int newCount)
        {
            if (oldIndex < 0 || oldIndex > stackPanel.Children.Count - oldCount || oldCount <= 0 || newCount <= 0)
            {
                throw new ArgumentException();
            }

            for (int i = 0; i < oldCount; i++)
            {
                stackPanel.Children.RemoveAt(oldIndex);
            }

            for (int i = 0; i < newCount; i++)
            {
                TextBlock textBlock = new TextBlock();
                textBlock.Text = "TB#" + stackPanel.Children.Count + "_" + operationCount;
                textBlock.Name = "textBlock" + stackPanel.Children.Count + "_" + operationCount;
                textBlock.HorizontalAlignment = HorizontalAlignment.Center;
                textBlock.VerticalAlignment = VerticalAlignment.Center;

                Border border = new Border();
                border.Name = "border" + stackPanel.Children.Count + "_" + operationCount;
                border.BorderThickness = border.Margin = new Thickness(3);
                border.BorderBrush = chartreuseBrush;
                border.Background = blanchedAlmondBrush;
                if (chkHorizontalOrientation.IsChecked == true)
                {
                    border.Width = 120;
                    border.Height = 170;
                }
                else
                {
                    border.Width = 170;
                    border.Height = 120;
                }
                border.Child = textBlock;

                stackPanel.Children.Insert(oldIndex + i, border);
            }

            operationCount++;
        }

        private void Shrink(int index, int amount)
        {
            if (index < 0 || index >= stackPanel.Children.Count)
            {
                throw new ArgumentException();
            }

            Border border = stackPanel.Children[index] as Border;
            border.Height = Math.Max(20, border.Height - 20 * amount);

            operationCount++;
        }

        private void Expand(int index, int amount)
        {
            if (index < 0 || index >= stackPanel.Children.Count)
            {
                throw new ArgumentException();
            }

            Border border = stackPanel.Children[index] as Border;
            border.Height += 20 * amount;

            operationCount++;
        }

        private void Scroller_ExtentChanged(Scroller sender, object args)
        {
            if (chkLogScrollerEvents.IsChecked == true)
            {
                AppendAsyncEventMessage("ExtentChanged ExtentWidth=" + sender.ExtentWidth.ToString() + ", ExtentHeight=" + sender.ExtentHeight.ToString());
            }
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            if (chkLogScrollerEvents.IsChecked == true)
            {
                AppendAsyncEventMessage("StateChanged " + sender.State.ToString());
            }
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            if (chkLogScrollerEvents.IsChecked == true)
            {
                AppendAsyncEventMessage("ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset.ToString() + ", S=" + sender.ZoomFactor.ToString());
            }

            double newScrollerOffset = stackPanel.Orientation == Orientation.Horizontal ? scroller.HorizontalOffset : scroller.VerticalOffset;

            if (lstTriggeredOperations.Count > 0 && 
                ((lastScrollerOffset <= 350.0 && newScrollerOffset > 350.0) || (lastScrollerOffset >= 350.0 && newScrollerOffset < 350.0)))
            {
                ExecuteTriggerableOperations();
            }
            else
            {
                scroller.InvalidateArrange();
            }

            lastScrollerOffset = newScrollerOffset;
        }

        private void Scroller_ScrollCompleted(Scroller sender, ScrollCompletedEventArgs args)
        {
            if (chkLogScrollerEvents.IsChecked == true)
            {
                ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

                AppendAsyncEventMessage("ScrollCompleted OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
            }
        }

        private void Scroller_AnchorRequested(Scroller sender, ScrollerAnchorRequestedEventArgs args)
        {
            try
            {
                IList<UIElement> anchorCandidates = args.AnchorCandidates;

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    AppendAsyncEventMessage("AnchorRequested anchor=" + (args.AnchorElement == null ? "null" : "non-null") + ", count=" + anchorCandidates.Count);
                }

                if (anchorElement == null)
                {
                    foreach (UIElement child in stackPanel.Children)
                    {
                        anchorCandidates.Add(child);
                    }
                }
                else
                {
                    args.AnchorElement = anchorElement;
                }
            }
            catch (Exception ex)
            {
                cmbAnchorElement.SelectedIndex = 0;
                BtnSetAnchorElement_Click(null, null);

                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalAnchorRatio();
        }

        private void BtnSetHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.HorizontalAnchorRatio = Convert.ToDouble(txtHorizontalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalAnchorRatio();
        }

        private void BtnSetVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.VerticalAnchorRatio = Convert.ToDouble(txtVerticalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            txtWidth.Text = scroller.Width.ToString();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            txtHeight.Text = scroller.Height.ToString();
        }

        private void BtnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetAnchorElement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string asyncEventMessage = "AnchorElement=";

                if (anchorElement == null)
                {
                    cmbAnchorElement.SelectedIndex = 0;
                    asyncEventMessage += "null";
                }
                else if (anchorElement == scroller)
                {
                    cmbAnchorElement.SelectedIndex = 1;
                    asyncEventMessage += "scroller";
                }
                else if (anchorElement == tblCollapsedAnchorElement)
                {
                    cmbAnchorElement.SelectedIndex = 2;
                    asyncEventMessage += "collapsed";
                }
                else if (anchorElement == border)
                {
                    cmbAnchorElement.SelectedIndex = 3;
                    asyncEventMessage += "border";
                }
                else
                {
                    cmbAnchorElement.SelectedIndex = 4;
                    asyncEventMessage += "item";
                }
                AppendAsyncEventMessage(asyncEventMessage);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetAnchorElement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbAnchorElement.SelectedIndex)
                {
                    case 0:
                        anchorElement = null;
                        break;
                    case 1:
                        anchorElement = scroller;
                        break;
                    case 2:
                        anchorElement = tblCollapsedAnchorElement;
                        break;
                    case 3:
                        anchorElement = border;
                        break;
                    case 4:
                        anchorElement = stackPanel.Children[int.Parse(txtItemIndex.Text)];
                        break;
                }

                scroller.InvalidateArrange();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void CmbAnchorElement_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (tblItemIndex != null && txtItemIndex != null && cmbAnchorElement != null)
                tblItemIndex.Visibility = txtItemIndex.Visibility = cmbAnchorElement.SelectedIndex == 4 ? Visibility.Visible : Visibility.Collapsed;
        }

        private void UpdateHorizontalAnchorRatio()
        {
            txtHorizontalAnchorRatio.Text = scroller.HorizontalAnchorRatio.ToString();
        }

        private void UpdateVerticalAnchorRatio()
        {
            txtVerticalAnchorRatio.Text = scroller.VerticalAnchorRatio.ToString();
        }

        private void BtnInvalidateArrange_Click(object sender, RoutedEventArgs e)
        {
            scroller.InvalidateArrange();
        }

        private void BtnScrollTo_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtStockOffsetsChangeDuration.Text = string.Empty;

                int viewChangeId = scroller.ScrollTo(
                    chkHorizontalOrientation.IsChecked == true ? Convert.ToDouble(txtCOAO.Text) : 0,
                    chkHorizontalOrientation.IsChecked == true ? 0 : Convert.ToDouble(txtCOAO.Text),
                    new ScrollOptions(AnimationMode.Auto, SnapPointsMode.Ignore)).OffsetsChangeId;
                AppendAsyncEventMessage("Invoked ScrollTo Id=" + viewChangeId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_ScrollAnimationStarting(Scroller sender, ScrollAnimationStartingEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ScrollAnimationStarting OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId);

                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null && !string.IsNullOrWhiteSpace(txtOverriddenOffsetsChangeDuration.Text))
                {
                    txtStockOffsetsChangeDuration.Text = stockKeyFrameAnimation.Duration.TotalMilliseconds.ToString();
                    double durationOverride = Convert.ToDouble(txtOverriddenOffsetsChangeDuration.Text);
                    stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(durationOverride);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnScrollFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Vector2? inertiaDecayRate = null;

                if (txtCOWAVAIDR.Text != "null")
                {
                    inertiaDecayRate = new Vector2(
                        chkHorizontalOrientation.IsChecked == true ? Convert.ToSingle(txtCOWAVAIDR.Text) : 0,
                        chkHorizontalOrientation.IsChecked == true ? 0 : Convert.ToSingle(txtCOWAVAIDR.Text));
                }

                txtStockOffsetsChangeDuration.Text = string.Empty;

                int viewChangeId = scroller.ScrollFrom(
                    new Vector2(
                        chkHorizontalOrientation.IsChecked == true ? Convert.ToSingle(txtCOWAVAV.Text) : 0,
                        chkHorizontalOrientation.IsChecked == true ? 0 : Convert.ToSingle(txtCOWAVAV.Text)),
                    inertiaDecayRate).OffsetsChangeId;
                AppendAsyncEventMessage("Invoked ScrollFrom Id=" + viewChangeId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnClearScrollerEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollerEvents.Items.Clear();
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void UpdateRaiseAnchorNotifications(bool raiseAnchorNotifications)
        {
            if (raiseAnchorNotifications)
            {
                if (!ScrollerTestHooks.AreAnchorNotificationsRaised)
                {
                    ScrollerTestHooks.AreAnchorNotificationsRaised = true;
                    ScrollerTestHooks.AnchorEvaluated += ScrollerTestHooks_AnchorEvaluated;
                }
            }
            else
            {
                if (ScrollerTestHooks.AreAnchorNotificationsRaised)
                {
                    ScrollerTestHooks.AreAnchorNotificationsRaised = false;
                    ScrollerTestHooks.AnchorEvaluated -= ScrollerTestHooks_AnchorEvaluated;
                }

                if (currentAnchor != null)
                {
                    currentAnchor.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
                    currentAnchor = null;
                }
                cnsAnchorPoint.Visibility = Visibility.Collapsed;
            }
        }

        private void ChkHorizontalOrientation_Checked(object sender, RoutedEventArgs e)
        {
            stackPanel.Orientation = Orientation.Horizontal;
            scroller.ContentOrientation = ContentOrientation.Horizontal;
            scroller.Width = 600;
            scroller.Height = 300;
            cnsAnchorPoint.Width = 600;
            cnsAnchorPoint.Height = 300;
            Grid.SetRow(scroller, 0);
            Grid.SetColumnSpan(scroller, 4);
            Grid.SetRow(cnsAnchorPoint, 0);
            Grid.SetColumnSpan(cnsAnchorPoint, 4);

            foreach (Border border in stackPanel.Children)
            {
                border.Width = 120;
                border.Height = 170;
            }
        }

        private void ChkHorizontalOrientation_Unchecked(object sender, RoutedEventArgs e)
        {
            stackPanel.Orientation = Orientation.Vertical;
            scroller.ContentOrientation = ContentOrientation.Vertical;
            scroller.Width = 300;
            scroller.Height = 600;
            cnsAnchorPoint.Width = 300;
            cnsAnchorPoint.Height = 600;
            Grid.SetRow(scroller, 1);
            Grid.SetColumnSpan(scroller, 1);
            Grid.SetRow(cnsAnchorPoint, 1);
            Grid.SetColumnSpan(cnsAnchorPoint, 1);

            foreach (Border border in stackPanel.Children)
            {
                border.Width = 170;
                border.Height = 120;
            }
        }

        private void ChkLogScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            scroller.ExtentChanged += Scroller_ExtentChanged;
            scroller.StateChanged += Scroller_StateChanged;
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scroller.ExtentChanged -= Scroller_ExtentChanged;
            scroller.StateChanged -= Scroller_StateChanged;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string asyncEventMessage = string.Empty;
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
            }

            if (args.IsVerboseLevel)
            {
                asyncEventMessage = "Verbose: " + senderName + "m:" + msg;
            }
            else
            {
                asyncEventMessage = "Info: " + senderName + "m:" + msg;
            }

            AppendAsyncEventMessage(asyncEventMessage);
        }

        private void ScrollerTestHooks_AnchorEvaluated(Scroller sender, ScrollerTestHooksAnchorEvaluatedEventArgs args)
        {
            string anchorName;
            Border newAnchor = null;

            if (args.AnchorElement == null)
            {
                anchorName = "null";
            }
            else
            {
                if (args.AnchorElement is FrameworkElement)
                {
                    FrameworkElement anchorElementAsFE = args.AnchorElement as FrameworkElement;

                    if (string.IsNullOrWhiteSpace(anchorElementAsFE.Name))
                        anchorName = "unnamed-FE";
                    else
                        anchorName = anchorElementAsFE.Name;

                    newAnchor = anchorElementAsFE as Border;
                }
                else
                {
                    anchorName = "UIE";
                }
            }

            if (newAnchor != currentAnchor)
            {
                if (currentAnchor != null)
                {
                    currentAnchor.BorderBrush = chartreuseBrush;
                }
                if (newAnchor != null)
                {
                    newAnchor.BorderBrush = orangeBrush;
                }
                currentAnchor = newAnchor;
            }

            if (double.IsNaN(args.ViewportAnchorPointHorizontalOffset) && double.IsNaN(args.ViewportAnchorPointVerticalOffset))
            {
                cnsAnchorPoint.Visibility = Visibility.Collapsed;
            }
            else
            {
                cnsAnchorPoint.Visibility = Visibility.Visible;

                if (double.IsNaN(args.ViewportAnchorPointHorizontalOffset))
                {
                    rectAnchorPoint.Width = scroller.Width;
                    Canvas.SetLeft(rectAnchorPoint, 0);
                }
                else
                {
                    rectAnchorPoint.Width = double.IsNaN(args.ViewportAnchorPointVerticalOffset) ? 2 : 4;
                    Canvas.SetLeft(rectAnchorPoint, args.ViewportAnchorPointHorizontalOffset * scroller.ZoomFactor - scroller.HorizontalOffset - rectAnchorPoint.Width / 2);
                }

                if (double.IsNaN(args.ViewportAnchorPointVerticalOffset))
                {
                    rectAnchorPoint.Height = scroller.Height;
                    Canvas.SetTop(rectAnchorPoint, 0);
                }
                else
                {
                    rectAnchorPoint.Height = double.IsNaN(args.ViewportAnchorPointHorizontalOffset) ? 2 : 4;
                    Canvas.SetTop(rectAnchorPoint, args.ViewportAnchorPointVerticalOffset * scroller.ZoomFactor - scroller.VerticalOffset - rectAnchorPoint.Height / 2);
                }
            }

            if (chkLogScrollerAnchorNotifications.IsChecked == true)
            {
                AppendAsyncEventMessage("  AnchorEvaluated: s:" + sender.Name + ", a:" + anchorName + ", ap:(" + (int)args.ViewportAnchorPointHorizontalOffset + ", " + (int)args.ViewportAnchorPointVerticalOffset + ")");
            }
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (asyncEventReportingLock)
            {
                lstAsyncEventMessage.Add(asyncEventMessage);

                var ignored = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, AppendAsyncEventMessage);
            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in lstAsyncEventMessage)
                {
                    lstScrollerEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private enum QueuedOperationType
        {
            Insert,
            Remove,
            Replace,
            Shrink,
            Expand,
        }

        private class QueuedOperation
        {
            public QueuedOperation(QueuedOperationType type, int oldIndex, int oldCount, int newIndex, int newCount)
            {
                this.Type = type;
                this.OldIndex = oldIndex;
                this.OldCount = oldCount;
                this.NewIndex = newIndex;
                this.NewCount = newCount;
            }

            public QueuedOperationType Type { get; set; }
            public int OldIndex { get; set; }
            public int NewIndex { get; set; }
            public int OldCount { get; set; }
            public int NewCount { get; set; }
        }
    }
}
