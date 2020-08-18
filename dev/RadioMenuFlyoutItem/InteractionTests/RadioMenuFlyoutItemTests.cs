﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
﻿using System;

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RadioMenuFlyoutItemTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        List<string> Items = new List<string>
        {
            "Red",
            "Orange",
            "Yellow",
            "Green",
            "Blue",
            "Indigo",
            "Violet",
            "Compact",
            "Normal",
            "Expanded"
        };

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 19603059
        public void BasicTest()
        {
            using (var setup = new TestSetupHelper("RadioMenuFlyoutItem Tests"))
            {
                Log.Comment("Verify initial states");
                VerifySelectedItems("Orange", "Compact");

                InvokeItem("Yellow");
                VerifySelectedItems("Yellow", "Compact");

                InvokeItem("Expanded");
                VerifySelectedItems("Yellow", "Expanded");

                Log.Comment("Verify you can't uncheck an item");
                InvokeItem("Yellow");
                VerifySelectedItems("Yellow", "Expanded");
            }
        }

        public void InvokeItem(string item)
        {
            Log.Comment("Open flyout");
            Button flyoutButton = FindElement.ByName<Button>("FlyoutButton");
            flyoutButton.Invoke();
            Wait.ForIdle();

            Log.Comment("Invoke item: " + item);
            MenuItem menuItem = FindElement.ByName<MenuItem>(item + "Item");
            menuItem.Click();
            Wait.ForIdle();
        }

        public void VerifySelectedItems(string item1, string item2)
        {
            foreach (string item in Items)
            {
                TextBlock itemState = FindElement.ByName<TextBlock>(item + "State");

                if (item == item1 || item == item2)
                {
                    Verify.AreEqual(itemState.DocumentText, "Checked", "Verify " + item + " is checked");
                }
                else
                {
                    Verify.AreEqual(itemState.DocumentText, "Unchecked", "Verify " + item + " is unchecked");
                }
            }
        }
    }
}
