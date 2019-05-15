﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Numerics;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Composition.Interactions;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Input;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using InteractionState = Microsoft.UI.Xaml.Controls.InteractionState;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class ScrollerTests
    {
        private const InteractionState c_defaultState = InteractionState.Idle;
        private const ChainingMode c_defaultHorizontalScrollChainingMode = ChainingMode.Auto;
        private const ChainingMode c_defaultVerticalScrollChainingMode = ChainingMode.Auto;
        private const RailingMode c_defaultHorizontalScrollRailingMode = RailingMode.Enabled;
        private const RailingMode c_defaultVerticalScrollRailingMode = RailingMode.Enabled;
#if USE_SCROLLMODE_AUTO
        private const ScrollMode c_defaultHorizontalScrollMode = ScrollMode.Auto;
        private const ScrollMode c_defaultVerticalScrollMode = ScrollMode.Auto;
#else
        private const ScrollMode c_defaultHorizontalScrollMode = ScrollMode.Enabled;
        private const ScrollMode c_defaultVerticalScrollMode = ScrollMode.Enabled;
#endif
        private const ChainingMode c_defaultZoomChainingMode = ChainingMode.Auto;
        private const ZoomMode c_defaultZoomMode = ZoomMode.Disabled;
        private const InputKind c_defaultIgnoredInputKind = InputKind.None;
        private const ContentOrientation c_defaultContentOrientation = ContentOrientation.None;
        private const double c_defaultMinZoomFactor = 0.1;
        private const double c_defaultZoomFactor = 1.0;
        private const double c_defaultMaxZoomFactor = 10.0;
        private const double c_defaultHorizontalOffset = 0.0;
        private const double c_defaultVerticalOffset = 0.0;
        private const double c_defaultAnchorRatio = 0.0;

        private const double c_defaultUIScrollerContentWidth = 1200.0;
        private const double c_defaultUIScrollerContentHeight = 600.0;
        private const double c_defaultUIScrollerWidth = 300.0;
        private const double c_defaultUIScrollerHeight = 200.0;
        private const double c_defaultUIScrollControllerThickness = 44.0;

        private const float c_defaultOffsetResultTolerance = 0.0001f;

        private const string c_visualHorizontalOffsetTargetedPropertyName = "Translation.X";
        private const string c_visualVerticalOffsetTargetedPropertyName = "Translation.Y";
        private const string c_visualScaleTargetedPropertyName = "Scale.X";

        private const string c_expressionAnimationSourcesExtentPropertyName = "Extent";
        private const string c_expressionAnimationSourcesViewportPropertyName = "Viewport";
        private const string c_expressionAnimationSourcesOffsetPropertyName = "Offset";
        private const string c_expressionAnimationSourcesPositionPropertyName = "Position";
        private const string c_expressionAnimationSourcesMinPositionPropertyName = "MinPosition";
        private const string c_expressionAnimationSourcesMaxPositionPropertyName = "MaxPosition";
        private const string c_expressionAnimationSourcesZoomFactorPropertyName = "ZoomFactor";

        [TestMethod]
        [TestProperty("Description", "Verifies the Scroller default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();
                Verify.IsNotNull(scroller);

                Log.Comment("Verifying Scroller default property values");
                Verify.IsNull(scroller.HorizontalScrollController);
                Verify.IsNull(scroller.VerticalScrollController);
                Verify.IsNull(scroller.Content);
                Verify.IsNotNull(scroller.ExpressionAnimationSources);
                Verify.AreEqual(scroller.State, c_defaultState);
                Verify.AreEqual(scroller.HorizontalScrollChainingMode, c_defaultHorizontalScrollChainingMode);
                Verify.AreEqual(scroller.VerticalScrollChainingMode, c_defaultVerticalScrollChainingMode);
                Verify.AreEqual(scroller.HorizontalScrollRailingMode, c_defaultHorizontalScrollRailingMode);
                Verify.AreEqual(scroller.VerticalScrollRailingMode, c_defaultVerticalScrollRailingMode);
                Verify.AreEqual(scroller.HorizontalScrollMode, c_defaultHorizontalScrollMode);
                Verify.AreEqual(scroller.VerticalScrollMode, c_defaultVerticalScrollMode);
                Verify.AreEqual(scroller.ZoomChainingMode, c_defaultZoomChainingMode);
                Verify.AreEqual(scroller.ContentOrientation, c_defaultContentOrientation);
                Verify.AreEqual(scroller.ZoomMode, c_defaultZoomMode);
                Verify.AreEqual(scroller.IgnoredInputKind, c_defaultIgnoredInputKind);
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);
                Verify.AreEqual(scroller.ZoomFactor, c_defaultZoomFactor);
                Verify.AreEqual(scroller.HorizontalOffset, c_defaultHorizontalOffset);
                Verify.AreEqual(scroller.VerticalOffset, c_defaultVerticalOffset);
                Verify.AreEqual(scroller.HorizontalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scroller.VerticalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scroller.ExtentWidth, 0.0);
                Verify.AreEqual(scroller.ExtentHeight, 0.0);
                Verify.AreEqual(scroller.ViewportWidth, 0.0);
                Verify.AreEqual(scroller.ViewportHeight, 0.0);
                Verify.AreEqual(scroller.ScrollableWidth, 0.0);
                Verify.AreEqual(scroller.ScrollableHeight, 0.0);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Exercises the Scroller property setters and getters for non-default values.")]
        public void VerifyPropertyGettersAndSetters()
        {
            Scroller scroller = null;
            Rectangle rectangle = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();
                Verify.IsNotNull(scroller);

                rectangle = new Rectangle();
                Verify.IsNotNull(rectangle);

                Log.Comment("Setting Scroller properties to non-default values");
                scroller.Content = rectangle;
                scroller.HorizontalScrollChainingMode = ChainingMode.Always;
                scroller.VerticalScrollChainingMode = ChainingMode.Never;
                scroller.HorizontalScrollRailingMode = RailingMode.Disabled;
                scroller.VerticalScrollRailingMode = RailingMode.Disabled;
                scroller.HorizontalScrollMode = ScrollMode.Disabled;
                scroller.VerticalScrollMode = ScrollMode.Disabled;
                scroller.ZoomChainingMode = ChainingMode.Never;
                scroller.ZoomMode = ZoomMode.Enabled;
                scroller.IgnoredInputKind = InputKind.MouseWheel;
                scroller.ContentOrientation = ContentOrientation.Horizontal;
                scroller.MinZoomFactor = 0.5f;
                scroller.MaxZoomFactor = 2.0f;
                scroller.HorizontalAnchorRatio = 0.25f;
                scroller.VerticalAnchorRatio = 0.75f;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying Scroller non-default property values");
                Verify.AreEqual(scroller.Content, rectangle);
                Verify.AreEqual(scroller.State, c_defaultState);
                Verify.AreEqual(scroller.HorizontalScrollChainingMode, ChainingMode.Always);
                Verify.AreEqual(scroller.VerticalScrollChainingMode, ChainingMode.Never);
                Verify.AreEqual(scroller.HorizontalScrollRailingMode, RailingMode.Disabled);
                Verify.AreEqual(scroller.VerticalScrollRailingMode, RailingMode.Disabled);
                Verify.AreEqual(scroller.HorizontalScrollMode, ScrollMode.Disabled);
                Verify.AreEqual(scroller.VerticalScrollMode, ScrollMode.Disabled);
                Verify.AreEqual(scroller.ZoomChainingMode, ChainingMode.Never);
                Verify.AreEqual(scroller.ZoomMode, ZoomMode.Enabled);
                Verify.AreEqual(scroller.IgnoredInputKind, InputKind.MouseWheel);
                Verify.AreEqual(scroller.ContentOrientation, ContentOrientation.Horizontal);
                Verify.AreEqual(scroller.MinZoomFactor, 0.5f);
                Verify.AreEqual(scroller.MaxZoomFactor, 2.0f);
                Verify.AreEqual(scroller.HorizontalAnchorRatio, 0.25f);
                Verify.AreEqual(scroller.VerticalAnchorRatio, 0.75f);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the Scroller ExtentWidth/Height, ViewportWidth/Height and ScrollableWidth/Height properties.")]
        public void VerifyExtentAndViewportProperties()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.ExtentWidth, c_defaultUIScrollerContentWidth);
                Verify.AreEqual(scroller.ExtentHeight, c_defaultUIScrollerContentHeight);
                Verify.AreEqual(scroller.ViewportWidth, c_defaultUIScrollerWidth);
                Verify.AreEqual(scroller.ViewportHeight, c_defaultUIScrollerHeight);
                Verify.AreEqual(scroller.ScrollableWidth, c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth);
                Verify.AreEqual(scroller.ScrollableHeight, c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid Scroller.MinZoomFactor values.")]
        public void SetInvalidMinZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();

                Log.Comment("Attempting to set Scroller.MinZoomFactor to double.NaN");
                try
                {
                    scroller.MinZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);

                Log.Comment("Attempting to set Scroller.MinZoomFactor to double.NegativeInfinity");
                try
                {
                    scroller.MinZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);

                Log.Comment("Attempting to set Scroller.MinZoomFactor to double.PositiveInfinity");
                try
                {
                    scroller.MinZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to set invalid Scroller.MaxZoomFactor values.")]
        public void SetInvalidMaxZoomFactorValues()
        {
            RunOnUIThread.Execute(() =>
            {
                Scroller scroller = new Scroller();

                Log.Comment("Attempting to set Scroller.MaxZoomFactor to double.NaN");
                try
                {
                    scroller.MaxZoomFactor = double.NaN;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);

                Log.Comment("Attempting to set Scroller.MaxZoomFactor to double.NegativeInfinity");
                try
                {
                    scroller.MaxZoomFactor = double.NegativeInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);

                Log.Comment("Attempting to set Scroller.MaxZoomFactor to double.PositiveInfinity");
                try
                {
                    scroller.MaxZoomFactor = double.PositiveInfinity;
                }
                catch (Exception e)
                {
                    Log.Comment("Exception thrown: {0}", e.ToString());
                }
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the InteractionTracker's VisualInteractionSource properties get set according to Scroller properties.")]
        public void VerifyInteractionSourceSettings()
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: true,
                enableExpressionAnimationStatusNotifications : false))
            {
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
                    CompositionInteractionSourceCollection interactionSources = scrollerTestHooksHelper.GetInteractionSources(scroller);
                    Verify.IsNotNull(interactionSources);
                    ScrollerTestHooksHelper.LogInteractionSources(interactionSources);
                    Verify.AreEqual(interactionSources.Count, 1);
                    IEnumerator<ICompositionInteractionSource> sourcesEnumerator = (interactionSources as IEnumerable<ICompositionInteractionSource>).GetEnumerator();
                    sourcesEnumerator.MoveNext();
                    VisualInteractionSource visualInteractionSource = sourcesEnumerator.Current as VisualInteractionSource;
                    Verify.IsNotNull(visualInteractionSource);

                    Verify.AreEqual(visualInteractionSource.ManipulationRedirectionMode,
                        PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5) ? VisualInteractionSourceRedirectionMode.CapableTouchpadAndPointerWheel : VisualInteractionSourceRedirectionMode.CapableTouchpadOnly);
                    Verify.IsTrue(visualInteractionSource.IsPositionXRailsEnabled);
                    Verify.IsTrue(visualInteractionSource.IsPositionYRailsEnabled);
                    Verify.AreEqual(visualInteractionSource.PositionXChainingMode, InteractionChainingMode.Auto);
                    Verify.AreEqual(visualInteractionSource.PositionYChainingMode, InteractionChainingMode.Auto);
                    Verify.AreEqual(visualInteractionSource.ScaleChainingMode, InteractionChainingMode.Auto);
                    Verify.AreEqual(visualInteractionSource.PositionXSourceMode, InteractionSourceMode.EnabledWithInertia);
                    Verify.AreEqual(visualInteractionSource.PositionYSourceMode, InteractionSourceMode.EnabledWithInertia);
                    Verify.AreEqual(visualInteractionSource.ScaleSourceMode, InteractionSourceMode.Disabled);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionXSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionYSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.ScaleSourceMode, InteractionSourceRedirectionMode.Enabled);
                    }

                    Log.Comment("Changing Scroller properties that affect the primary VisualInteractionSource");
                    scroller.HorizontalScrollChainingMode = ChainingMode.Always;
                    scroller.VerticalScrollChainingMode = ChainingMode.Never;
                    scroller.HorizontalScrollRailingMode = RailingMode.Disabled;
                    scroller.VerticalScrollRailingMode = RailingMode.Disabled;
                    scroller.HorizontalScrollMode = ScrollMode.Enabled;
                    scroller.VerticalScrollMode = ScrollMode.Disabled;
                    scroller.ZoomChainingMode = ChainingMode.Never;
                    scroller.ZoomMode = ZoomMode.Enabled;
                    scroller.IgnoredInputKind = InputKind.All & ~InputKind.Touch;
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    CompositionInteractionSourceCollection interactionSources = scrollerTestHooksHelper.GetInteractionSources(scroller);
                    Verify.IsNotNull(interactionSources);
                    ScrollerTestHooksHelper.LogInteractionSources(interactionSources);
                    Verify.AreEqual(interactionSources.Count, 1);
                    IEnumerator<ICompositionInteractionSource> sourcesEnumerator = (interactionSources as IEnumerable<ICompositionInteractionSource>).GetEnumerator();
                    sourcesEnumerator.MoveNext();
                    VisualInteractionSource visualInteractionSource = sourcesEnumerator.Current as VisualInteractionSource;
                    Verify.IsNotNull(visualInteractionSource);

                    Verify.AreEqual(visualInteractionSource.ManipulationRedirectionMode, VisualInteractionSourceRedirectionMode.CapableTouchpadOnly);
                    Verify.IsFalse(visualInteractionSource.IsPositionXRailsEnabled);
                    Verify.IsFalse(visualInteractionSource.IsPositionYRailsEnabled);
                    Verify.AreEqual(visualInteractionSource.PositionXChainingMode, InteractionChainingMode.Always);
                    Verify.AreEqual(visualInteractionSource.PositionYChainingMode, InteractionChainingMode.Never);
                    Verify.AreEqual(visualInteractionSource.ScaleChainingMode, InteractionChainingMode.Never);
                    Verify.AreEqual(visualInteractionSource.PositionXSourceMode, InteractionSourceMode.EnabledWithInertia);
                    Verify.AreEqual(visualInteractionSource.PositionYSourceMode, InteractionSourceMode.Disabled);
                    Verify.AreEqual(visualInteractionSource.ScaleSourceMode, InteractionSourceMode.EnabledWithInertia);
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionXSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.PositionYSourceMode, InteractionSourceRedirectionMode.Enabled);
                        Verify.AreEqual(visualInteractionSource.PointerWheelConfig.ScaleSourceMode, InteractionSourceRedirectionMode.Enabled);
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Decreases the Scroller.MaxZoomFactor property and verifies the Scroller.ZoomFactor value decreases accordingly. Verifies the impact on the Scroller.Content Visual.")]
        public void PinchContentThroughMaxZoomFactor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX Scroller tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const double newMaxZoomFactor = 0.5;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            Visual visualScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerContent, scrollerLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMaxZoomFactor < c_defaultZoomFactor);
                Verify.IsTrue(newMaxZoomFactor < c_defaultMaxZoomFactor);
                Verify.IsTrue(newMaxZoomFactor > c_defaultMinZoomFactor);
                scroller.MaxZoomFactor = newMaxZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollerContent, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollerContent, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollerContent = ElementCompositionPreview.GetElementVisual(rectangleScrollerContent);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);


            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollerContent);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollerContent);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Cancelling spying");
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.MinZoomFactor, c_defaultMinZoomFactor);
                Verify.AreEqual(scroller.ZoomFactor, newMaxZoomFactor);
                Verify.AreEqual(scroller.MaxZoomFactor, newMaxZoomFactor);
            });

            Log.Comment("Validating final transform of Scroller.Content's Visual after MaxZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;
            
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollerContent, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(translation.X, c_defaultHorizontalOffset);
                    Verify.AreEqual(translation.Y, c_defaultVerticalOffset);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollerContent, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(scale.X, newMaxZoomFactor);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultHorizontalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultVerticalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(zoomFactor, newMaxZoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Increases the Scroller.MinZoomFactor property and verifies the Scroller.ZoomFactor value increases accordingly. Verifies the impact on the Scroller.Content Visual.")]
        public void StretchContentThroughMinZoomFactor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                //BUGBUG Bug 19277312: MUX Scroller tests fail on RS5_Release
                return;
            }

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                // Skipping this test on pre-RS2 since it uses Visual's Translation property.
                return;
            }

            const double newMinZoomFactor = 2.0;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            Visual visualScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Compositor compositor = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerContent, scrollerLoadedEvent);
                compositor = Window.Current.Compositor;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(newMinZoomFactor > c_defaultZoomFactor);
                Verify.IsTrue(newMinZoomFactor > c_defaultMinZoomFactor);
                Verify.IsTrue(newMinZoomFactor < c_defaultMaxZoomFactor);
                scroller.MinZoomFactor = newMinZoomFactor;
            });

            IdleSynchronizer.Wait();

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy on translation and scale facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(rectangleScrollerContent, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(rectangleScrollerContent, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting up spy property set");
                    visualScrollerContent = ElementCompositionPreview.GetElementVisual(rectangleScrollerContent);

                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);
            Log.Comment("Cancelling spying");

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(rectangleScrollerContent);
                    CompositionPropertySpy.StopSpyingScaleFacade(rectangleScrollerContent);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.MinZoomFactor, newMinZoomFactor);
                Verify.AreEqual(scroller.ZoomFactor, newMinZoomFactor);
                Verify.AreEqual(scroller.MaxZoomFactor, c_defaultMaxZoomFactor);
            });
            
            Log.Comment("Validating final transform of Scroller.Content's Visual after MinZoomFactor change");
            CompositionGetValueStatus status;
            float offset;
            float zoomFactor;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation;
                    status = CompositionPropertySpy.TryGetTranslationFacade(rectangleScrollerContent, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    Verify.AreEqual(translation.X, c_defaultHorizontalOffset);
                    Verify.AreEqual(translation.Y, c_defaultVerticalOffset);

                    Vector3 scale;
                    status = CompositionPropertySpy.TryGetScaleFacade(rectangleScrollerContent, out scale);
                    Log.Comment("status={0}, vertical offset={1}", status, scale.X);
                    Verify.AreEqual(scale.X, newMinZoomFactor);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, horizontal offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultHorizontalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName, out offset);
                    Log.Comment("status={0}, vertical offset={1}", status, offset);
                    Verify.AreEqual(offset, c_defaultVerticalOffset);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualScaleTargetedPropertyName, out zoomFactor);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                    Verify.AreEqual(zoomFactor, newMinZoomFactor);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Reads the properties exposed by the Scroller.ExpressionAnimationSources CompositionPropertySet.")]
        public void ReadExpressionAnimationSources()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up spy property set");
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingVector2Property(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, Vector2.Zero);
                CompositionPropertySpy.StartSpyingScalarProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, 1.0f);
            });

            Log.Comment("Jumping to absolute zoomFactor");
            ZoomTo(scroller, 2.0f, 100.0f, 200.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);            

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Cancelling spying");
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName);
                CompositionPropertySpy.StopSpyingProperty(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName);
            });

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Validating final property values of Scroller.ExpressionAnimationSources");
                CompositionGetValueStatus status;
                float zoomFactor;
                Vector2 offset;
                Vector2 position;
                Vector2 minPosition;
                Vector2 maxPosition;
                Vector2 extent;
                Vector2 viewport;

                Log.Comment("Validating final zoomFactor");
                status = CompositionPropertySpy.TryGetScalar(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesZoomFactorPropertyName, out zoomFactor);
                Log.Comment("status={0}, zoomFactor={1}", status, zoomFactor);
                Verify.AreEqual(zoomFactor, 2.0f);

                Log.Comment("Validating final offset");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesOffsetPropertyName, out offset);
                Log.Comment("status={0}, offset={1}", status, offset);
                Verify.AreEqual(offset.X, 0.0f);
                Verify.AreEqual(offset.Y, 0.0f);

                Log.Comment("Validating final position");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesPositionPropertyName, out position);
                Log.Comment("status={0}, position={1}", status, position);
                Verify.AreEqual(position.X, 100.0f);
                Verify.AreEqual(position.Y, 200.0f);

                Log.Comment("Validating final minPosition");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMinPositionPropertyName, out minPosition);
                Log.Comment("status={0}, minPosition={1}", status, minPosition);
                Verify.AreEqual(minPosition.X, 0.0f);
                Verify.AreEqual(minPosition.Y, 0.0f);

                Log.Comment("Validating final maxPosition");
                status = CompositionPropertySpy.TryGetVector2(scroller.ExpressionAnimationSources, c_expressionAnimationSourcesMaxPositionPropertyName, out maxPosition);
                Log.Comment("status={0}, maxPosition={1}", status, maxPosition);
                Verify.AreEqual(maxPosition.X, 2100.0f);
                Verify.AreEqual(maxPosition.Y, 1000.0f);

                Log.Comment("Validating final extent");
                status = scroller.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesExtentPropertyName, out extent);
                Log.Comment("status={0}, extent={1}", status, extent);
                Verify.AreEqual(extent.X, c_defaultUIScrollerContentWidth);
                Verify.AreEqual(extent.Y, c_defaultUIScrollerContentHeight);

                Log.Comment("Validating final viewport");
                status = scroller.ExpressionAnimationSources.TryGetVector2(c_expressionAnimationSourcesViewportPropertyName, out viewport);
                Log.Comment("status={0}, viewport={1}", status, viewport);
                Verify.AreEqual(viewport.X, c_defaultUIScrollerWidth);
                Verify.AreEqual(viewport.Y, c_defaultUIScrollerHeight);
            });
        }

        [TestMethod]
        public void ValidateXYFocusNavigation()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Skipped ValidateXYFocusNavigation because XYFocus support for third party scrollers is only available in RS4+.");
                return;
            }

            Button innerCenterButton = null;

            RunOnUIThread.Execute(() =>
            {
                var rootPanel = (Grid)XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                    @"<Grid Width='600' Height='600' 
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controlsPrimitives='using:Microsoft.UI.Xaml.Controls.Primitives'>
                        <Button Content='Outer Left Button' HorizontalAlignment='Left' VerticalAlignment='Center' />
                        <Button Content='Outer Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' />
                        <Button Content='Outer Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' />
                        <Button Content='Outer Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' />

                        <controlsPrimitives:Scroller x:Name='scroller' Width='200' Height='200' HorizontalAlignment='Center' VerticalAlignment='Center'>
                            <Grid Width='600' Height='600' Background='Gray'>
                                
                                <!-- Inner buttons are larger than the outer so that they get ranked better by the XY focus algorithm. -->
                                <Button Content='Inner Left Button' HorizontalAlignment='Left' VerticalAlignment='Center'  Width='180' Height='180' />
                                <Button Content='Inner Top Button' HorizontalAlignment='Center' VerticalAlignment='Top' Width='180' Height='180' />
                                <Button Content='Inner Right Button' HorizontalAlignment='Right' VerticalAlignment='Center' Width='180' Height='180' />
                                <Button Content='Inner Bottom Button' HorizontalAlignment='Center' VerticalAlignment='Bottom' Width='180' Height='180' />

                                <Button x:Name='innerCenterButton' Content='Inner Center Button' HorizontalAlignment='Center' VerticalAlignment='Center' />
                            </Grid>
                        </controlsPrimitives:Scroller>
                    </Grid>"));

                innerCenterButton = (Button)rootPanel.FindName("innerCenterButton");
                MUXControlsTestApp.App.TestContentRoot = rootPanel;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Ensure inner center button has keyboard focus.");
                innerCenterButton.Focus(FocusState.Keyboard);
                Verify.AreEqual(innerCenterButton, FocusManager.GetFocusedElement());

                Verify.AreEqual("Inner Right Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Right)).Content);
                Verify.AreEqual("Inner Left Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Left)).Content);
                Verify.AreEqual("Inner Top Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Up)).Content);
                Verify.AreEqual("Inner Bottom Button", ((Button)FocusManager.FindNextFocusableElement(FocusNavigationDirection.Down)).Content);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Listens to the Scroller.Content.EffectiveViewportChanged event and expects it to be raised while changing offsets.")]
        public void ListenToContentEffectiveViewportChanged()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // Skipping this test on pre-RS5 since it uses RS5's FrameworkElement.EffectiveViewportChanged event.
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            int effectiveViewportChangedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);

                rectangleScrollerContent.EffectiveViewportChanged += (FrameworkElement sender, EffectiveViewportChangedEventArgs args) =>
                {
                    Log.Comment("Scroller.Content.EffectiveViewportChanged: BringIntoViewDistance=" +
                        args.BringIntoViewDistanceX + "," + args.BringIntoViewDistanceY + ", EffectiveViewport=" +
                        args.EffectiveViewport.ToString() + ", MaxViewport=" + args.MaxViewport.ToString());
                    effectiveViewportChangedCount++;
                };
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            ScrollTo(
                scroller,
                horizontalOffset: 250,
                verticalOffset: 150,
                animationMode: AnimationMode.Enabled,
                snapPointsMode: SnapPointsMode.Ignore,
                hookViewChanged: false);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Expect multiple EffectiveViewportChanged occurrences during the animated offsets change.");
                Verify.IsGreaterThanOrEqual(effectiveViewportChangedCount, 1);
            });
        }

        private void SetupDefaultUI(
            Scroller scroller,
            Rectangle rectangleScrollerContent,
            AutoResetEvent scrollerLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default UI with Scroller" + (rectangleScrollerContent == null ? "" : " and Rectangle"));

            if (rectangleScrollerContent != null)
            {
                LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

                GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
                twoColorLGB.GradientStops.Add(brownGS);

                GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
                twoColorLGB.GradientStops.Add(orangeGS);

                rectangleScrollerContent.Width = c_defaultUIScrollerContentWidth;
                rectangleScrollerContent.Height = c_defaultUIScrollerContentHeight;
                rectangleScrollerContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scroller);
            scroller.Width = c_defaultUIScrollerWidth;
            scroller.Height = c_defaultUIScrollerHeight;
            if (rectangleScrollerContent != null)
            {
                scroller.Content = rectangleScrollerContent;
            }

            if (scrollerLoadedEvent != null)
            {
                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scroller;
            }
        }

        private void SpyTranslationAndScale(Scroller scroller, Compositor compositor, out float horizontalOffset, out float verticalOffset, out float zoomFactor)
        {
            Verify.IsTrue(PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2));

            float horizontalOffsetTmp = 0.0f;
            float verticalOffsetTmp = 0.0f;
            float zoomFactorTmp = 1.0f;

            horizontalOffset = verticalOffset = 0.0f;
            zoomFactor = 0.0f;
            
            Visual visualScrollerContent = null;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scroller);
                    Verify.IsNotNull(scroller.Content);

                    Log.Comment("Setting up spying on facades");

                    CompositionPropertySpy.StartSpyingTranslationFacade(scroller.Content, compositor, Vector3.Zero);
                    CompositionPropertySpy.StartSpyingScaleFacade(scroller.Content, compositor, Vector3.One);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(scroller);
                    Verify.IsNotNull(scroller.Content);

                    Log.Comment("Setting up spy property set");
                    visualScrollerContent = ElementCompositionPreview.GetElementVisual(scroller.Content);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StartSpyingScalarProperty(visualScrollerContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for spied properties to be captured");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Cancelling spying");
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingTranslationFacade(scroller.Content);
                    CompositionPropertySpy.StopSpyingScaleFacade(scroller.Content);
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName);
                    CompositionPropertySpy.StopSpyingProperty(visualScrollerContent, c_visualScaleTargetedPropertyName);
                });
            }

            Log.Comment("Waiting for captured properties to be updated");
            CompositionPropertySpy.SynchronouslyTickUIThread(10);

            Log.Comment("Reading Scroller.Content's Visual Transform");
            CompositionGetValueStatus status;

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                RunOnUIThread.Execute(() =>
                {
                    Vector3 translation = Vector3.Zero;
                    status = CompositionPropertySpy.TryGetTranslationFacade(scroller.Content, out translation);
                    Log.Comment("status={0}, horizontal offset={1}", status, translation.X);
                    Log.Comment("status={0}, vertical offset={1}", status, translation.Y);
                    horizontalOffsetTmp = translation.X;
                    verticalOffsetTmp = translation.Y;

                    Vector3 scale = Vector3.One;
                    status = CompositionPropertySpy.TryGetScaleFacade(scroller.Content, out scale);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                    zoomFactorTmp = scale.X;
                });
            }
            else
            {
                RunOnUIThread.Execute(() =>
                {
                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualHorizontalOffsetTargetedPropertyName, out horizontalOffsetTmp);
                    Log.Comment("status={0}, horizontal offset={1}", status, horizontalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualVerticalOffsetTargetedPropertyName, out verticalOffsetTmp);
                    Log.Comment("status={0}, vertical offset={1}", status, verticalOffsetTmp);

                    status = CompositionPropertySpy.TryGetScalar(visualScrollerContent, c_visualScaleTargetedPropertyName, out zoomFactorTmp);
                    Log.Comment("status={0}, zoomFactor={1}", status, zoomFactorTmp);
                });
            }

            horizontalOffset = horizontalOffsetTmp;
            verticalOffset = verticalOffsetTmp;
            zoomFactor = zoomFactorTmp;
        }
    }
}
