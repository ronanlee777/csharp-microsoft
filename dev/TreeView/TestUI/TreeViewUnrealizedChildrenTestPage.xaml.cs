﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class TreeViewUnrealizedChildrenTestPage : TestPage
    {

        public TreeViewNode VirtualizingTestRootNode;
        public CustomContent CustomContentRootNode;

        public TreeViewUnrealizedChildrenTestPage()
        {

            this.InitializeComponent();
            CustomContentRootNode = new CustomContent(3);
            VirtualizingTestRootNode = CustomContentRootNode.GetTreeViewNode();
            UnrealizedTreeViewSelection.RootNodes.Add(VirtualizingTestRootNode);
        }

        private void GetSelectedItemName_Click(object sender, RoutedEventArgs e)
        {
            SelectedItemName.Text = ((UnrealizedTreeViewSelection.SelectedItem as TreeViewNode).Content as CustomContent).ToString();
        }

        private void UnrealizedTreeViewSelection_Expanding(Microsoft.UI.Xaml.Controls.TreeView sender, Microsoft.UI.Xaml.Controls.TreeViewExpandingEventArgs args)
        {
            VirtualizingDataSource.FillTreeNode(args.Node);
        }
        private void UnrealizedTreeViewSelection_Collapsed(Microsoft.UI.Xaml.Controls.TreeView sender, Microsoft.UI.Xaml.Controls.TreeViewCollapsedEventArgs args)
        {
            VirtualizingDataSource.EmptyTreeNode(args.Node);
        }
    }
}
