﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using Windows.UI.Xaml.Automation;

using TabView = Microsoft.UI.Xaml.Controls.TabView;
using TabViewItem = Microsoft.UI.Xaml.Controls.TabViewItem;
using TabViewTabCloseRequestedEventArgs = Microsoft.UI.Xaml.Controls.TabViewTabCloseRequestedEventArgs;
using TabViewTabDragStartingEventArgs = Microsoft.UI.Xaml.Controls.TabViewTabDragStartingEventArgs;
using TabViewTabDragCompletedEventArgs = Microsoft.UI.Xaml.Controls.TabViewTabDragCompletedEventArgs;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
using System.Collections.ObjectModel;
using Windows.Devices.PointOfService;
using Windows.ApplicationModel.DataTransfer;
using MUXControlsTestApp.Utilities;
using System.Threading.Tasks;

namespace MUXControlsTestApp
{
    public class TabDataItem : DependencyObject
    {
        public String Header { get; set; }
        public SymbolIconSource IconSource { get; set; }
        public String Content { get; set; }
    }

    [TopLevelTestPage(Name = "TabView")]
    public sealed partial class TabViewPage : TestPage
    {
        int _newTabNumber = 1;
        SymbolIconSource _iconSource;

        public TabViewPage()
        {
            this.InitializeComponent();

            _iconSource = new SymbolIconSource();
            _iconSource.Symbol = Symbol.Placeholder;

            ObservableCollection<TabDataItem> itemSource = new ObservableCollection<TabDataItem>();
            for (int i = 0; i < 5; i++)
            {
                var item = new TabDataItem();
                item.IconSource = _iconSource;
                item.Header = "Item " + i;
                item.Content = "This is tab " + i + ".";
                itemSource.Add(item);
            }
            DataBindingTabView.TabItemsSource = itemSource;
        }

        protected async override void OnNavigatedTo(Windows.UI.Xaml.Navigation.NavigationEventArgs args) 
        {
            NotCloseableTab.Visibility = Visibility.Collapsed;
            await Task.Delay(TimeSpan.FromMilliseconds(1));
            NotCloseableTab.Visibility = Visibility.Visible;
        }

        public void IsClosableCheckBox_CheckChanged(object sender, RoutedEventArgs e)
        {
            if (FirstTab != null)
            {
                FirstTab.IsClosable = (bool)IsClosableCheckBox.IsChecked;
            }
        }

        public void AddButtonClick(object sender, object e)
        {
            if (Tabs != null)
            {
                TabViewItem item = new TabViewItem();
                item.IconSource = _iconSource;
                item.Header = "New Tab " + _newTabNumber;
                item.Content = item.Header;

                Tabs.TabItems.Add(item);

                _newTabNumber++;
            }
        }

        public void RemoveTabButton_Click(object sender, RoutedEventArgs e)
        {
            if (Tabs != null && Tabs.TabItems.Count > 0)
            {
                Tabs.TabItems.RemoveAt(Tabs.TabItems.Count - 1);
            }
        }


        public void SelectItemButton_Click(object sender, RoutedEventArgs e)
        {
            if (Tabs != null)
            {
                Tabs.SelectedItem = Tabs.TabItems[1];
            }
        }

        public void SelectIndexButton_Click(object sender, RoutedEventArgs e)
        {
            if (Tabs != null)
            {
                Tabs.SelectedIndex = 2;
            }
        }

        public void ChangeShopTextButton_Click(object sender, RoutedEventArgs e)
        {
            SecondTab.Header = "Changed";
        }

        public void CustomTooltipButton_Click(object sender, RoutedEventArgs e)
        {
            ToolTipService.SetToolTip(SecondTab, "Custom");
        }

        public void GetTab0ToolTipButton_Click(object sender, RoutedEventArgs e)
        {
            GetToolTipStringForTab(FirstTab, Tab0ToolTipTextBlock);
        }

        public void GetTab1ToolTipButton_Click(object sender, RoutedEventArgs e)
        {
            GetToolTipStringForTab(SecondTab, Tab1ToolTipTextBlock);
        }

        public void GetToolTipStringForTab(TabViewItem item, TextBlock textBlock)
        {
            var tooltip = ToolTipService.GetToolTip(item);
            if (tooltip is ToolTip)
            {
                textBlock.Text = (tooltip as ToolTip).Content.ToString();
            }
            else
            {
                textBlock.Text = tooltip.ToString();
            }
        }

        public void GetFirstTabLocationButton_Click(object sender, RoutedEventArgs e)
        {
            FrameworkElement element = FirstTab as FrameworkElement;
            while (element != null)
            {
                if (element == Tabs)
                {
                    FirstTabLocationTextBlock.Text = "FirstTabView";
                    return;
                }
                if (element == SecondTabView)
                {
                    FirstTabLocationTextBlock.Text = "SecondTabView";
                    return;
                }

                element = VisualTreeHelper.GetParent(element) as FrameworkElement;
            }

            FirstTabLocationTextBlock.Text = "";
        }

        private void TabWidthComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (Tabs != null)
            {
                switch (TabWidthComboBox.SelectedIndex)
                {
                    case 0: Tabs.TabWidthMode = Microsoft.UI.Xaml.Controls.TabViewWidthMode.SizeToContent; break;
                    case 1: Tabs.TabWidthMode = Microsoft.UI.Xaml.Controls.TabViewWidthMode.Equal; break;
                }
            }
        }

