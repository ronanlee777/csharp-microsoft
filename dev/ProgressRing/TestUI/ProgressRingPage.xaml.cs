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

using ProgressRing = Microsoft.UI.Xaml.Controls.ProgressRing;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ProgressRing")]
    public sealed partial class ProgressRingPage : TestPage
    {
        public ProgressRingPage()
        {
            this.InitializeComponent();
            Loaded += ProgressRingPage_Loaded;
        }

        private void ProgressRingPage_Loaded(object sender, RoutedEventArgs e)
        {
            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestProgressRing, 0);

            VisualStateManager.GetVisualStateGroups(layoutRoot)[0].CurrentStateChanged += this.ProgressRingPage_CurrentStateChanged;
            VisualStateText.Text = VisualStateManager.GetVisualStateGroups(layoutRoot)[0].CurrentState.Name;

            var animatedVisualPlayer = (Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)VisualTreeHelper.GetChild(layoutRoot, 0);

            IsPlayingText.Text = animatedVisualPlayer.IsPlaying.ToString();

            Loaded -= ProgressRingPage_Loaded;
        }

        private void ProgressRingPage_CurrentStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            VisualStateText.Text = e.NewState.Name;

            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestProgressRing, 0);
            var animatedVisualPlayer = (Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)VisualTreeHelper.GetChild(layoutRoot, 0);
            IsPlayingText.Text = animatedVisualPlayer.IsPlaying.ToString();
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestProgressRing.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
            TestProgressRing.Height = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }
    }
}
