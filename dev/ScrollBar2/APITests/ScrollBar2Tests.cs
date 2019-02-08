﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ScrollBar2 = Microsoft.UI.Xaml.Controls.ScrollBar2;
using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class ScrollBar2Tests
    {
        private const ScrollingIndicatorMode c_defaultIndicatorMode = ScrollingIndicatorMode.None;
        private const Orientation c_defaultOrientation = Orientation.Vertical;
        private const double c_defaultOffset = 0.0;
        private const double c_defaultMinOffset = 0.0;
        private const double c_defaultMaxOffset = 100.0;
        private const double c_defaultViewport = 10.0;
        private const bool c_defaultIsEnabled = true;

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollBar2 default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollBar2 scrollBar2 = new ScrollBar2();
                Verify.IsNotNull(scrollBar2);

                Log.Comment("Verifying ScrollBar2 default property values");
                Verify.AreEqual(scrollBar2.IndicatorMode, c_defaultIndicatorMode);
                Verify.AreEqual(scrollBar2.Orientation, c_defaultOrientation);
                Verify.AreEqual(scrollBar2.IsEnabled, c_defaultIsEnabled);
                Verify.AreEqual(scrollBar2.MinOffset, c_defaultMinOffset);
                Verify.AreEqual(scrollBar2.MaxOffset, c_defaultMaxOffset);
                Verify.AreEqual(scrollBar2.Offset, c_defaultOffset);
                Verify.AreEqual(scrollBar2.Viewport, c_defaultViewport);
                Verify.IsNull(scrollBar2.ScrollBarStyle);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Exercises the ScrollBar2 property setters and getters for non-default values.")]
        public void VerifyPropertyGettersAndSetters()
        {
            ScrollBar2 scrollBar2 = null;

            RunOnUIThread.Execute(() =>
            {
                scrollBar2 = new ScrollBar2();
                Verify.IsNotNull(scrollBar2);

                Log.Comment("Setting ScrollBar2 properties to non-default values");
                scrollBar2.IndicatorMode = ScrollingIndicatorMode.TouchIndicator;
                scrollBar2.Orientation = Orientation.Horizontal;
                scrollBar2.IsEnabled = !c_defaultIsEnabled;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying ScrollBar2 non-default property values");
                Verify.AreEqual(scrollBar2.IndicatorMode, ScrollingIndicatorMode.TouchIndicator);
                Verify.AreEqual(scrollBar2.Orientation, Orientation.Horizontal);
                Verify.AreEqual(scrollBar2.IsEnabled, !c_defaultIsEnabled);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Exercises ScrollBar2's IScrollController implementation.")]
        public void VerifyScrollControllerImplementation()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollBar2 scrollBar2 = new ScrollBar2();
                Verify.IsNotNull(scrollBar2);

                IScrollController scrollBar2AsIScrollController = scrollBar2 as IScrollController;
                Verify.IsNotNull(scrollBar2AsIScrollController);

                Log.Comment("Verifying ScrollBar2's IScrollController default property values");
                Verify.IsFalse(scrollBar2AsIScrollController.AreInteractionsAllowed);
                Verify.IsTrue(scrollBar2AsIScrollController.AreScrollerInteractionsAllowed);
                Verify.IsNull(scrollBar2AsIScrollController.InteractionVisual);
                Verify.AreEqual(scrollBar2AsIScrollController.InteractionVisualScrollOrientation, c_defaultOrientation);
                Verify.IsTrue(scrollBar2AsIScrollController.IsInteractionVisualRailEnabled);
                Verify.IsFalse(scrollBar2AsIScrollController.IsInteracting);

                Log.Comment("Invoking ScrollBar2's IScrollController.SetScrollMode method to enable interactions");
                scrollBar2AsIScrollController.SetScrollMode(ScrollMode.Enabled);
                Verify.IsTrue(scrollBar2AsIScrollController.AreInteractionsAllowed);

                Log.Comment("Invoking ScrollBar2's IScrollController.SetValues method to disable interactions");
                scrollBar2AsIScrollController.SetValues(minOffset: 0.0, maxOffset: 0.0, offset: 0.0, viewport: 0.0);
                Verify.IsFalse(scrollBar2AsIScrollController.AreInteractionsAllowed);

                Log.Comment("Invoking ScrollBar2's IScrollController.SetValues method to enable interactions");
                scrollBar2AsIScrollController.SetValues(minOffset: 10.0, maxOffset: 250.0, offset: 75.0, viewport: 30.0);
                Verify.IsTrue(scrollBar2AsIScrollController.AreInteractionsAllowed);
            });
        }
    }
}
