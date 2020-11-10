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

using PipsControl = Microsoft.UI.Xaml.Controls.PipsControl;
using Windows.UI.Xaml.Input;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "PipsControl")]
    public sealed partial class PipsControlPage : TestPage
    {
        public PipsControlPage()
        {
            this.InitializeComponent();
        }

        private void MyPointerExited(object sender, PointerRoutedEventArgs args)
        {
            var position = args.GetCurrentPoint(pipsControl).Position;
            if (sender == VisualTreeHelper.GetChild(pipsControl, 0))
            {
                var hey = 1;
                hey++;
            }
        }

      
    }

}
