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
using TabViewTabClosingEventArgs = Microsoft.UI.Xaml.Controls.TabViewTabClosingEventArgs;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TabView")]
    public sealed partial class TabViewPage : TestPage
    {
        int _newTabNumber = 1;

        public TabViewPage()
        {
            this.InitializeComponent();
        }

        public void CanCloseCheckBox_CheckChanged(object sender, RoutedEventArgs e)
        {
            if (Tabs != null)
            {
                Tabs.CanCloseTabs = (bool)CanCloseCheckBox.IsChecked;
            }
        }

        public void IsCloseableCheckBox_CheckChanged(object sender, RoutedEventArgs e)
        {
            if (FirstTab != null)
            {
                FirstTab.IsCloseable = (bool)IsCloseableCheckBox.IsChecked;
            }
        }

        public void AddButtonClick(object sender, object e)
        {
            if (Tabs != null)
            {
                TabViewItem item = new TabViewItem();
                item.Icon = new SymbolIcon(Symbol.Calendar);
                item.Header = "New Tab " + _newTabNumber;
                item.Content = item.Header;
                item.SetValue(AutomationProperties.NameProperty, item.Header);

                Tabs.Items.Add(item);

                _newTabNumber++;
            }
        }

        public void RemoveTabButton_Click(object sender, RoutedEventArgs e)
        {
            if (Tabs != null && Tabs.Items.Count > 0)
            {
                Tabs.Items.RemoveAt(Tabs.Items.Count - 1);
            }
        }


        public void SelectItemButton_Click(object sender, RoutedEventArgs e)
        {
            if (Tabs != null)
            {
                Tabs.SelectedItem = Tabs.Items[1];
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


        private void TabViewTabClosing(object sender, Microsoft.UI.Xaml.Controls.TabViewTabClosingEventArgs e)
        {
            e.Cancel = (bool)CancelCloseCheckBox.IsChecked;
        }

        private void FirstTab_TabClosing(object sender, Microsoft.UI.Xaml.Controls.TabViewTabClosingEventArgs e)
        {
            e.Cancel = (bool)CancelItemCloseCheckBox.IsChecked;
        }

        private void TabViewTabDraggedOutside(object sender, Microsoft.UI.Xaml.Controls.TabViewTabDraggedOutsideEventArgs e)
        {
            TabViewItem tab = e.Tab;
            if (tab != null)
            {
                TabDraggedOutsideTextBlock.Text = tab.Header.ToString();
            }
        }
    }
}