        private void TabViewSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            SelectedIndexTextBlock.Text = Tabs.SelectedIndex.ToString();
        }

        private void TabViewTabCloseRequested(object sender, Microsoft.UI.Xaml.Controls.TabViewTabCloseRequestedEventArgs e)
        {
            if ((bool)HandleTabCloseRequestedCheckBox.IsChecked)
            {
                Tabs.TabItems.Remove(e.Tab);
            }
        }

        private void FirstTab_CloseRequested(object sender, Microsoft.UI.Xaml.Controls.TabViewTabCloseRequestedEventArgs e)
        {
            if ((bool)HandleTabItemCloseRequestedCheckBox.IsChecked)
            {
                Tabs.TabItems.Remove(e.Tab);
            }
        }

        private void TabViewTabDroppedOutside(object sender, Microsoft.UI.Xaml.Controls.TabViewTabDroppedOutsideEventArgs e)
        {
            TabViewItem tab = e.Tab;
            if (tab != null)
            {
                TabDroppedOutsideTextBlock.Text = tab.Header.ToString();
            }
        }

        // Drag/drop stuff

        private const string DataIdentifier = "MyTabItem";
        private const string DataTabView = "MyTabView";

        private TabViewItem FindTabViewItemFromContent(TabView tabView, object content)
        {
            var numItems = tabView.TabItems.Count;
            for (int i = 0; i < numItems; i++)
            {
                var tabItem = tabView.ContainerFromIndex(i) as TabViewItem;
                if (tabItem.Content == content)
                {
                    return tabItem;
                }
            }
            return null;
        }

        private void OnTabDragStarting(object sender, TabViewTabDragStartingEventArgs e)
        {
            // Set the drag data to the tab
            e.Data.Properties.Add(DataIdentifier, e.Tab);
            e.Data.Properties.Add(DataTabView, sender as TabView);

            // And indicate that we can move it 
            e.Data.RequestedOperation = DataPackageOperation.Move;
        }

        private void OnTabStripDragOver(object sender, DragEventArgs e)
        {
            if (e.DataView.Properties.ContainsKey(DataIdentifier))
            {
                e.AcceptedOperation = DataPackageOperation.Move;
            }
        }

        private void OnTabStripDrop(object sender, DragEventArgs e)
        {
            // This event is called when we're dragging between different TabViews
            // It is responsible for handling the drop of the item into the second TabView

            object obj;
            object objOriginTabView;
            if (e.DataView.Properties.TryGetValue(DataIdentifier, out obj) && e.DataView.Properties.TryGetValue(DataTabView, out objOriginTabView))
            {
                // TODO - BUG: obj should never be null, but occassionally is. Why?
                if (obj == null || objOriginTabView == null)
                {
                    return;
                }

                var originTabView = objOriginTabView as TabView;
                var destinationTabView = sender as TabView;
                var destinationItems = destinationTabView.TabItems;
                var tabViewItem = obj as TabViewItem;

                if (destinationItems != null)
                {
                    // First we need to get the position in the List to drop to
                    var index = -1;

                    // Determine which items in the list our pointer is inbetween.
                    for (int i = 0; i < destinationTabView.TabItems.Count; i++)
                    {
                        var item = destinationTabView.ContainerFromIndex(i) as TabViewItem;

                        if (e.GetPosition(item).X - item.ActualWidth < 0)
                        {
                            index = i;
                            break;
                        }
                    }

                    // Remove item from the old TabView
                    originTabView.TabItems.Remove(tabViewItem);

                    if (index < 0)
                    {
                        // We didn't find a transition point, so we're at the end of the list
                        destinationItems.Add(tabViewItem);
                    }
                    else if (index < destinationTabView.TabItems.Count)
                    {
                        // Otherwise, insert at the provided index.
                        destinationItems.Insert(index, tabViewItem);
                    }

                    // Select the newly dragged tab
                    destinationTabView.SelectedItem = tabViewItem;
                }
            }
        }

        public void SetTabViewWidth_Click(object sender, RoutedEventArgs e)
        {
            Tabs.Width = 690;
        }

        public void GetScrollButtonsVisible_Click(object sender, RoutedEventArgs e)
        {
            var scrollDecrease = VisualTreeUtils.FindVisualChildByName(Tabs, "ScrollDecreaseButton") as FrameworkElement;
            var scrollIncrease = VisualTreeUtils.FindVisualChildByName(Tabs, "ScrollIncreaseButton") as FrameworkElement;
            if(scrollDecrease.Visibility == Visibility.Visible && scrollIncrease.Visibility == Visibility.Visible)
            {
                ScrollButtonsVisible.Text = "True";
            }
            else if(scrollIncrease.Visibility == Visibility.Collapsed && scrollDecrease.Visibility == Visibility.Collapsed)
            {
                ScrollButtonsVisible.Text = "False";
            }
            else
            {
                ScrollButtonsVisible.Text = "Unexpected";
            }
        }

        private void TabViewSizingPageButton_Click(object sender, RoutedEventArgs e)
        {
            this.Frame.Navigate(typeof(TabViewSizingPage));
        }

        private void ShortLongTextButton_Click(object sender, RoutedEventArgs e)
        {
            FirstTab.Header = "s";
            LongHeaderTab.Header = "long long long long long long long long";
        }
    }
}
