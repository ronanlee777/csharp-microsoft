﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Threading;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Composition;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests
    {
        private enum BiDirectionalAlignment
        {
            Near,
            Center,
            Stretch,
            Far
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Content.Margin and verifies InteractionTracker.MaxPosition.")]
        public void BasicMargin()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }

            const double c_Margin = 50.0;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding positive Margin to Scroller.Content");
                rectangleScrollerContent.Margin = new Thickness(c_Margin);
            });

            // Try to jump beyond maximum offsets
            ScrollTo(
                scroller,
                c_defaultUIScrollerContentWidth + 2 * c_Margin - c_defaultUIScrollerWidth + 10.0,
                c_defaultUIScrollerContentHeight + 2 * c_Margin - c_defaultUIScrollerHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                c_defaultUIScrollerContentWidth + 2 * c_Margin - c_defaultUIScrollerWidth,
                c_defaultUIScrollerContentHeight + 2 * c_Margin - c_defaultUIScrollerHeight);

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding negative Margin to Scroller.Content");
                rectangleScrollerContent.Margin = new Thickness(-c_Margin);
            });

            // Try to jump beyond maximum offsets
            ScrollTo(
                scroller,
                c_defaultUIScrollerContentWidth - 2 * c_Margin - c_defaultUIScrollerWidth + 10.0,
                c_defaultUIScrollerContentHeight - 2 * c_Margin - c_defaultUIScrollerHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false,
                isAnimationsEnabledOverride: null,
                c_defaultUIScrollerContentWidth - 2 * c_Margin - c_defaultUIScrollerWidth,
                c_defaultUIScrollerContentHeight - 2 * c_Margin - c_defaultUIScrollerHeight);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Content.Padding and verifies InteractionTracker.MaxPosition.")]
        public void BasicPadding()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because test sometimes hangs indefinitely."); // Bug 12286203
                return;
            }

            const double c_Padding = 50.0;
            Scroller scroller = null;
            Border borderScrollerContent = null;
            Rectangle rectangle = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                borderScrollerContent = new Border();
                rectangle = new Rectangle();
                scroller = new Scroller();

                borderScrollerContent.Width = c_defaultUIScrollerContentWidth;
                borderScrollerContent.Height = c_defaultUIScrollerContentHeight;
                borderScrollerContent.Child = rectangle;
                scroller.Content = borderScrollerContent;

                SetupDefaultUI(scroller, null /*rectangleScrollerContent*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding Padding to Scroller.Content");
                borderScrollerContent.Padding = new Thickness(c_Padding);
            });

            // Try to jump beyond maximum offsets
            ScrollTo(
                scroller,
                c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth + 10.0,
                c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth,
                c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Content.HorizontalAlignment/VerticalAlignment and verifies content positioning.")]
        public void BasicAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.15f;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scroller, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Stretch/Strech alignments");
                Verify.AreEqual(HorizontalAlignment.Stretch, rectangleScrollerContent.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, rectangleScrollerContent.VerticalAlignment);
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollerWidth - c_defaultUIScrollerContentWidth * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollerHeight - c_defaultUIScrollerContentHeight * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Left/Top alignments");
                rectangleScrollerContent.HorizontalAlignment = HorizontalAlignment.Left;
                rectangleScrollerContent.VerticalAlignment = VerticalAlignment.Top;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.AreEqual(0.0f, horizontalOffset);
            Verify.AreEqual(0.0f, verticalOffset);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Right/Bottom alignments");
                rectangleScrollerContent.HorizontalAlignment = HorizontalAlignment.Right;
                rectangleScrollerContent.VerticalAlignment = VerticalAlignment.Bottom;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollerWidth - c_defaultUIScrollerContentWidth * c_smallZoomFactor)) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollerHeight - c_defaultUIScrollerContentHeight * c_smallZoomFactor)) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Center/Center alignments");
                rectangleScrollerContent.HorizontalAlignment = HorizontalAlignment.Center;
                rectangleScrollerContent.VerticalAlignment = VerticalAlignment.Center;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - (float)(c_defaultUIScrollerWidth - c_defaultUIScrollerContentWidth * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - (float)(c_defaultUIScrollerHeight - c_defaultUIScrollerContentHeight * c_smallZoomFactor) / 2.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);
        }

        [TestMethod]
        [TestProperty("Description", "Validates Scroller.ViewportHeight for various layouts.")]
        public void ViewportHeight()
        {
            for (double scrollerContentHeight = 50.0; scrollerContentHeight <= 350.0; scrollerContentHeight += 300.0)
            {
                ViewportHeight(
                    isScrollerParentSizeSet: false,
                    isScrollerParentMaxSizeSet: false,
                    scrollerVerticalAlignment: VerticalAlignment.Top,
                    scrollerContentHeight);
                ViewportHeight(
                    isScrollerParentSizeSet: true,
                    isScrollerParentMaxSizeSet: false,
                    scrollerVerticalAlignment: VerticalAlignment.Top,
                    scrollerContentHeight);
                ViewportHeight(
                    isScrollerParentSizeSet: false,
                    isScrollerParentMaxSizeSet: true,
                    scrollerVerticalAlignment: VerticalAlignment.Top,
                    scrollerContentHeight);

                ViewportHeight(
                    isScrollerParentSizeSet: false,
                    isScrollerParentMaxSizeSet: false,
                    scrollerVerticalAlignment: VerticalAlignment.Center,
                    scrollerContentHeight);
                ViewportHeight(
                    isScrollerParentSizeSet: true,
                    isScrollerParentMaxSizeSet: false,
                    scrollerVerticalAlignment: VerticalAlignment.Center,
                    scrollerContentHeight);
                ViewportHeight(
                    isScrollerParentSizeSet: false,
                    isScrollerParentMaxSizeSet: true,
                    scrollerVerticalAlignment: VerticalAlignment.Center,
                    scrollerContentHeight);

                ViewportHeight(
                    isScrollerParentSizeSet: false,
                    isScrollerParentMaxSizeSet: false,
                    scrollerVerticalAlignment: VerticalAlignment.Stretch,
                    scrollerContentHeight);
                ViewportHeight(
                    isScrollerParentSizeSet: true,
                    isScrollerParentMaxSizeSet: false,
                    scrollerVerticalAlignment: VerticalAlignment.Stretch,
                    scrollerContentHeight);
                ViewportHeight(
                    isScrollerParentSizeSet: false,
                    isScrollerParentMaxSizeSet: true,
                    scrollerVerticalAlignment: VerticalAlignment.Stretch,
                    scrollerContentHeight);
            }
        }

        private void ViewportHeight(
            bool isScrollerParentSizeSet,
            bool isScrollerParentMaxSizeSet,
            VerticalAlignment scrollerVerticalAlignment,
            double scrollerContentHeight)
        {
            Log.Comment($"ViewportHeight test case - isScrollerParentSizeSet: {isScrollerParentSizeSet}, isScrollerParentMaxSizeSet: {isScrollerParentMaxSizeSet}, scrollerVerticalAlignment: {scrollerVerticalAlignment}, scrollerContentHeight: {scrollerContentHeight}");

            Border border = null;
            Scroller scroller = null;
            StackPanel stackPanel = null;
            Rectangle rectangle = null;
            AutoResetEvent borderLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangle = new Rectangle()
                {
                    Width = 30,
                    Height = scrollerContentHeight
                };

                stackPanel = new StackPanel()
                {
                    BorderThickness = new Thickness(5),
                    Margin = new Thickness(7),
                    VerticalAlignment = VerticalAlignment.Top
                };
                stackPanel.Children.Add(rectangle);

                scroller = new Scroller()
                {
                    Content = stackPanel,
                    ContentOrientation = ContentOrientation.Vertical,
                    VerticalAlignment = scrollerVerticalAlignment
                };

                border = new Border()
                {
                    BorderThickness = new Thickness(2),
                    Margin = new Thickness(3),
                    VerticalAlignment = VerticalAlignment.Center,
                    Child = scroller
                };
                if (isScrollerParentSizeSet)
                {
                    border.Width = 300.0;
                    border.Height = 200.0;
                }
                if (isScrollerParentMaxSizeSet)
                {
                    border.MaxWidth = 300.0;
                    border.MaxHeight = 200.0;
                }

                border.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Border.Loaded event handler");
                    borderLoadedEvent.Set();
                };

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = border;
            });

            WaitForEvent("Waiting for Border.Loaded event", borderLoadedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                double expectedViewportHeight = 
                    rectangle.Height + stackPanel.BorderThickness.Top + stackPanel.BorderThickness.Bottom +
                    stackPanel.Margin.Top + stackPanel.Margin.Bottom;

                double borderChildAvailableHeight = border.ActualHeight - border.BorderThickness.Top - border.BorderThickness.Bottom;

                if (expectedViewportHeight > borderChildAvailableHeight || scrollerVerticalAlignment == VerticalAlignment.Stretch)
                {
                    expectedViewportHeight = borderChildAvailableHeight;
                }

                Log.Comment($"border.ActualWidth: {border.ActualWidth}, border.ActualHeight: {border.ActualHeight}");
                Log.Comment($"Checking ViewportWidth - scroller.ViewportWidth: {scroller.ViewportWidth}, scroller.ActualWidth: {scroller.ActualWidth}");
                Verify.AreEqual(scroller.ViewportWidth, scroller.ActualWidth);

                Log.Comment($"Checking ViewportHeight - expectedViewportHeight: {expectedViewportHeight}, scroller.ViewportHeight: {scroller.ViewportHeight}, scroller.ActualHeight: {scroller.ActualHeight}");
                Verify.AreEqual(expectedViewportHeight, scroller.ViewportHeight);
                Verify.AreEqual(scroller.ViewportHeight, scroller.ActualHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Uses a StackPanel with Stretch alignment as Scroller.Content to verify it stretched to the size of the Scroller.")]
        public void StretchAlignment()
        {
            Scroller scroller = null;
            StackPanel stackPanel = null;
            Rectangle rectangle = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangle = new Rectangle();
                rectangle.Height = c_defaultUIScrollerContentHeight;
                stackPanel = new StackPanel();
                stackPanel.Children.Add(rectangle);
                scroller = new Scroller();
                scroller.Width = c_defaultUIScrollerWidth;
                scroller.Height = c_defaultUIScrollerHeight;
                scroller.Content = stackPanel;

                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scroller;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Checking Stretch/Strech alignments");
                Verify.AreEqual(HorizontalAlignment.Stretch, stackPanel.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, stackPanel.VerticalAlignment);

                Log.Comment("Checking StackPanel size");
                Verify.AreEqual(c_defaultUIScrollerWidth, stackPanel.ActualWidth);
                Verify.AreEqual(c_defaultUIScrollerContentHeight, stackPanel.ActualHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Content.HorizontalAlignment/VerticalAlignment and Scroller.Content.Margin and verifies content positioning.")]
        public void BasicMarginAndAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.15f;
            const double c_Margin = 40.0;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);

                Log.Comment("Adding positive Margin to Scroller.Content");
                rectangleScrollerContent.Margin = new Thickness(c_Margin);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scroller, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Stretch/Strech alignments");
                Verify.AreEqual(HorizontalAlignment.Stretch, rectangleScrollerContent.HorizontalAlignment);
                Verify.AreEqual(VerticalAlignment.Stretch, rectangleScrollerContent.VerticalAlignment);

            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 20.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 15.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Left/Top alignments");
                rectangleScrollerContent.HorizontalAlignment = HorizontalAlignment.Left;
                rectangleScrollerContent.VerticalAlignment = VerticalAlignment.Top;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset + 34.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset + 34.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Right/Bottom alignments");
                rectangleScrollerContent.HorizontalAlignment = HorizontalAlignment.Right;
                rectangleScrollerContent.VerticalAlignment = VerticalAlignment.Bottom;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 74.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 64.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering Center/Center alignments");
                rectangleScrollerContent.HorizontalAlignment = HorizontalAlignment.Center;
                rectangleScrollerContent.VerticalAlignment = VerticalAlignment.Center;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Verify.IsTrue(Math.Abs(horizontalOffset - 20.0f) < c_defaultOffsetResultTolerance);
            Verify.IsTrue(Math.Abs(verticalOffset - 15.0f) < c_defaultOffsetResultTolerance);
            Verify.AreEqual(c_smallZoomFactor, zoomFactor);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Content to an unsized and stretched Image, verifies resulting Scroller extents.")]
        public void StretchedImage()
        {
            Scroller scroller = null;
            Image imageScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            const double margin = 10.0;

            RunOnUIThread.Execute(() =>
            {
                Uri uri = new Uri("ms-appx:/Assets/ingredient8.png");
                Verify.IsNotNull(uri);
                imageScrollerContent = new Image();
                imageScrollerContent.Source = new BitmapImage(uri);
                imageScrollerContent.Margin = new Thickness(margin);

                scroller = new Scroller();
                scroller.Content = imageScrollerContent;

                SetupDefaultUI(scroller, rectangleScrollerContent: null, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.ContentOrientation, ContentOrientation.None);
                // Image is unconstrained and stretches to largest square contained in the 300 x 200 viewport: 200 x 200.
                ValidateStretchedImageSize(
                    scroller,
                    imageScrollerContent,
                    desiredSize: 80.0 + 2.0 * margin /*natural size + margins*/,
                    actualSize: c_defaultUIScrollerHeight - 2.0 * margin,
                    extentSize: c_defaultUIScrollerHeight);

                Log.Comment("Changing Scroller.ContentOrientation to Vertical.");
                scroller.ContentOrientation = ContentOrientation.Vertical;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.ContentOrientation, ContentOrientation.Vertical);
                // Image is constrained horizontally to 300 and stretches to the 300 x 300 square.
                ValidateStretchedImageSize(
                    scroller,
                    imageScrollerContent,
                    desiredSize: c_defaultUIScrollerWidth,
                    actualSize: c_defaultUIScrollerWidth - 2.0 * margin,
                    extentSize: c_defaultUIScrollerWidth);

                Log.Comment("Changing Scroller.ContentOrientation to Horizontal.");
                scroller.ContentOrientation = ContentOrientation.Horizontal;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.ContentOrientation, ContentOrientation.Horizontal);
                // Image is constrained vertically to 200 and stretches to the 200 x 200 square.
                ValidateStretchedImageSize(
                    scroller,
                    imageScrollerContent,
                    desiredSize: c_defaultUIScrollerHeight,
                    actualSize: c_defaultUIScrollerHeight - 2.0 * margin,
                    extentSize: c_defaultUIScrollerHeight);
            });
        }

        private void ValidateStretchedImageSize(
            Scroller scroller,
            Image imageScrollerContent,
            double desiredSize,
            double actualSize,
            double extentSize)
        {
            Log.Comment($"Sizes with Scroller.ContentOrientation={scroller.ContentOrientation}");
            Log.Comment($"Image DesiredSize=({imageScrollerContent.DesiredSize.Width} x {imageScrollerContent.DesiredSize.Height})");
            Log.Comment($"Image RenderSize=({imageScrollerContent.RenderSize.Width} x {imageScrollerContent.RenderSize.Height})");
            Log.Comment($"Image ActualSize=({imageScrollerContent.ActualWidth} x {imageScrollerContent.ActualHeight})");
            Log.Comment($"Scroller ExtentSize=({scroller.ExtentWidth} x {scroller.ExtentHeight})");

            Verify.AreEqual(imageScrollerContent.DesiredSize.Width, desiredSize);
            Verify.AreEqual(imageScrollerContent.DesiredSize.Height, desiredSize);
            Verify.AreEqual(imageScrollerContent.RenderSize.Width, actualSize);
            Verify.AreEqual(imageScrollerContent.RenderSize.Height, actualSize);
            Verify.AreEqual(imageScrollerContent.ActualWidth, actualSize);
            Verify.AreEqual(imageScrollerContent.ActualHeight, actualSize);
            Verify.AreEqual(scroller.ExtentWidth, extentSize);
            Verify.AreEqual(scroller.ExtentHeight, extentSize);
        }

        [TestMethod]
        [TestProperty("Description", "Sets Scroller.Content to Image with unnatural size and verifies InteractionTracker.MaxPosition.")]
        public void ImageWithUnnaturalSize()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }

            const double c_UnnaturalImageWidth = 1200.0;
            const double c_UnnaturalImageHeight = 1000.0;
            Scroller scroller = null;
            Image imageScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                imageScrollerContent = new Image();
                scroller = new Scroller();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollerContent.Source = new BitmapImage(uri);
                imageScrollerContent.Width = c_UnnaturalImageWidth;
                imageScrollerContent.Height = c_UnnaturalImageHeight;
                scroller.Content = imageScrollerContent;

                SetupDefaultUI(scroller, null /*rectangleScrollerContent*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Try to jump beyond maximum offsets
            ScrollTo(
                scroller,
                c_UnnaturalImageWidth - c_defaultUIScrollerWidth + 10.0,
                c_UnnaturalImageHeight - c_defaultUIScrollerHeight + 10.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                c_UnnaturalImageWidth - c_defaultUIScrollerWidth,
                c_UnnaturalImageHeight - c_defaultUIScrollerHeight);
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets Scroller.ContentOrientation to Vertical and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedWidth()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.5f;
            const float c_largeZoomFactor = 2.0f;
            const double c_imageHeight = 300.0;
            const double c_scrollerWidth = 200.0;
            const double c_leftMargin = 20.0;
            const double c_rightMargin = 30.0;
            Scroller scroller = null;
            Image imageScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                imageScrollerContent = new Image();
                scroller = new Scroller();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollerContent.Source = new BitmapImage(uri);
                imageScrollerContent.Margin = new Thickness(c_leftMargin, 0, c_rightMargin, 0);
                scroller.Content = imageScrollerContent;
                scroller.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                SetupDefaultUI(scroller, null /*rectangleScrollerContent*/, scrollerLoadedEvent);

                // Constraining the Image width and making the Scroller smaller than the Image
                imageScrollerContent.Height = c_imageHeight;
                scroller.ContentOrientation = ContentOrientation.Vertical;
                scroller.Width = c_scrollerWidth;
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scroller, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Stretch,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: (float)(-c_scrollerWidth * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Left,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Right,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: (float)(-c_scrollerWidth * c_smallZoomFactor), // -100
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Center,
                expectedVerticalOffset: (float)(c_defaultUIScrollerHeight - c_imageHeight * c_smallZoomFactor) / 2.0f, // (200 - 300 * 0.5) / 2 = 25
                expectedMinPosition: (float)(-c_scrollerWidth * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            // Jump to absolute large zoomFactor to make the content larger than the viewport.
            ZoomTo(scroller, c_largeZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Stretch,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Left,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Right,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedWidth(
                compositor,
                scroller,
                content: imageScrollerContent,
                horizontalAlignment: HorizontalAlignment.Center,
                expectedVerticalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);
        }

        [TestMethod]
        [TestProperty("Description",
            "Sets Scroller.IsChildAvailableHeightConstrained to True and verifies Image positioning for various alignments and zoom factors.")]
        public void ImageWithConstrainedHeight()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const float c_smallZoomFactor = 0.5f;
            const float c_largeZoomFactor = 2.0f;
            const double c_imageWidth = 250.0;
            const double c_topMargin = 40.0;
            const double c_bottomMargin = 10.0;
            Scroller scroller = null;
            Image imageScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                imageScrollerContent = new Image();
                scroller = new Scroller();

                Uri uri = new Uri("ms-appx:/Assets/ingredient3.png");
                Verify.IsNotNull(uri);
                imageScrollerContent.Source = new BitmapImage(uri);
                imageScrollerContent.Margin = new Thickness(0, c_topMargin, 0, c_bottomMargin);
                scroller.Content = imageScrollerContent;
                scroller.Background = new Media.SolidColorBrush(Colors.Chartreuse);

                SetupDefaultUI(scroller, rectangleScrollerContent: null, scrollerLoadedEvent: scrollerLoadedEvent);

                // Constraining the Image height and making the Scroller smaller than the Image
                imageScrollerContent.Width = c_imageWidth;
                scroller.ContentOrientation = ContentOrientation.Horizontal;
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute small zoomFactor to make the content smaller than the viewport.
            ZoomTo(scroller, c_smallZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Stretch,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: (float)(-c_defaultUIScrollerHeight * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Top,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Bottom,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: (float)(-c_defaultUIScrollerHeight * c_smallZoomFactor), // -100
                expectedZoomFactor: c_smallZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Center,
                expectedHorizontalOffset: (float)(c_defaultUIScrollerWidth - c_imageWidth * c_smallZoomFactor) / 2.0f, // 87.5
                expectedMinPosition: (float)(-c_defaultUIScrollerHeight * c_smallZoomFactor / 2.0), // -50
                expectedZoomFactor: c_smallZoomFactor);

            // Jump to absolute large zoomFactor to make the content larger than the viewport.
            ZoomTo(scroller, c_largeZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Stretch,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Top,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Bottom,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);

            ValidateContentWithConstrainedHeight(
                compositor,
                scroller,
                content: imageScrollerContent,
                verticalAlignment: VerticalAlignment.Center,
                expectedHorizontalOffset: 0.0f,
                expectedMinPosition: 0.0f,
                expectedZoomFactor: c_largeZoomFactor);
        }

        private void ValidateContentWithConstrainedWidth(
            Compositor compositor,
            Scroller scroller,
            FrameworkElement content,
            HorizontalAlignment horizontalAlignment,
            float expectedVerticalOffset,
            float expectedMinPosition,
            float expectedZoomFactor)
        {
            const double c_leftMargin = 20.0;
            const double c_rightMargin = 30.0;
            const double c_scrollerWidth = 200.0;

            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            double arrangeRenderSizesDelta = 0.0;
            double expectedHorizontalOffset = 0.0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering alignment " + horizontalAlignment.ToString());
                content.HorizontalAlignment = horizontalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(horizontalAlignment, content.HorizontalAlignment);
                Verify.AreEqual(c_scrollerWidth, content.DesiredSize.Width); // 200
                Verify.AreEqual(c_scrollerWidth - c_leftMargin - c_rightMargin, content.RenderSize.Width); // 200 - 20 - 30 = 150
                arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromHorizontalAlignment(horizontalAlignment),
                    extentSize: c_scrollerWidth,
                    renderSize: c_scrollerWidth - c_leftMargin - c_rightMargin,
                    nearMargin: c_leftMargin,
                    farMargin: c_rightMargin);
                Verify.AreEqual(20.0, arrangeRenderSizesDelta);
                expectedHorizontalOffset = -expectedMinPosition + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Log.Comment("horizontalOffset={0}, verticalOffset={1}, zoomFactor={2}",
                horizontalOffset, verticalOffset, zoomFactor);
            Log.Comment("expectedHorizontalOffset={0}, expectedVerticalOffset={1}, expectedZoomFactor={2}",
                expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor);
            Verify.AreEqual(expectedHorizontalOffset, horizontalOffset);
            Verify.AreEqual(expectedVerticalOffset, verticalOffset);
            Verify.AreEqual(expectedZoomFactor, zoomFactor);
        }

        private void ValidateContentWithConstrainedHeight(
            Compositor compositor,
            Scroller scroller,
            FrameworkElement content,
            VerticalAlignment verticalAlignment,
            float expectedHorizontalOffset,
            float expectedMinPosition,
            float expectedZoomFactor)
        {
            const double c_topMargin = 40.0;
            const double c_bottomMargin = 10.0;

            float horizontalOffset = 0.0f;
            float verticalOffset = 0.0f;
            float zoomFactor = 1.0f;
            double arrangeRenderSizesDelta = 0.0;
            double expectedVerticalOffset = 0.0;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Covering alignment " + verticalAlignment.ToString());
                content.VerticalAlignment = verticalAlignment;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(verticalAlignment, content.VerticalAlignment);
                Verify.AreEqual(c_defaultUIScrollerHeight, content.DesiredSize.Height); // 200
                Verify.AreEqual(c_defaultUIScrollerHeight - c_topMargin - c_bottomMargin, content.RenderSize.Height); // 200 - 40 - 10 = 150
                arrangeRenderSizesDelta = GetArrangeRenderSizesDelta(
                    BiDirectionalAlignmentFromVerticalAlignment(verticalAlignment),
                    extentSize: c_defaultUIScrollerHeight,
                    renderSize: c_defaultUIScrollerHeight - c_topMargin - c_bottomMargin,
                    nearMargin: c_topMargin,
                    farMargin: c_bottomMargin);
                Verify.AreEqual(40.0, arrangeRenderSizesDelta);
                expectedVerticalOffset = -expectedMinPosition + (expectedZoomFactor - 1.0f) * arrangeRenderSizesDelta;
            });

            SpyTranslationAndScale(scroller, compositor, out horizontalOffset, out verticalOffset, out zoomFactor);

            Log.Comment("horizontalOffset={0}, verticalOffset={1}, zoomFactor={2}",
                horizontalOffset, verticalOffset, zoomFactor);
            Log.Comment("expectedHorizontalOffset={0}, expectedVerticalOffset={1}, expectedZoomFactor={2}",
                expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor);
            Verify.AreEqual(expectedHorizontalOffset, horizontalOffset);
            Verify.AreEqual(expectedVerticalOffset, verticalOffset);
            Verify.AreEqual(expectedZoomFactor, zoomFactor);
        }

        private double GetArrangeRenderSizesDelta(
            BiDirectionalAlignment alignment,
            double extentSize,
            double renderSize,
            double nearMargin,
            double farMargin)
        {
            double delta = (alignment == BiDirectionalAlignment.Stretch) ? 0.0 : extentSize - renderSize;

            if (alignment == BiDirectionalAlignment.Center ||
                alignment == BiDirectionalAlignment.Far)
            {
                delta -= nearMargin + farMargin;
            }

            if (alignment == BiDirectionalAlignment.Center)
            {
                delta /= 2.0f;
            }
            else if (alignment == BiDirectionalAlignment.Near)
            {
                delta = 0.0f;
            }

            delta += nearMargin;

            Log.Comment("GetArrangeRenderSizesDelta returns {0}.", delta);
            return delta;
        }

        private BiDirectionalAlignment BiDirectionalAlignmentFromHorizontalAlignment(HorizontalAlignment horizontalAlignment)
        {
            switch (horizontalAlignment)
            {
                case HorizontalAlignment.Stretch:
                    return BiDirectionalAlignment.Stretch;
                case HorizontalAlignment.Left:
                    return BiDirectionalAlignment.Near;
                case HorizontalAlignment.Right:
                    return BiDirectionalAlignment.Far;
                default:
                    return BiDirectionalAlignment.Center;
            }
        }

        private BiDirectionalAlignment BiDirectionalAlignmentFromVerticalAlignment(VerticalAlignment verticalAlignment)
        {
            switch (verticalAlignment)
            {
                case VerticalAlignment.Stretch:
                    return BiDirectionalAlignment.Stretch;
                case VerticalAlignment.Top:
                    return BiDirectionalAlignment.Near;
                case VerticalAlignment.Bottom:
                    return BiDirectionalAlignment.Far;
                default:
                    return BiDirectionalAlignment.Center;
            }
        }
    }
}
