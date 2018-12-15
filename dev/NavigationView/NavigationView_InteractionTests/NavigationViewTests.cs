﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Text;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class NavigationViewTests
    {
        enum ControlWidth { Narrow, Medium, Wide }
        enum ControlHeight { Default, Small }
        enum Threshold { Low, High }
        enum ComboBoxName { CompactModeComboBox, ExpandedModeComboBox }
        enum TopNavPosition { Primary, Overflow }
        enum PaneOpenStatus { Opened, Closed }

        private const string minimal = "Minimal";
        private const string compact = "Compact";
        private const string expanded = "Expanded";

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        [TestProperty("MUXControlsTestEnabledForPhone", "True")]
        [TestProperty("NavViewTestSuite", "A")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void DisplayModeTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));

                    // Tests with PaneDisplayMode='Auto', which enables adaptive layout.

                    Log.Comment("Test the adaptive layout with the default Compact and Expanded mode Thresholds");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                    Log.Comment("Test adaptive layout when the compact mode threshold is larger than the expanded mode threshold");
                    SetThreshold(Threshold.High, ComboBoxName.CompactModeComboBox);
                    SetThreshold(Threshold.Low, ComboBoxName.ExpandedModeComboBox);

                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                    Log.Comment("Test adaptive layout when the compact mode threshold is equal to the expanded mode threshold");
                    SetThreshold(Threshold.Low, ComboBoxName.CompactModeComboBox);
                    SetThreshold(Threshold.Low, ComboBoxName.ExpandedModeComboBox);

                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void PaneDisplayModeLeftLeftCompactLeftMinimalTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Test"))
            {
                var displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));

                // Tests with PaneDisplayMode=Left/LeftCompact/LeftMinimal.
                // This disables all adaptive layout behavior.

                Log.Comment("Test PaneDisplayMode=Left");
                panelDisplayModeComboBox.SelectItemByName("Left");
                Wait.ForIdle();

                Log.Comment("DisplayMode should be 'Expanded' regardless of size");
                SetNavViewWidth(ControlWidth.Narrow);
                Wait.ForIdle();
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);


                Log.Comment("Test PaneDisplayMode=LeftCompact");
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                Log.Comment("DisplayMode should be 'Compact' regardless of size");
                SetNavViewWidth(ControlWidth.Narrow);
                Wait.ForIdle();
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);


                Log.Comment("Test PaneDisplayMode=LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                Wait.ForIdle();

                Log.Comment("DisplayMode should be 'Minimal' regardless of size");
                SetNavViewWidth(ControlWidth.Narrow);
                Wait.ForIdle();
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);
            }
        }

        private void SetNavViewWidth(ControlWidth width)
        {
            ComboBox widthComboBox = new ComboBox(FindElement.ByName("WidthComboBox"));
            string currentWidth = "unset";

            if (widthComboBox.Selection.Count > 0)
            {
                ComboBoxItem selectedComboBoxItem = widthComboBox.Selection[0];
                currentWidth = selectedComboBoxItem.Name;
                Log.Comment("Current width " + currentWidth);
            }
            string widthString = width.ToString();

            if (currentWidth != widthString)
            {
                Wait.ForIdle();
                Log.Comment("Changing to width " + widthString);
                widthComboBox.SelectItemByName(widthString);
            }
        }

        private void SetNavViewHeight(ControlHeight height)
        {
            ComboBox heightComboBox = new ComboBox(FindElement.ByName("HeightCombobox"));
            string currentHeight = "Default";

            if (heightComboBox.Selection.Count > 0)
            {
                ComboBoxItem selectedComboBoxItem = heightComboBox.Selection[0];
                currentHeight = selectedComboBoxItem.Name;
                Log.Comment("Current height " + currentHeight);
            }

            Log.Comment("Changing height to " + height.ToString());
            heightComboBox.SelectItemByName(height.ToString());
        }

        private void SetThreshold(Threshold threshold, ComboBoxName name)
        {
            ComboBox thresholdComboBox = new ComboBox(FindElement.ByName(name.ToString()));
            string currentThreshold = "unset";

            if (thresholdComboBox.Selection.Count > 0)
            {
                ComboBoxItem selectedComboBoxItem = thresholdComboBox.Selection[0];
                currentThreshold = selectedComboBoxItem.Name;
                Log.Comment("Current threshold " + currentThreshold);
            }
            string thresholdString = threshold.ToString();

            if (currentThreshold != thresholdString)
            {
                Log.Comment("Changing to width " + thresholdString);
                thresholdComboBox.SelectItemByName(thresholdString);
            }
        }

        private void ClickClearSelectionButton()
        {
            Log.Comment("Clear the selection by set NavView.SelectedItem to null");
            var ClearSelectedItemButton = new Button(FindElement.ByName("ClearSelectedItemButton"));
            ClearSelectedItemButton.Invoke();
            Wait.ForIdle();
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void MenuItemInvokedTest()
        {
            var testScenarios = RegressionTestScenario.BuildTopNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Log.Comment("Click games item");
                    UIObject menuItem = FindElement.ByName("Games");
                    InputHelper.LeftClick(menuItem);
                    Wait.ForIdle();
                    TextBlock header = new TextBlock(FindElement.ByName("Games as header"));
                    Verify.AreEqual("Games as header", header.DocumentText);

                    Log.Comment("Click music item");
                    menuItem = FindElement.ByName("Music");
                    InputHelper.LeftClick(menuItem);
                    Wait.ForIdle();
                    header = new TextBlock(FindElement.ByName("Music as header"));
                    Verify.AreEqual("Music as header", header.DocumentText);

                    Log.Comment("Click settings item");
                    menuItem = testScenario.IsLeftNavTest ? FindElement.ByName("Settings") : FindElement.ByName("SettingsTopNavPaneItem");
                    InputHelper.LeftClick(menuItem);
                    Wait.ForIdle();
                    header = new TextBlock(FindElement.ByName("Settings as header"));
                    Verify.AreEqual("Settings as header", header.DocumentText);

                    Log.Comment("Move mouse to upper left to ensure that tooltip on settings closes.");
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void PaneOpenCloseTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    // On phone, the pane will initially be in the closed compact state, so open it before
                    // proceeding with the test.
                    if (isPaneOpenCheckBox.ToggleState == ToggleState.Off)
                    {
                        using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                        {
                            isPaneOpenCheckBox.Toggle();
                            waiter.Wait();
                        }
                    }

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                    Log.Comment("Verify that clicking the navigation button closes the nav pane");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                    Log.Comment("Verify that clicking the navigation button opens the nav pane");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    Log.Comment("Verify that decreasing the width of the control from expanded to compact closes the pane");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                    Log.Comment("Verify that increasing the width of the control from compact to expanded opens the pane");
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);
                }
            }
        }

        [TestMethod] // Bug 18159731
        [TestProperty("NavViewTestSuite", "A")]
        public void PaneOpenForceCloseTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    //mobile test is tracked by Task 16603328.
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Warning("Test is disabled on phone.");
                        return;
                    }

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                    Log.Comment("Verify that clicking the navigation button closes the nav pane");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));

                    Log.Comment("Verify that decreasing the width of the control from expanded to Narrow and force closed pane");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    Log.Comment("Verify that decreasing the width of the control from Narrow to compact and force closed pane");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                    Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                    Log.Comment("Verify that increasing the width of the control from compact to expanded and force closed pane");
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void PaneOpenCloseTestPartTwo() // Otherwise this test will exceed the 30 second timeout in catgates chk runs
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    // On phone, the pane will initially be in the closed compact state, so open it before
                    // proceeding with the test.
                    if (isPaneOpenCheckBox.ToggleState == ToggleState.Off)
                    {
                        using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                        {
                            isPaneOpenCheckBox.Toggle();
                            waiter.Wait();
                        }
                    }

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                    Log.Comment("Verify that after explicitly closing the nav pane, changing display mode doesn't reopen it");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                    Log.Comment("Verify that selecting a menu item in minimal display mode closes the pane");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    Log.Comment("Invoke Music item to close the pane");
                    var music = new Button(FindElement.ByName("Music"));
                    music.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void IsSettingsVisibleTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    String settings = testScenario.IsLeftNavTest ? "Settings" : "SettingsTopNavPaneItem";
                    Log.Comment("Verify that settings item is enabled by default");
                    VerifyElement.Found(settings, FindBy.Name);

                    CheckBox settingsCheckbox = new CheckBox(FindElement.ByName("SettingsItemVisibilityCheckbox"));

                    Log.Comment("Verify that settings item is not visible when IsSettingsVisible == false");
                    settingsCheckbox.Uncheck();
                    ElementCache.Clear();
                    Wait.ForIdle();
                    VerifyElement.NotFound(settings, FindBy.Name);

                    Log.Comment("Verify that settings item is visible when IsSettingsVisible == true");
                    settingsCheckbox.Check();
                    Wait.ForIdle();
                    VerifyElement.Found(settings, FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void IsPaneToggleButtonVisibleTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Log.Comment("Verify that toggle button item is enabled by default");
                    VerifyElement.Found("TogglePaneButton", FindBy.Id);

                    CheckBox toggleCheckbox = new CheckBox(FindElement.ByName("PaneToggleButtonVisiblityCheckbox"));

                    Log.Comment("Verify that toggle button is not visible when IsPaneToggleButtonVisible == false");
                    toggleCheckbox.Uncheck();
                    Wait.ForIdle();
                    VerifyElement.NotFound("TogglePaneButton", FindBy.Id);

                    Log.Comment("Verify that settings item is visible when IsSettingsVisible == true");
                    toggleCheckbox.Check();
                    Wait.ForIdle();
                    VerifyElement.Found("SettingsNavPaneItem", FindBy.Id);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void AlwaysShowHeaderTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Log.Comment("Verify that header is visible by default");
                    VerifyElement.Found("Home as header", FindBy.Name);

                    CheckBox headerVisibilityCheckbox = new CheckBox(FindElement.ByName("HeaderVisiblityCheckbox"));

                    Log.Comment("Verify that header is not visible in display mode expanded when AlwaysShowHeader == false");
                    headerVisibilityCheckbox.Uncheck();
                    Wait.ForIdle();
                    VerifyElement.NotFound("Home as header", FindBy.Name);

                    Log.Comment("Verify that header is visible in display mode minimal when AlwaysShowHeader == false");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    VerifyElement.Found("Home as header", FindBy.Name);

                    Log.Comment("Verify that header is not visible in display mode compact when AlwaysShowHeader == false");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    VerifyElement.NotFound("Home as header", FindBy.Name);

                    Log.Comment("Verify that header is visible in display mode compact when AlwaysShowHeader == true");
                    headerVisibilityCheckbox.Check();
                    Wait.ForIdle();
                    VerifyElement.Found("Home as header", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void PaneFooterContentTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Test"))
            {
                Log.Comment("Verify that button in PaneFooterContent exists");
                VerifyElement.Found("FooterButton", FindBy.Id);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void AddRemoveItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    var addButton = FindElement.ById<Button>("AddItemButton");
                    var removeButton = FindElement.ById<Button>("RemoveItemButton");

                    Log.Comment("Verify that menu items can be added");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Menu Item 0", FindBy.Name);

                    Log.Comment("Verify that more menu items can be added");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Menu Item 1", FindBy.Name);

                    Log.Comment("Verify that menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("New Menu Item 1", FindBy.Name);
                    VerifyElement.Found("New Menu Item 0", FindBy.Name);

                    Log.Comment("Verify that more menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("New Menu Item 0", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void AddRemoveOriginalItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    var addButton = FindElement.ById<Button>("AddItemButton");
                    var removeButton = FindElement.ById<Button>("RemoveItemButton");

                    Log.Comment("Verify that original menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("Integer", FindBy.Name);

                    Log.Comment("Verify that menu items can be added after removing");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Menu Item 0", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void ItemSourceTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                            page2 = new TestSetupHelper("NavigationView Init Test"))
            {
                var addButton = FindElement.ByName<Button>("AddItemButton");
                var removeButton = FindElement.ByName<Button>("RemoveItemButton");

                Log.Comment("Verify that the MenuItemsSource was loaded and is selected");
                UIObject item1 = FindElement.ByName("Menu Item 1");
                Verify.IsNotNull(item1);
                Verify.IsTrue(Convert.ToBoolean(item1.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                Log.Comment("Verify that menu items added to MenuItemsSource appear in the list");
                addButton.Invoke();
                Wait.ForIdle();
                VerifyElement.Found("New Menu Item", FindBy.Name);

                Log.Comment("Verify that menu items removed from MenuItemsSource disappear from the list");
                removeButton.Invoke();
                Wait.ForIdle();
                VerifyElement.NotFound("New Menu Item", FindBy.Name);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void ForceIsPaneOpenToFalseOnLeftNavTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                            page2 = new TestSetupHelper("Navigation IsPaneOpen Test"))
            {
                Log.Comment("Verify IsPaneOpen=False would not open the pane by default");
                CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                Log.Comment("Verify that clicking the navigation button open the nav pane");
                navButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void DisabledItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    UIObject moviesItem = FindElement.ByName("Movies");
                    CheckBox moviesEnabledCheckbox = new CheckBox(FindElement.ByName("MoviesEnabledCheckbox"));

                    Log.Comment("Verify that Movies item is enabled");
                    Verify.IsTrue(moviesItem.IsEnabled);

                    Log.Comment("Uncheck checkbox to disable Movies item");
                    moviesEnabledCheckbox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that Movies item is disabled");
                    Verify.IsFalse(moviesItem.IsEnabled);

                    Log.Comment("Check checkbox to enable Movies item");
                    moviesEnabledCheckbox.Check();
                    Wait.ForIdle();

                    Log.Comment("Verify that Movies item is enabled");
                    Verify.IsTrue(moviesItem.IsEnabled);
                }
            }
        }

        [TestMethod] // bug 16644730
        [TestProperty("NavViewTestSuite", "A")]
        public void VerifySettingsWidthOnLeftNavMediumMode()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Test"))
            {
                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();

                Button navButton = new Button(FindElement.ById("SettingsNavPaneItem"));
                Log.Comment("Verify that the SettingsNavPaneItem size in Expanded mode and actual width is " + navButton.BoundingRectangle.Width);

                // NavigationViewCompactPaneLength is 40 or 48 in different release. This test case doesn't need an exactly number of width, so just choose 48 as the boundary
                Verify.IsTrue(navButton.BoundingRectangle.Width > 48);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();

                Log.Comment("Verify that the SettingsNavPaneItem size in Medium mode and actual width is " + navButton.BoundingRectangle.Width);
                Verify.IsTrue(navButton.BoundingRectangle.Width <= 48);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void AutoSuggestBoxTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();

                    Log.Comment("Verify that the AutoSuggestBox is visible and the search button is not in Expanded mode");
                    VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);
                    VerifyElement.NotFound("Click to search", FindBy.Name);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();

                    Log.Comment("Verify that the AutoSuggestBox is not visible and the search button is in Compact mode with the pane closed");
                    VerifyElement.NotFound("PaneAutoSuggestBox", FindBy.Name);
                    Button searchButton = new Button(FindElement.ByName("Click to search"));
                    Verify.IsNotNull(searchButton);

                    Log.Comment("Verify that invoking the search button opens the pane and put focus in the AutoSuggestBox");
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                    searchButton.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    // AutoSuggestBox never gets keyboard focus itself; focus goes to the edit box inside it
                    UIObject autoSuggestEditBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");
                    Verify.IsNotNull(autoSuggestEditBox);
                    Verify.IsTrue(autoSuggestEditBox.HasKeyboardFocus);

                    Log.Comment("Verify that setting AutoSuggestBox to null removes it and the search button");
                    CheckBox autoSuggestCheckBox = new CheckBox(FindElement.ByName("AutoSuggestCheckbox"));
                    autoSuggestCheckBox.Uncheck();
                    Wait.ForIdle();

                    VerifyElement.NotFound("PaneAutoSuggestBox", FindBy.Name);
                    VerifyElement.NotFound("Click to search", FindBy.Name);

                    Log.Comment("Verify that setting AutoSuggestBox puts it back");
                    autoSuggestCheckBox.Check();
                    Wait.ForIdle();

                    VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void AutoSuggestBoxOnTopNavTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView TopNav Test"))
            {
                ElementCache.Refresh();
                ElementCache.Dump();
                Log.Comment("Verify that the AutoSuggestBox is visible by default");
                VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);

                Log.Comment("Verify that setting AutoSuggestBox to null removes it and the search button");
                CheckBox autoSuggestCheckBox = new CheckBox(FindElement.ByName("AutoSuggestCheckbox"));
                autoSuggestCheckBox.Uncheck();
                Wait.ForIdle();

                VerifyElement.NotFound("PaneAutoSuggestBox", FindBy.Name);

                Log.Comment("Verify that setting AutoSuggestBox puts it back");
                autoSuggestCheckBox.Check();
                Wait.ForIdle();

                VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "A")]
        public void VerifyFocusNotLostWhenTabbingWithBackButtonEnabled()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView TopNav Test"))
            {
                CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));
                Log.Comment("Checking Back Enabled");

                checkBox.Check();
                Wait.ForIdle();

                // Pick an item close to the end of the content and set focus on it.
                CheckBox cancelClosingCheckbox = new CheckBox(FindElement.ById("CancelClosingEvents"));
                cancelClosingCheckbox.SetFocus();
                Wait.ForIdle();

                // Tab a number of times so that we get past the last item
                // in the content.
                for (int i = 0; i < 10; i++)
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                }

                // Verify that we do not lose focus.
                AutomationElement focusedElement = AutomationElement.FocusedElement;
                Verify.IsNotNull(focusedElement);
            }
        }

        [TestMethod] //bug 17792706
        [TestProperty("NavViewTestSuite", "A")]
        public void BackButtonPlaceHolderOnTopNavTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                var getActiveVisualStateButton = new Button(FindElement.ByName("GetActiveVisualState"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }

                Verify.IsTrue(invokeResult.Value.Contains("BackButtonVisible"));

                Log.Comment("Hide backbutton");
                var backButtonCheckBox = new CheckBox(FindElement.ByName("BackButtonVisibilityCheckbox"));

                backButtonCheckBox.Uncheck();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }

                Verify.IsTrue(invokeResult.Value.Contains("BackButtonCollapsed"));

                Log.Comment("Show backbutton");
                backButtonCheckBox.Check();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("BackButtonVisible"));
            }
        }

        //[TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        // Disabled due to: Bug 18650478: Test instability: NavigationViewTests.TitleBarTest
        public void TitleBarTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Comment("Test does not run on phone because there is no title bar.");
                        return;
                    }

                    CheckBox titleBarCheckbox = new CheckBox(FindElement.ByName("TitleBarCheckbox"));
                    CheckBox testFrameCheckbox = new CheckBox(FindElement.ByName("TestFrameCheckbox"));
                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                    Button backButton = new Button(FindElement.ByName("NavigationViewBackButton"));
                    Button fullScreenButton = new Button(FindElement.ById("FullScreenInvokerButton"));

                    int testFrameBarHeight = FindElement.ById("__BackButton").BoundingRectangle.Height;

                    // The title bar is a peer of the CoreWindow, so we have to search one level up.
                    UIObject titleBar;
                    TestEnvironment.Application.CoreWindow.Parent.Descendants.TryFind(UICondition.CreateFromClassName("ApplicationFrameTitleBarWindow"), out titleBar);
                    int titleBarHeight = titleBar.BoundingRectangle.Height;

                    double backButtonSpace = backButton.BoundingRectangle.Height;
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        backButtonSpace += 4; // back button is bigger on older OSes
                    }

                    int tabbedShellAffordance = 0;
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        if (TestHelpers.SystemTabbedShellIsEnabled)
                        {
                            titleBarHeight = 0;
                            tabbedShellAffordance = testFrameBarHeight;
                            Log.Comment("Tabbed shell is enabled, adjusting expectations...");
                        }
                    }

                    Log.Comment("Test scenario: Standard NavView control.");
                    Log.Comment("Verify that the toggle button y = height of title bar + height of test frame bar + spacing of back button");
                    Verify.AreEqual(testFrameBarHeight + titleBarHeight + backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);

                    Log.Comment("Test scenario: ExtendViewIntoTitleBar:");
                    titleBarCheckbox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that the toggle button y = height of test frame bar + back button spacing");

                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => (int)(testFrameBarHeight + backButtonSpace + tabbedShellAffordance),
                        () => (int)(navButton.BoundingRectangle.Y));


                    Log.Comment("Test scenario: Hide TestFrame:");
                    titleBarCheckbox.Check();
                    testFrameCheckbox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that the toggle button y = height of title bar + back button spacing");
                    Verify.AreEqual(titleBarHeight + backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);


                    Log.Comment("Test scenario: Hide TestFrame and ExtendViewIntoTitleBar:");
                    titleBarCheckbox.Uncheck();
                    Wait.ForIdle();

                    if(!testScenario.IsUsingRS4Style)
                    {
                        // If we extend the backbutton to titlebar area, the button is not clickable. so the new implementation keeps backbutton not in titlebar area.
                        Log.Comment("Verify that the toggle button y = height of title bar + back button spacing");
                        Verify.AreEqual(titleBarHeight + backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);
                    }
                    else
                    {
                        // To maintain back compat we maintain RS4 behavior when using the RS4 style:
                        Log.Comment("Verify that the toggle button y = back button spacing");
                        Verify.AreEqual(backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);
                    }

                    Log.Comment("Test scenario: Fullscreen mode:");
                    Log.Comment("Invoking fullscreen button:");
                    fullScreenButton.Invoke();
                    Wait.ForIdle();

                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => 0,
                        () => backButton.BoundingRectangle.Y);
                    Log.Comment("Verify that the toggle button y = back button spacing");
                    Verify.AreEqual(backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);


                    fullScreenButton.Invoke();
                    Wait.ForIdle();
                    titleBarCheckbox.Check();
                    testFrameCheckbox.Check();
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void VerifyBackButtonHidesWhenInMinimalOpenState()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));

                    Log.Comment("Test the adaptive layout with the default Compact and Expanded mode Thresholds");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    isPaneOpenCheckBox.Check();
                    Wait.ForIdle();

                    VerifyElement.NotFound("NavigationViewBackButton", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void ArrowKeyNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    SetNavViewWidth(ControlWidth.Wide);

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    // Grab references to all the menu items in the test UI
                    UIObject searchBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");
                    UIObject item1 = FindElement.ByName("Home");
                    UIObject item2 = FindElement.ByName("Apps");
                    UIObject item3 = FindElement.ByName("Games");
                    UIObject item4 = FindElement.ByName("Music");
                    UIObject item5 = FindElement.ByName("Movies");
                    UIObject item6 = FindElement.ByName("TV");
                    UIObject settingsItem = FindElement.ByName("Settings");

                    Log.Comment("Verify that tab from the TogglePaneButton goes to the search box");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that tab from search box goes to the first item");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(item1.HasKeyboardFocus);

                    Log.Comment("Verify that down arrow can navigate through all items");
                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item2.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item3.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item4.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item5.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item6.HasKeyboardFocus);

                    Log.Comment("Verify that tab twice from the last menu item goes to the settings item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 2);
                    Wait.ForIdle();
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    Log.Comment("Verify that shift+tab twice from the settings item goes to the last menu item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 2);
                    Wait.ForIdle();
                    Verify.IsTrue(item6.HasKeyboardFocus);

                    Log.Comment("Verify that up arrow can navigate through all items");
                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item5.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item4.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item3.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item2.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item1.HasKeyboardFocus);

                    Log.Comment("Verify that shift+tab from the first menu item goes to the search box");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that shift+tab from the search box goes to the TogglePaneButton");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TabNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    SetNavViewWidth(ControlWidth.Wide);

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    UIObject searchBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject settingsItem = FindElement.ByName("Settings");
                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to the search box");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that pressing tab while the search box has focus moves to the first menu item");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify that pressing tab twice more will move focus to the settings item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 2);
                    Wait.ForIdle();
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    Log.Comment("Verify that pressing SHIFT+tab twice will move focus to the first menu item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 2);
                    Wait.ForIdle();
                    Verify.IsTrue(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify that pressing SHIFT+tab will move focus to the search box");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that pressing SHIFT+tab will move focus to the TogglePaneButton");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                    Wait.ForIdle();
                    Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void LeftNavigationFocusKindRevealTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Test"))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Warning("Test is disabled because FocusVisualKind.Reveal requires rs4+");
                    return;
                }

                Log.Comment("Invoke button GetHomeItemRevealVisualState");
                Button getHomeItemRevealVisualState = new Button(FindElement.ByName("GetHomeItemRevealVisualState"));
                getHomeItemRevealVisualState.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify No OnLeftNavigationReveal visualstate");
                var state = TryFindElement.ById("OnLeftNavigationRevealVisualState");
                Verify.AreEqual("False", state.GetText());

                Log.Comment("Change FocusVisualKind to Reveal");
                Button changeFocusVisualKind = new Button(FindElement.ByName("ChangeFocusVisualKind"));
                changeFocusVisualKind.Invoke();
                Wait.ForIdle();

                Log.Comment("Invoke button GetHomeItemRevealVisualState");
                getHomeItemRevealVisualState.Invoke();
                Wait.ForIdle();
                // changing FocusVisualKind impacts others test cases, and we need to change it back immediately.
                // So recover it to default before verify state1 visualstate
                var state1 = state.GetText();

                Log.Comment("Change FocusVisualKind to default");
                changeFocusVisualKind.Invoke();
                Wait.ForIdle();

                Log.Comment("Invoke button GetHomeItemRevealVisualState");
                getHomeItemRevealVisualState.Invoke();
                Wait.ForIdle();
                var state2 = state.GetText();

                Verify.AreEqual("True", state1, "There is OnLeftNavigationReveal visualstate");
                Verify.AreEqual("False", state2, "No OnLeftNavigationReveal visualstate");
            }
        }

        private bool IsItemInTopNavPrimaryList(string text)
        {
            var list = TryFindElement.ById("TopNavMenuItemsHost");
            Verify.IsTrue(list != null, "TopNavMenuItemsHost exists");
            foreach (var item in list.Children)
            {
                if (item != null)
                {
                    foreach (var v in new List<string> { item.AutomationId, item.ClassName, item.Name })
                    {
                        if (v != null && v.Contains(text))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        // To verify two problems:
        // 1. NavigationViewItem not in overflow menu
        //      Layout doesn't know about overflow, so changing the content of NavigationViewItem may not trigger MeasureOverride
        //      Verify NavigationView will handle this
        // 2. NavigationViewItem in overflow menu
        //      We cached the Width when moving items to overflow, and cached width would be used to recover items to primary
        //      This test case verifies that cache is invalidated if content is changed for NavigationViewItem
        public void VerifyNavigationViewContentChangeOnTopNavImpactsLayout()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                Log.Comment("Verify there is overflow button");
                VerifyElement.Found("TopNavOverflowButton", FindBy.Id);

                Verify.IsTrue(IsItemInTopNavPrimaryList("Games"), "Games not in overflow");

                Log.Comment("Change Games Content to a long string");
                Button changeGamesContent = new Button(FindElement.ByName("ChangeGamesContent"));
                changeGamesContent.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify Content change in primary may push Games item to overflow");
                Verify.IsFalse(IsItemInTopNavPrimaryList("Games"), "Games is moved to overflow");

                Log.Comment("Clear Content of all NavigationViewItems");
                Button clearNavItemContent = new Button(FindElement.ByName("ClearNavItemContent"));
                clearNavItemContent.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify all items are moved out of overflow menu");
                VerifyElement.NotFound("TopNavOverflowButton", FindBy.Id);

                Log.Comment("Change Games Content to a long string");
                changeGamesContent.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify change content of Game makes it to overflow");
                Verify.IsFalse(IsItemInTopNavPrimaryList("Games"), "Games is moved to overflow");

                Log.Comment("Clear Content of Games which is in overflow");
                clearNavItemContent.Invoke();
                Wait.ForIdle();

                // If NavigationViewItem is in overflow and the popup is not opened, we can't get NavigationView by the visualtree from NavigationViewItem itself
                // Change the content of NavigationViewItem in overflow will not Invalidate Layout, and request user to invalid measure.
                Log.Comment("Invalid Measure");
                Button invalidateMeasure = new Button(FindElement.ByName("NavInvalidateMeasure"));
                invalidateMeasure.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify cached width is invalidated and InvalidateMeasure will move all items out of overflow");
                VerifyElement.NotFound("TopNavOverflowButton", FindBy.Id);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TopNavigationOverflowWidthLongNavItemTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                var longNavItemPartialContent = "Gates";
                var primaryCount = GetTopNavigationItems(TopNavPosition.Primary).Count;

                Log.Comment("Add a long navigationview item which text includes " + longNavItemPartialContent);
                Button button = new Button(FindElement.ByName("AddLongNavItem"));
                button.Invoke();
                Wait.ForIdle();

                var count = GetTopNavigationItems(TopNavPosition.Primary).Count;
                Verify.AreEqual(primaryCount, count, "The appended nav item goes to overflow");

                // Select the longest nav item
                OpenOverflowMenuAndInvokeItem(longNavItemPartialContent);
                
                count = GetTopNavigationItems(TopNavPosition.Primary).Count;
                Verify.IsTrue(primaryCount - count >= 2, "Longest nav item make more than 1 items to overflow " + primaryCount + " vs " + count);

                // Select the shortest IntegerItem which content is 7
                OpenOverflowMenuAndInvokeItem("IntegerItem");
                Verify.IsTrue(
                    GetTopNavigationItems(TopNavPosition.Primary).
                        Where(item => UIObjectContains(item, longNavItemPartialContent)).
                        Count() == 0, 
                    "Longest nav item is pushed to overflow");

                count = GetTopNavigationItems(TopNavPosition.Primary).Count;
                Verify.IsTrue(primaryCount <= count, 
                    "Select the shortest item make more item to primary " + primaryCount + " vs " + count);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TopNavigationOverflowButtonTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                Log.Comment("Add ten items to make overflow happen");
                Button addTenItems = new Button(FindElement.ByName("AddTenItems"));
                addTenItems.Invoke();
                Wait.ForIdle();

                Log.Comment("Get and Check that active visualstate contains OverflowButtonWithLabel");
                var getActiveVisualStateButton = new Button(FindElement.ByName("GetActiveVisualState"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }

                Verify.IsTrue(invokeResult.Value.Contains("OverflowButtonWithLabel"));

                Log.Comment("Hide the overflow button label");
                var changeOverflowLabelVisibility = new CheckBox(FindElement.ByName("ChangeOverflowLabelVisibility"));
                changeOverflowLabelVisibility.Uncheck();
                Wait.ForIdle();

                Log.Comment("Get and Check that active visualstate contains OverflowButtonNoLabel");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }

                Verify.IsTrue(invokeResult.Value.Contains("OverflowButtonNoLabel"));
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void ContentOverlayTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                var topNavAutomationId = "TopNavMenuItemsHost";
                var contentOverlayName = "CONTENT OVERLAY";
                var addRemoveContentOverlayButton = new Button(FindElement.ById("AddRemoveContentOverlay"));
                var changeTopNavVisibilityButton = new Button(FindElement.ById("ChangeTopNavVisibility"));

                Log.Comment("Verify that after NavView loads, the top nav is visible and that there is no content overlay");
                VerifyElement.Found(topNavAutomationId, FindBy.Id);
                VerifyElement.NotFound(contentOverlayName, FindBy.Name);

                addRemoveContentOverlayButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify that after content overlay is added, the top nav is visible and the content overlay too");
                VerifyElement.Found(topNavAutomationId, FindBy.Id);
                VerifyElement.Found(contentOverlayName, FindBy.Name);

                changeTopNavVisibilityButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify that after setting IsPaneVisible to false, the top nav is hidden and the content overlay is still visible");
                VerifyElement.NotFound(topNavAutomationId, FindBy.Id);
                VerifyElement.Found(contentOverlayName, FindBy.Name);

                changeTopNavVisibilityButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify that after setting IsPaneVisible to true, the top nav is visible and the content overlay remains visible");
                VerifyElement.Found(topNavAutomationId, FindBy.Id);
                VerifyElement.Found(contentOverlayName, FindBy.Name);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TopPaddingTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Store Test"))
            {
                var moveContentUnderTitleBarButton = new Button(FindElement.ById("MoveContentUnderTopnavTitleBar"));
                var getTopPaddingHeightButton = new Button(FindElement.ById("GetTopPaddingHeightButton"));
                var fullScreenButton = new Button(FindElement.ById("FullScreenInvokerButton"));
                var topPaddingRenderedValueId = "TopPaddingRenderedValue";
                UIObject topNavTopPadding = null;

                // Checking top padding is added for regular Desktop                
                moveContentUnderTitleBarButton.Click();
                Wait.ForIdle();
                getTopPaddingHeightButton.Click();
                Wait.ForIdle();
                topNavTopPadding = TryFindElement.ById(topPaddingRenderedValueId);

                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    // For phone we only check once to make sure the padding is 0
                    Verify.AreEqual(0, Int32.Parse(topNavTopPadding.GetText()));
                    return;
                }

                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Verify.AreEqual(32, Int32.Parse(topNavTopPadding.GetText()));
                }
                else
                {
                    // To detect if it's in tabbed sets, CoreApplicationViewTitleBar.IsVisible can be used to check in MUXControlsTestApp
                    // Not all branches enabled tabbed shell, also not all default setting with tabbed shell enabled
                    // We skip the test if tabbedshell is disabled.
                    if (TryFindElement.ById("TitleBarIsVisible").GetText().Equals("True"))
                    {
                        Log.Comment("Tabbed Shell is disabled or not enabled for this application, skip Verify");
                    }
                    else
                    {
                        Verify.AreEqual(0, Int32.Parse(topNavTopPadding.GetText()));
                    }
                }

                // Checking top padding is NOT added for fullscreen Desktop
                moveContentUnderTitleBarButton.Click();
                Wait.ForIdle();
                fullScreenButton.Click();
                Wait.ForIdle();
                moveContentUnderTitleBarButton.Click();
                Wait.ForIdle();
                getTopPaddingHeightButton.Click();
                Wait.ForIdle();

                topNavTopPadding = TryFindElement.ById(topPaddingRenderedValueId);
                Verify.AreEqual(0, Int32.Parse(topNavTopPadding.GetText()));

                // Reverting changes to leave app in original state
                moveContentUnderTitleBarButton.Click();
                Wait.ForIdle();
                fullScreenButton.Click();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void SuppressSelectionItemInvokeTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Store Test"))
            {
                var removeItemButton = new Button(FindElement.ById("RemoveItemButton"));
                var addItemSuppressSelectionButton = new Button(FindElement.ById("AddItemSuppressSelectionButton"));
                var clearItemInvokedTextButton = new Button(FindElement.ById("ClearItemInvokedTextButton"));

                removeItemButton.Click();
                Wait.ForIdle();
                removeItemButton.Click();
                Wait.ForIdle();
                removeItemButton.Click();
                Wait.ForIdle();
                addItemSuppressSelectionButton.Click();
                Wait.ForIdle();

                VerifyElement.Found("sup-selection-nav-item-0", FindBy.Id);
                var supSelectItem0 = TryFindElement.ById("sup-selection-nav-item-0");
                supSelectItem0.Click();
                Wait.ForIdle();

                var itemInvokedText = TryFindElement.ById("ItemInvokedText");
                Verify.AreEqual("New Menu Item S.S", itemInvokedText.GetText());

                clearItemInvokedTextButton.Click();
                Wait.ForIdle();
                Verify.AreEqual(string.Empty, itemInvokedText.GetText());

                UIObject moreButton = null;
                Log.Comment("Adding items until the More button shows up.");

                while (moreButton == null)
                {
                    moreButton = TryFindElement.ById("TopNavOverflowButton");

                    if (moreButton == null)
                    {
                        addItemSuppressSelectionButton.Click();
                        Log.Comment("Item added.");
                        Wait.ForIdle();
                    }
                }

                moreButton.Click();
                Wait.ForIdle();

                var firstChildOverflow = GetTopNavigationItems(TopNavPosition.Overflow)[0];
                firstChildOverflow.Click();
                Wait.ForIdle();

                itemInvokedText = TryFindElement.ById("ItemInvokedText");
                Verify.AreEqual("New Menu Item S.S", itemInvokedText.GetText());
            }
        }

        [TestMethod] //bug 18033309
        [TestProperty("NavViewTestSuite", "B")]
        public void TopNavigationSecondClickOnSuppressSelectionItemTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                Button resetResultButton = new Button(FindElement.ById("ResetResult"));
                UIObject suppressSelection = FindElement.ByName("SuppressSelection");

                var invokeResult = new Edit(FindElement.ById("ItemInvokedResult"));
                var selectResult = new Edit(FindElement.ById("SelectionChangedResult"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    suppressSelection.Click();
                    waiter.Wait();
                }

                // First time selection only raise ItemInvoke
                Verify.AreEqual(invokeResult.Value, "SuppressSelection");
                Verify.AreEqual(selectResult.Value, "");

                resetResultButton.Click();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    suppressSelection.Click();
                    waiter.Wait();
                }

                // Click it again, only raise ItemInvoke event
                Verify.AreEqual(invokeResult.Value, "SuppressSelection");
                Verify.AreEqual(selectResult.Value, "");
            }
        }

        private void InvokeNavigationViewAccessKeyAndVerifyKeyTipPlacement(string expectedKeyTipTargetElementId)
        {
            string keyTipText = "H";
            Log.Comment("Send AccessKey to invoke toggle button for left nav or more button for top nav");
            KeyboardHelper.PressDownModifierKey(ModifierKey.Alt);
            KeyboardHelper.ReleaseModifierKey(ModifierKey.Alt);
            Wait.ForIdle();

            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
            {
                // Verify that KeyTip appears near the target element.
                // This scenario only works on RS4+
                var keytip = TryFindElement.ByName(keyTipText);
                Verify.IsNotNull(keytip, "keytip");
                var keyTipPopup = keytip.Parent;
                Verify.IsNotNull(keyTipPopup, "keyTipPopup");
                var keyTipBounds = keyTipPopup.BoundingRectangle;
                Log.Comment("KeyTip bounds are: " + keyTipBounds);

                var target = FindElement.ById(expectedKeyTipTargetElementId);
                var targetBounds = target.BoundingRectangle;
                Log.Comment("Target bounds are: " + targetBounds);
                targetBounds.Inflate(20, 20);

                Verify.IsTrue(keyTipBounds.IntersectsWith(targetBounds), "KeyTip bounds should be close to target bounds.");
            }

            // Invoke the AccessKey:
            TextInput.SendText(keyTipText);
            Wait.ForIdle();
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TopNavigationWithAccessKeysTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                Log.Comment("Add ten items to make overflow happen");
                Button addTenItems = new Button(FindElement.ByName("AddTenItems"));
                addTenItems.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify overflow menu is not opened");
                Verify.IsTrue(GetTopNavigationItems(TopNavPosition.Overflow).Count == 0);

                InvokeNavigationViewAccessKeyAndVerifyKeyTipPlacement("TopNavOverflowButton");

                Log.Comment("Verify overflow menu is opened");
                Verify.IsTrue(GetTopNavigationItems(TopNavPosition.Overflow).Count > 0);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void LeftNavigationWithAccessKeysTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Log.Comment("Set control to compact");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    Log.Comment("Verify that the pane is closed");
                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => ToggleState.Off,
                        () => isPaneOpenCheckBox.ToggleState,
                        () =>
                        {
                            Task.Delay(TimeSpan.FromMilliseconds(100)).Wait(); // UIA's state isn't updating immediately. Wait a sec.
                            ElementCache.Clear(); /* Test is flaky sometimes -- perhaps element cache is stale? Clear it and try again. */
                        });

                    InvokeNavigationViewAccessKeyAndVerifyKeyTipPlacement("TogglePaneButton");

                    Log.Comment("Verify that the pane is open");
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TopNavigationSelectionTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    Log.Warning("Skipping: only works in RS2 and above");
                    return;
                }

               
                Button resetResultButton = new Button(FindElement.ById("ResetResult"));
                UIObject home = FindElement.ByName("Home");
                UIObject apps = FindElement.ById("AppsItem");
                UIObject suppressSelection = FindElement.ByName("SuppressSelection");

                var invokeResult = new Edit(FindElement.ById("ItemInvokedResult"));
                var selectResult = new Edit(FindElement.ById("SelectionChangedResult"));
                var invokeRecommendedTransition = new Edit(FindElement.ById("InvokeRecommendedTransition"));
                var selectionChangeRecommendedTransition = new Edit(FindElement.ById("SelectionChangeRecommendedTransition"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    apps.Click();
                    waiter.Wait();
                }

                // First time selection raise ItemInvoke and SelectionChange events
                Verify.AreEqual(invokeResult.Value, "Apps");
                Verify.AreEqual(selectResult.Value, "Apps");
                Verify.AreEqual(invokeRecommendedTransition.Value, "Default");
                Verify.AreEqual(selectionChangeRecommendedTransition.Value, "Default");

                resetResultButton.Click();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    apps.Click();
                    waiter.Wait();
                }

                // Click it again, only raise ItemInvoke event
                Verify.AreEqual(invokeResult.Value, "Apps");
                Verify.AreEqual(selectResult.Value, "");
                Verify.AreEqual(invokeRecommendedTransition.Value, "Default");
                Verify.AreEqual(selectionChangeRecommendedTransition.Value, "");

                resetResultButton.Click();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    suppressSelection.Click();
                    waiter.Wait();
                }

                // Only click for suppress items
                Verify.AreEqual(invokeResult.Value, "SuppressSelection");
                Verify.AreEqual(selectResult.Value, "");
                Verify.AreEqual(invokeRecommendedTransition.Value, "Default");
                Verify.AreEqual(selectionChangeRecommendedTransition.Value, "");

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    home.Click();
                    waiter.Wait();
                }

                // Click home again, it raise two events. transition from right to left
                Verify.AreEqual(invokeResult.Value, "Home");
                Verify.AreEqual(selectResult.Value, "Home");
                
                // Only RS5 or above supports SlideNavigationTransitionInfo
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Verify.AreEqual(invokeRecommendedTransition.Value, "FromLeft");
                    Verify.AreEqual(selectionChangeRecommendedTransition.Value, "FromLeft");
                }

                resetResultButton.Click();
                Wait.ForIdle();

                // click apps again. transition from left to right
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    apps.Click();
                    waiter.Wait();
                }

                Verify.AreEqual(invokeResult.Value, "Apps");
                Verify.AreEqual(selectResult.Value, "Apps");

                // Only RS5 or above supports SlideNavigationTransitionInfo
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Verify.AreEqual(invokeRecommendedTransition.Value, "FromRight");
                    Verify.AreEqual(selectionChangeRecommendedTransition.Value, "FromRight");
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void TopNavigationSetSelectedItemToNullInItemInvoke()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    Log.Warning("Skipping: only works in RS2 and above");
                    return;
                }


                Button resetResultButton = new Button(FindElement.ById("ResetResult"));
                UIObject home = FindElement.ByName("Home");
                UIObject apps = FindElement.ById("AppsItem");

                var invokeResult = new Edit(FindElement.ById("ItemInvokedResult"));
                var selectResult = new Edit(FindElement.ById("SelectionChangedResult"));
                var invokeRecommendedTransition = new Edit(FindElement.ById("InvokeRecommendedTransition"));
                var selectionChangeRecommendedTransition = new Edit(FindElement.ById("SelectionChangeRecommendedTransition"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    apps.Click();
                    waiter.Wait();
                }

                // First time selection raise ItemInvoke and SelectionChange events
                Verify.AreEqual(invokeResult.Value, "Apps");
                Verify.AreEqual(selectResult.Value, "Apps");
                Verify.AreEqual(invokeRecommendedTransition.Value, "Default");
                Verify.AreEqual(selectionChangeRecommendedTransition.Value, "Default");

                resetResultButton.Click();
                Wait.ForIdle();

                Button expectNullSelectedItemInItemInvoke = new Button(FindElement.ById("ExpectNullSelectedItemInItemInvoke"));
                expectNullSelectedItemInItemInvoke.Click();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    home.Click();
                    waiter.Wait();
                }

                // Click home, expect Null in select change event 
                Verify.AreEqual(invokeResult.Value, "Home");
                Verify.AreEqual(selectResult.Value, "Null");

                // Only RS5 or above supports SlideNavigationTransitionInfo
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Verify.AreEqual(invokeRecommendedTransition.Value, "FromLeft");
                    Verify.AreEqual(selectionChangeRecommendedTransition.Value, "Default");
                }                
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void VerifyTopNavigationItemFocusVisualKindRevealTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Store Test"))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Warning("Skipping: Focus Reveal was added starting RS4");
                    return;
                }

                Log.Comment("Getting navitem active visual states");
                var getActiveVisualStateButton = new Button(FindElement.ByName("GetNavItemActiveVisualState"));

                var activeVisualStates = new Edit(FindElement.ById("NavItemActiveVisualStates"));

                // didn't figure out why, retry helps the stability.
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => {
                        getActiveVisualStateButton.Click();
                        Wait.ForIdle();
                        return activeVisualStates.GetText().Contains("OnTopNavigationPrimaryReveal");
                    },
                    retryTimoutByMilliseconds : 3000
                );

                Log.Comment("Visual states: " + activeVisualStates.GetText());
                Verify.IsTrue(activeVisualStates.GetText().Contains("OnTopNavigationPrimaryReveal"));
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void PaneTabNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                    {
                        Log.Warning("Skipping: Correct pane tab navigation only works in RS2 and above");
                        return;
                    }

                    SetNavViewWidth(ControlWidth.Wide);

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject settingsItem = FindElement.ByName("Settings");
                    UIObject nextTabTarget = FindElement.ByName("WidthComboBox");

                    CheckBox autoSuggestCheckBox = new CheckBox(FindElement.ByName("AutoSuggestCheckbox"));
                    autoSuggestCheckBox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that in Expanded mode, tab navigation can leave the pane");
                    firstItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                    Wait.ForIdle();

                    Wait.RetryUntilEvalFuncSuccessOrTimeout(
                        () => { return togglePaneButton.HasKeyboardFocus; },
                        retryTimoutByMilliseconds : 3000
                    );

                    Log.Comment("Verify pressing shift-tab from the first menu item goes to the toggle button");
                    Verify.IsTrue(togglePaneButton.HasKeyboardFocus);

                    settingsItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Log.Comment("Verify pressing tab from settings goes to the first tab stop in the content area");
                    Verify.IsTrue(nextTabTarget.HasKeyboardFocus);

                    SetNavViewWidth(ControlWidth.Medium);

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    isPaneOpenCheckBox.Check();
                    Wait.ForIdle();

                    Log.Comment("Verify that in an overlay mode, tab navigation cannot leave the pane while the pane is open");
                    firstItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                    Wait.ForIdle();
                    Log.Comment("Verify pressing shift-tab from the first menu item goes to settings");
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    settingsItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Log.Comment("Verify pressing tab from settings goes to the first menu item");
                    Verify.IsTrue(firstItem.HasKeyboardFocus);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void HomeEndNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                    {
                        Log.Warning("Skipping: Home/End behavior only works in RS2");
                        return;
                    }

                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject appsItem = FindElement.ByName("Apps");
                    UIObject lastItem = FindElement.ByName("Integer");

                    Log.Comment("Make sure something inside the ListView other than the first item has input focus");
                    appsItem.SetFocus();
                    Wait.ForIdle();
                    Verify.IsFalse(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify the Home key puts focus on the first menu item");
                    KeyboardHelper.PressKey(Key.Home);
                    Wait.ForIdle();
                    Verify.IsTrue(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify the End key puts focus on the last menu item");
                    KeyboardHelper.PressKey(Key.End);
                    Wait.ForIdle();
                    Verify.IsTrue(lastItem.HasKeyboardFocus);
                }
            }
        }

        public void MenuItemKeyboardInvokeTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Log.Comment("Verify the first menu item has focus and is selected");
                    UIObject firstItem = FindElement.ByName("Home");
                    firstItem.SetFocus();
                    Verify.IsTrue(firstItem.HasKeyboardFocus);
                    Verify.IsTrue(Convert.ToBoolean(firstItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    Log.Comment("Move focus to the second menu item by pressing down arrow");
                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();

                    Log.Comment("Verify second menu item has focus but is not selected");
                    UIObject secondItem = FindElement.ByName("Apps");
                    Verify.IsTrue(secondItem.HasKeyboardFocus);
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        Log.Comment("Select the second item by pressing enter");
                        KeyboardHelper.PressKey(Key.Enter);
                        Wait.ForIdle();
                        Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                    }
                    else
                    {
                        Log.Warning("Full test is not executing due to lack of selection on keyboard selection behaviour in older versions of ListView");
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "B")]
        public void SelectionFollowFocusTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on RS1 and earlier because SingleSelectionFollowFocus on RS2.");
                return;
            }
            var testScenarios = RegressionTestScenario.BuildTopNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {

                    Log.Comment("Check SelectionFollowFocus");
                    CheckBox selectionFollowFocusCheckbox = new CheckBox(FindElement.ById("SelectionFollowFocusCheckbox"));
                    selectionFollowFocusCheckbox.Check();
                    Wait.ForIdle();

                    UIObject firstItem = FindElement.ByName("Apps");
                    UIObject secondItem = FindElement.ByName("Games");

                    Log.Comment("Verify the second item is not already selected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    firstItem.Click();
                    Wait.ForIdle();

                    Verify.IsTrue(Convert.ToBoolean(firstItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    Log.Comment("Move focus to the second item by pressing down(left nav)/right(right nav) arrow once");
                    var key = Key.Right;
                    if (testScenario.IsLeftNavTest)
                    {
                        key = Key.Down;
                    }
                    KeyboardHelper.PressKey(key);
                    Wait.ForIdle();

                    Log.Comment("Verify second item is selected and has focus because of SelectionFollowFocus");
                    Verify.IsTrue(secondItem.HasKeyboardFocus);
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    ClickClearSelectionButton();
                    Log.Comment("second item is unselected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void MenuItemAutomationSelectionTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject secondItem = FindElement.ByName("Apps");
                    UIObject thirdItem = FindElement.ByName("Games");

                    Log.Comment("Verify the second item is not already selected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    firstItem.SetFocus();
                    AutomationElement firstItemAE = AutomationElement.FocusedElement;
                    SelectionItemPattern firstItemSIP = firstItemAE.GetCurrentPattern(SelectionItemPattern.Pattern) as SelectionItemPattern;

                    Log.Comment("Move focus to the second item by pressing down(left nav)/right(right nav) arrow once");
                    var key = Key.Right;
                    if (testScenario.IsLeftNavTest)
                    {
                        key = Key.Down;
                    }
                    KeyboardHelper.PressKey(key);
                    Wait.ForIdle();
                    Verify.IsTrue(secondItem.HasKeyboardFocus);

                    AutomationElement secondItemAE = AutomationElement.FocusedElement;
                    SelectionItemPattern secondItemSIP = secondItemAE.GetCurrentPattern(SelectionItemPattern.Pattern) as SelectionItemPattern;

                    Log.Comment("Select the second item using SelectionItemPattern.Select and verify");
                    secondItemSIP.Select();
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    Log.Comment("Deselect the second item");
                    firstItemSIP.Select();
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(firstItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));


                    Log.Comment("Select the second item using SelectionItemPattern.AddToSelection and verify");
                    secondItemSIP.AddToSelection();
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    ClickClearSelectionButton();
                    Log.Comment("second item is unselected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void SettingsCanBeUnselected()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Test"))
            {
                var readSettingsSelectedButton = new Button(FindElement.ByName("ReadSettingsSelected"));
                var SettingsSelectionStateTextBlock = new TextBlock(FindElement.ByName("SettingsSelectedState"));

                var settings = new Button(FindElement.ByName("Settings"));
                settings.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the top settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                ClickClearSelectionButton();

                Log.Comment("Verify the top settings item is unselected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "False");
            }
        }

        // Like bug 17517627, Customer like WallPaper Studio 10 expects a HeaderContent visual even if Header() is null. 
        // App crashes when they have dependency on that visual, but the crash is not directly state that it's a header problem.   
        // NavigationView doesn't use quirk, but we determine the version by themeresource.
        // As a workaround, we 'quirk' it for RS4 or before release. if it's RS4 or before, HeaderVisible is not related to Header().
        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void HeaderIsVisibleForTargetRS4OrBelowApp()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("We are running with RS4 resource, not need to run on rs2 or below");
                return;
            }
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Regression Test"))
            {
                Button button = new Button(FindElement.ById("ClearHeaderButton"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    Log.Comment("Set Header to null");

                    button.Invoke();
                    waiter.Wait();
                }

                Verify.AreEqual(invokeResult.Value, "FoundHeaderContent");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void TopNavigationOverflowButtonClickTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                //mobile test is tracked by Task 16603328.
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Setting focus to Home");
                UIObject home = FindElement.ByName("Home");
                home.SetFocus();
                Wait.ForIdle();

                Log.Comment("Add ten items to make overflow happen");
                Button addTenItems = new Button(FindElement.ByName("AddTenItems"));
                addTenItems.Invoke();
                Wait.ForIdle();

                InvokeOverflowButton();

                UIObject overflowItem = FindElement.ByName("Added Item 5");
                var invokeResult = new Edit(FindElement.ById("ItemInvokedResult"));
                var selectResult = new Edit(FindElement.ById("SelectionChangedResult"));
                var invokeRecommendedTransition = new Edit(FindElement.ById("InvokeRecommendedTransition"));
                var selectionChangeRecommendedTransition = new Edit(FindElement.ById("SelectionChangeRecommendedTransition"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    overflowItem.Click();
                    waiter.Wait();
                }

                // First time selection raise ItemInvoke and SelectionChange events
                Verify.AreEqual(invokeResult.Value, "Added Item 5");
                Verify.AreEqual(selectResult.Value, "Added Item 5");

                // only RS5 or above supports SlideNavigationTransitionInfo
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Verify.AreEqual(invokeRecommendedTransition.Value, "FromRight");
                    Verify.AreEqual(selectionChangeRecommendedTransition.Value, "FromRight");
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void TopNavigationItemsAccessibilitySetTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                Log.Comment("Setting focus to Home");
                UIObject home = FindElement.ByName("Home");
                home.SetFocus();
                Wait.ForIdle();

                AutomationElement ae = AutomationElement.FocusedElement;
                int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                Verify.AreEqual(1, positionInSet, "Position in set");
                Verify.AreEqual(4, sizeOfSet, "Size of set");
                    

                Log.Comment("Add ten items to make overflow happen");
                Button addTenItems = new Button(FindElement.ByName("AddTenItems"));
                addTenItems.Invoke();
                Wait.ForIdle();

                InvokeOverflowButton();

                UIObject overflowItem = FindElement.ByName("Added Item 5");
                overflowItem.SetFocus();
                Wait.ForIdle();

                ae = AutomationElement.FocusedElement;
                positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                Verify.AreEqual(8, positionInSet, "Position in overflow.");
                Verify.AreEqual(13, sizeOfSet, "Size of set.");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void TopNavigationMenuItemTemplateBindingTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                                page2 = new TestSetupHelper("NavigationView ItemTemplate Test"))
            {
                ElementCache.Refresh();

                // If binding has problem, we will not see the button and text.
                var lastName = new Button(FindElement.ByName("Anderberg"));
                Verify.IsNotNull(lastName);

                var firstName = new Button(FindElement.ByName("Michael"));
                Verify.IsNotNull(firstName);
            }
        }

        // Bug 17512989. If we change the menu items for multiple times, the item may be not selected.
        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void TopNavigationHaveCorrectSelectionWhenChangingMenuItems()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                for (int i = 0; i < 3; i++)
                {
                    Log.Comment("Iteration: " + i);
                    Log.Comment("Invoke ChangeDataSource");
                    var button = new Button(FindElement.ById("ChangeDataSource"));
                    button.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Reset TestResult");
                    var resetButton = new Button(FindElement.ById("ResetResult"));
                    resetButton.Invoke();
                    Wait.ForIdle();

                    ElementCache.Refresh();
                    UIObject selectedItem = FindElement.ByName("Happy new year Item");

                    Log.Comment("Verify the item is selected");
                    Verify.IsTrue(Convert.ToBoolean(selectedItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void ItemsAccessibilitySetTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Log.Comment("Setting focus to Games");
                    UIObject gamesItem = FindElement.ByName("Games");
                    gamesItem.SetFocus();
                    Wait.ForIdle();

                    AutomationElement ae = AutomationElement.FocusedElement;
                    int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                    int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                    Verify.AreEqual(3, positionInSet, "Position in set");
                    Verify.AreEqual(3, sizeOfSet, "Size of set");

                    Log.Comment("Setting focus to Movies");
                    UIObject moviesItem = FindElement.ByName("Movies");
                    moviesItem.SetFocus();
                    Wait.ForIdle();

                    ae = AutomationElement.FocusedElement;
                    positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                    sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                    Verify.AreEqual(2, positionInSet, "Position in set, not including separator/header");
                    Verify.AreEqual(2, sizeOfSet, "Size of set");
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void ItemsSourceAccessibilitySetTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                                page2 = new TestSetupHelper("NavigationView Init Test"))
            {
                Log.Comment("Set focus to second menu item");
                UIObject item = FindElement.ByName("Menu Item 2");
                item.SetFocus();
                Wait.ForIdle();

                AutomationElement ae = AutomationElement.FocusedElement;
                int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                // choose 2nd item so if it accidentally succeeds
                // on firstor last item, we don't get false positives
                Verify.AreEqual(2, positionInSet, "Position in set");
                Verify.AreEqual(3, sizeOfSet, "Size of set");

                // Perform the test again with an IIterable (IEnumerable in C# projection)
                Button changeButton = new Button(FindElement.ByName("ChangeToIEnumerableButton"));
                changeButton.Invoke();
                Wait.ForIdle();
                ElementCache.Clear();

                Log.Comment("Set focus to second menu item [2]");
                item = FindElement.ByName("IIterator/Enumerable/LinkedList Item 2");
                item.SetFocus();
                Wait.ForIdle();

                ae = AutomationElement.FocusedElement;
                positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                // choose 2nd item so if it accidentally succeeds
                // on firstor last item, we don't get false positives
                Verify.AreEqual(2, positionInSet, "Position in set [2]");
                Verify.AreEqual(3, sizeOfSet, "Size of set [2]");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void SettingsItemInvokeTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    UIObject settingsItem = testScenario.IsLeftNavTest ? FindElement.ByName("Settings") : FindElement.ByName("SettingsTopNavPaneItem");

                    settingsItem.SetFocus();
                    Wait.ForIdle();

                    AutomationElement ae = AutomationElement.FocusedElement;
                    InvokePattern invokePattern = ae.GetCurrentPattern(InvokePattern.Pattern) as InvokePattern;

                    Log.Comment("Invoking settings");
                    invokePattern.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Verify settings is selected");
                    TextBlock header = new TextBlock(FindElement.ByName("Settings as header"));
                    Verify.AreEqual("Settings as header", header.DocumentText);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void SettingsItemGamepadTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone1))
                    {
                        Log.Warning("Test is disabled on TH2 and older due to lack of gamepad engagement feature.");
                        return;
                    }

                    UIObject settingsItem = testScenario.IsLeftNavTest ? FindElement.ByName("Settings") : FindElement.ByName("SettingsTopNavPaneItem");

                    settingsItem.SetFocus();
                    Wait.ForIdle();

                    Log.Comment("Invoking settings through the gamepad");
                    GamepadHelper.PressButton(settingsItem, GamepadButton.A);
                    Wait.ForIdle();

                    Log.Comment("Verify settings is selected");
                    TextBlock header = new TextBlock(FindElement.ByName("Settings as header"));
                    Verify.AreEqual("Settings as header", header.DocumentText);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void ScrollToMenuItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    SetNavViewHeight(ControlHeight.Small);

                    // Need to get hold of the pane and search through its children for the scrollbar so as to avoid the scrollbar in the content area
                    ListView paneListView = new ListView(FindElement.ById("MenuItemsHost"));

                    UIObject lastItem = FindElement.ByName("TV");
                    UIObject firstItem = FindElement.ByName("Home");

                    Log.Comment("Verify last item is offscreen");
                    Verify.AreEqual(true, lastItem.IsOffscreen);

                    Log.Comment("Move focus to the last item by giving focus to the toggle pane button then tab twice then down arrow 5 times");
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));

                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 2);
                    Wait.ForIdle();

                    Verify.IsTrue(firstItem.HasKeyboardFocus);
                    AutomationElement firstItemAE = AutomationElement.FocusedElement;
                    KeyboardHelper.PressKey(Key.Down, ModifierKey.None, 5);
                    Wait.ForIdle();

                    Verify.IsTrue(lastItem.HasKeyboardFocus, "Verify last item has keyboard focus");

                    Log.Comment("Verify the last item is now onscreen");
                    Verify.AreEqual(false, lastItem.IsOffscreen);
                    Log.Comment("Verify the first item is now offscreen");
                    Verify.AreEqual(true, firstItem.IsOffscreen);

                    Log.Comment("Scroll to the first item using the automation ScrollItemPattern");
                    ScrollItemPattern firstItemSIP = firstItemAE.GetCurrentPattern(ScrollItemPattern.Pattern) as ScrollItemPattern;
                    firstItemSIP.ScrollIntoView();
                    Wait.ForIdle();

                    Log.Comment("Verify the first item is onscreen again");
                    Verify.AreEqual(false, firstItem.IsOffscreen);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void SystemBackTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Comment("Test does not run on phone because back button cannot be clicked on phone.");
                        return;
                    }

                    Log.Comment("Set control to compact");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();

                    Log.Comment("Open pane");
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    isPaneOpenCheckBox.Check();
                    Wait.ForIdle();

                    Log.Comment("Click back button (invoking does not work)");
                    Button backButton = new Button(FindElement.ById("__BackButton"));
                    backButton.Click(); // NOTE: Must be Click because this is verifying that the mouse light dismiss behavior closes the nav view
                    Wait.ForIdle();

                    Log.Comment("Verify that the pane is closed");

                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => ToggleState.Off,
                        () => isPaneOpenCheckBox.ToggleState,
                        () =>
                        {
                            Task.Delay(TimeSpan.FromMilliseconds(100)).Wait(); // UIA's state isn't updating immediately. Wait a sec.
                            ElementCache.Clear(); /* Test is flaky sometimes -- perhaps element cache is stale? Clear it and try again. */
                        });

                    Log.Comment("Click pane toggle button");
                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                    navButton.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Verify that the pane is open");
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void AccTypeTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    UIObject menuItem = FindElement.ByName("Games");
                    Verify.AreEqual(ControlType.ListItem, menuItem.ControlType);
                }
            }
        }

        // [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void ToolTipTest() // Verify tooltips appear, and that their contents change when headers change
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Warning("Test is disabled on mobile due to lack of tooltips.");
                        return;
                    }

                    // Close the pane
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.Invoke();
                    Wait.ForIdle();

                    UIObject menuItem = FindElement.ByName("Games");
                    Verify.AreEqual(ControlType.ListItem, menuItem.ControlType);

                    Button reverseButton = new Button(FindElement.ById("ReverseButton"));

                    Button copyTextButton = new Button(FindElement.ById("CopyGamesLabelButton"));
                    copyTextButton.Invoke();
                    Wait.ForIdle();

                    TextBlock toolTipStatusTextBlock = new TextBlock(FindElement.ByName("ToolTipStatusTextBlock"));
                    Verify.AreEqual("There are no popups", toolTipStatusTextBlock.DocumentText);

                    using (var waiter = new ToolTipOpenedWaiter())
                    {
                        menuItem.Click(); // I think the underlying SinglePointGesture doesn't do what you expect sometimes unless you click first
                        menuItem.MovePointer();
                        Log.Comment("Waiting for tooltip open event");
                        if (waiter != null)
                        {
                            waiter.Wait();
                        }

                        copyTextButton.Invoke();

                        Verify.AreEqual("Games", toolTipStatusTextBlock.DocumentText); // Verify default case
                    }

                    reverseButton.MovePointer(); // dismissssss the tooltip
                    reverseButton.Invoke();
                    Wait.ForIdle();

                    using (var waiter = new ToolTipOpenedWaiter())
                    {
                        menuItem.Click(); // I think the underlying SinglePointGesture doesn't do what you expect sometimes unless you click first
                        menuItem.MovePointer();
                        Log.Comment("Waiting for tooltip open event [2]");
                        if (waiter != null)
                        {
                            waiter.Wait();
                        }

                        copyTextButton.Invoke();

                        Verify.AreEqual("semaG", toolTipStatusTextBlock.DocumentText); // Verify tooltips change when content changes
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void KeyboardFocusToolTipTest() // Verify tooltips appear when Keyboard focused
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on mobile due to lack of tooltips.");
                    return;
                }

                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    Log.Warning("Test is disabled on RS1 and earlier because XYFocusKeyboardNavigation is not supported.");
                    return;
                }

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();

                // ToolTip is not reliable on RS2, try many times
                bool foundToolTip = false;
                for (int i = 0; i < 5; i++)
                { 
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    UIObject home = FindElement.ByName("Home");
                    Button copyTextButton = new Button(FindElement.ById("CopyGamesLabelButton"));
                    TextBlock toolTipStatusTextBlock = new TextBlock(FindElement.ByName("ToolTipStatusTextBlock"));

                    Log.Comment("tab from the TogglePaneButton goes to the search box");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    Log.Comment("Down key from search box goes to the first item");
                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();

                    Verify.IsTrue(home.HasKeyboardFocus);
                    using (var waiter = new ToolTipOpenedWaiter())
                    {
                        KeyboardHelper.PressKey(Key.Down);

                        Log.Comment("Waiting for tooltip open event");
                        waiter.Wait(TimeSpan.FromSeconds(5));
                    }

                    copyTextButton.Invoke();
                    Wait.ForIdle();
                    if (toolTipStatusTextBlock.DocumentText != null && !toolTipStatusTextBlock.DocumentText.Contains("popups"))
                    {
                        Log.Comment("Found ToolTip on Iteration " + i);
                        Verify.AreEqual("Apps", toolTipStatusTextBlock.DocumentText);
                        foundToolTip = true;
                        break;
                    }
                    else
                    {
                        Log.Comment("ToolTip Window may be closed, and didn't find ToolTip on Iteration " + i);
                    }

                }
                Verify.IsTrue(foundToolTip, "Found ToolTip");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void ToolTipCustomContentTest() // Verify tooltips don't appear for custom NavViewItems (split off due to CatGates timeout)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("We are running with RS4 resource, not need to run on rs2 or below");
                return;
            }

            // Since RS5, ToolTip is removed from ControlTemplate. and this test case can't be run on "NavigationView Tests" page 
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("NavigationView Regression Test"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on mobile due to lack of tooltips.");
                    return;
                }

                // Close the pane
                Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                Verify.IsNotNull(togglePaneButton, "Finding TogglePaneButton");
                togglePaneButton.Invoke();
                Wait.ForIdle();

                Button volumeToolTipCopyButton = new Button(FindElement.ByName("CopyVolumeToolTipButton"));
                Verify.IsNotNull(togglePaneButton, "Finding CopyVolumeToolTipButton");

                TextBlock toolTipStatusTextBlock = new TextBlock(FindElement.ByName("ToolTipStatusTextBlock"));
                Verify.IsNotNull(togglePaneButton, "Finding ToolTipStatusTextBlock");

                volumeToolTipCopyButton.Invoke();
                Wait.ForIdle();

                Verify.AreEqual("The volume navigation view item tooltip content is null", toolTipStatusTextBlock.DocumentText);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void PaneOpenCloseEventsTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
                    {
                        Log.Warning("Test is disabled on RS2 and earlier because SplitView lacks the requisite events.");
                        return;
                    }

                    TextBlock lastIngEventTextblock = new TextBlock(FindElement.ByName("LastIngEventText"));
                    TextBlock lastEdEventTextblock = new TextBlock(FindElement.ByName("LastEdEventText"));

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.Invoke();
                    Wait.ForIdle();

                    WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                    Verify.AreEqual("PaneClosed event fired", lastEdEventTextblock.DocumentText);
                    Verify.AreEqual("PaneClosing event fired", lastIngEventTextblock.DocumentText);

                    togglePaneButton.Invoke();
                    Wait.ForIdle();

                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);
                    Verify.AreEqual("PaneOpened event fired", lastEdEventTextblock.DocumentText);
                    Verify.AreEqual("PaneOpening event fired", lastIngEventTextblock.DocumentText);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void VerifyPaneTitlePresentAndUpdates()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    TextBlock paneTitleTextBlock = new TextBlock(FindElement.ByName("NavView Test"));

                    Button changePaneTitleButton = new Button(FindElement.ByName("ChangePaneTitleButton"));
                    changePaneTitleButton.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual("", paneTitleTextBlock.DocumentText, "Verify that the pane title is empty");
                }
            }
        }


        [TestMethod]
        [TestProperty("NavViewTestSuite", "C")]
        public void VerifyCustomHeaderContentTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    TextBlock tb = new TextBlock(FindElement.ByName("Home as header"));
                    Verify.AreEqual("Home as header", tb.DocumentText, "Verify initial Header text");

                    Button button = new Button(FindElement.ByName("SetHeaderButton"));
                    button.Invoke();
                    Wait.ForIdle();

                    tb = new TextBlock(FindElement.ByName("Bananas"));
                    Verify.AreEqual("Bananas", tb.DocumentText, "Verify new TextBlock child was set as the Header");
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void BackRequestedTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    TextBlock textBlock = new TextBlock(FindElement.ByName("BackRequestedStateTextBlock"));
                    Button button = new Button(FindElement.ByName("NavigationViewBackButton"));
                    CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));

                    Verify.AreEqual("Test Not Started [2]", textBlock.DocumentText);

                    checkBox.Check();
                    Wait.ForIdle();

                    button.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual("Back was requested", textBlock.DocumentText);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void BackToolTipTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is disabled on mobile due to lack of tooltips.");
                return;
            }

            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                Button button = new Button(FindElement.ByName("NavigationViewBackButton"));
                CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));

                checkBox.Check();
                Wait.ForIdle();

                using (var waiter = new ToolTipOpenedWaiter())
                {
                    Log.Comment("Moving pointer around, over back button");
                    button.MovePointer();
                    Wait.ForIdle();
                    button.MovePointer(offsetX: 1, offsetY: 1);
                    Wait.ForIdle();
                    button.MovePointer(offsetX: -1, offsetY: -1);
                    Wait.ForIdle();

                    Log.Comment("Waiting for tooltip to open");
                    waiter.Wait(TimeSpan.FromSeconds(5));
                }
            }
        }

        private void WaitAndAssertPaneStatus(PaneOpenStatus status)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
            {
                // PaneOpened and PaneClosed is introduced in RS3 for SplitView, and NavigationView depends on SplitView
                // So we can't wait for the pane to opened/closed event but just delay seconds to make animation complete.
                Wait.ForSeconds(2);

                var expectToggleStatus = status == PaneOpenStatus.Opened ? ToggleState.On : ToggleState.Off;
                CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                TestEnvironment.VerifyAreEqualWithRetry(60, // wait max to 3s
                    () => isPaneOpenCheckBox.ToggleState,
                    () => expectToggleStatus);
            }
            else
            {
                string expectString = status == PaneOpenStatus.Opened ? "Opened" : "Closed";
                var eventTextBlock = new TextBlock(FindElement.ByName("PaneOpenedOrClosedEvent"));

                Log.Comment("PaneOpenedOrClosedEvent before wait: " + eventTextBlock.GetText());
                TestEnvironment.VerifyAreEqualWithRetry(100, // wait max to 5s
                    () => expectString,
                    () => eventTextBlock.GetText());
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void LightDismissTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Warning("Test is disabled on mobile");
                        return;
                    }

                    if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
                    {
                        Log.Warning("Test is disabled on RS1 and older");
                        return;
                    }

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "Pane should be open by default since test is disabled on mobile");

                    SetNavViewWidth(ControlWidth.Medium);
                    WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                    using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                    {
                        isPaneOpenCheckBox.Toggle();
                        waiter.Wait();
                    }
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    PaneOpenCloseTestCaseRetry(3, () =>
                    {
                        KeyboardHelper.PressKey(Key.Backspace, ModifierKey.Windows);
                        Wait.ForIdle();
                        WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                        Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "Verify Windows+Back light dismisses the pane");
                    });

                    isPaneOpenCheckBox.Toggle();
                    Wait.ForIdle();
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    PaneOpenCloseTestCaseRetry(3, () =>
                    {
                        KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt);
                        Wait.ForIdle();
                        WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                        Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "Verify Alt+Left light dismisses the pane");
                    });

                    isPaneOpenCheckBox.Toggle();
                    Wait.ForIdle();
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void CheckSelectedItemEdgeCase()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                   page2 = new TestSetupHelper("SelectedItem edge case test"))
            {
                Button button = new Button(FindElement.ByName("CopyStatusButton"));
                TextBlock textBlock = new TextBlock(FindElement.ByName("StatusTextBlock"));

                button.Invoke();
                Wait.ForIdle();
                Verify.AreEqual("False True", textBlock.DocumentText);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyCanCancelClosing()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Warning("Test is disabled on mobile");
                        return;
                    }

                    if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
                    {
                        Log.Warning("Test is disabled on RS2 and older due to lack of SplitView events");
                        return;
                    }

                    SetNavViewWidth(ControlWidth.Medium);

                    var cancelClosingCheckbox = new CheckBox(FindElement.ById("CancelClosingEvents"));
                    cancelClosingCheckbox.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, cancelClosingCheckbox.ToggleState);

                    var isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                    Log.Comment("Reset the event count");
                    new Button(FindElement.ById("ClosingEventCountResetButton")).Invoke();
                    Wait.ForIdle();

                    Log.Comment("Open the Pane");
                    using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                    {
                        isPaneOpenCheckBox.Toggle();
                        waiter.Wait();
                    }
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                    var closingCounts = new Edit(FindElement.ByName("ClosingEventCountTextBlock"));
                    var expectedString = "1-0";

                    //  trigger a light dismiss
                    KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt);
                    Wait.ForIdle();

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState,
                        "Verify Alt+Left light dismiss doesn't dismiss the pane when closing events are being canceled");

                    Verify.AreEqual(expectedString, closingCounts.GetText());
                }
            }
        }

        private void PaneOpenCloseTestCaseRetry(int retryNumber, Action action)
        {
            for (int i = 0; i < retryNumber; i++)
            {
                try
                {
                    if (i > 0)
                    {
                        Log.Comment("Retry on " + i);
                    }
                    action();
                    return;
                }
                catch (Exception e)
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Log.Comment("IsPaneOpenCheckBox toggle status: " + isPaneOpenCheckBox.ToggleState);

                    Edit closingCounts = new Edit(FindElement.ByName("ClosingEventCountTextBlock"));
                    Log.Comment("ClosingEventCountTextBlock text: " + closingCounts.GetText());

                    TextBlock eventTextBlock = new TextBlock(FindElement.ByName("PaneOpenedOrClosedEvent"));
                    Log.Comment("PaneOpenedOrClosedEvent text: " + eventTextBlock.GetText());

                    Log.Comment(e.Message);
                }
            }

            throw new Exception("Reach max number of retry " + retryNumber);
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyLightDismissDoesntSendDuplicateEvents()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                    {
                        Log.Warning("Test is disabled on mobile");
                        return;
                    }

                    if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
                    {
                        Log.Warning("Test is disabled on RS2 and older due to lack of SplitView events");
                        return;
                    }
                    
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "Pane should be open by default since test is disabled on mobile");

                    SetNavViewWidth(ControlWidth.Medium);
                    WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                    PaneOpenCloseTestCaseRetry(3, () =>
                        {
                            // recover from the exception if needed
                            if (isPaneOpenCheckBox.ToggleState != ToggleState.Off)
                            {
                                using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                                {
                                    isPaneOpenCheckBox.Toggle();
                                    waiter.Wait();
                                }
                                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                            }

                            Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                            Log.Comment("Reset the event count");
                            new Button(FindElement.ById("ClosingEventCountResetButton")).Invoke();
                            Wait.ForIdle();

                            Log.Comment("Open the pane");
                            using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                            {
                                isPaneOpenCheckBox.Toggle();
                                waiter.Wait();
                            }
                            WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                            Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);

                            var closingCounts = new Edit(FindElement.ByName("ClosingEventCountTextBlock"));
                            var expectedString = "1-1";

                            //  trigger a light dismiss
                            KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt);
                            Wait.ForIdle();

                            WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                            Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);
                            Verify.AreEqual(expectedString, closingCounts.GetText());
                        });
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyBackButtonAccessibleOnlyViaXYKeyboard()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    bool doVerfications = true;

                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        // I want the test to still run, just to uncover any crashes that could occur
                        Log.Warning("This test only works on RS4, but is running without 'verifications' on RS3 and below to weasel our crashes");
                        doVerfications = false;
                    }

                    Button backButton = new Button(FindElement.ByName("NavigationViewBackButton"));
                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                    Button systemBackButton = new Button(UIObject.Root.Descendants.Find(UICondition.CreateFromId("__BackButton")));
                    UIObject searchBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");

                    CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));
                    checkBox.Toggle();
                    searchBox.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    if (doVerfications) Verify.AreEqual(true, navButton.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    if (doVerfications) Verify.AreEqual(true, systemBackButton.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Tab);
                    if (doVerfications) Verify.AreEqual(true, navButton.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    if (doVerfications) Verify.AreEqual(true, backButton.HasKeyboardFocus);

                    Log.Comment("Test that it works with left/right in minimal closed mode");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();

                    navButton.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    if (doVerfications) Verify.AreEqual(true, systemBackButton.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Tab);
                    if (doVerfications) Verify.AreEqual(true, navButton.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Left);
                    if (doVerfications) Verify.AreEqual(true, backButton.HasKeyboardFocus);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyDeselectionDisabled()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    UIObject homeItem = FindElement.ByName("Home");
                    Verify.IsNotNull(homeItem);
                    Verify.IsTrue(Convert.ToBoolean(homeItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                    homeItem.Click(); // Explicitly testing ctrl+click here
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(homeItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                    KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void EnsureClearingListIsSafe()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    Button clearButton = new Button(FindElement.ByName("ClearMenuButton"));
                    Log.Comment("About to invoke list clear button");
                    clearButton.Invoke();
                    Log.Comment("About to wait for idle");
                    Wait.ForIdle();

                    // that's it, it's a stability test
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        [TestProperty("Description", "Ensure that the NavigationView button isn't running with rs3+ themeresource on when they're off :)")]
        public void VerifyNotShouldPreserveNavigationViewRS3Behavior() // Regression test to make sure that we aren't accidentally running quirks all the time
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        Log.Warning("This test is only designed to run on RS4+ machines");
                        return;
                    }
                    else
                    {
                        CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                        // On phone, the pane will initially be in the closed compact state, so open it before
                        // proceeding with the test.
                        if (isPaneOpenCheckBox.ToggleState == ToggleState.Off)
                        {
                            using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                            {
                                isPaneOpenCheckBox.Toggle();
                                waiter.Wait();
                            }
                        }

                        Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                        Verify.AreEqual(320, navButton.BoundingRectangle.Width);
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        [TestProperty("Description", "Ensure that the NavigationView button is rendering as expected if it's targeting RS3")]
        public void VerifyShouldPreserveNavigationViewRS3Behavior()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                                page2 = new TestSetupHelper("NavigationView PreserveRS3 Test"))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Warning("This test is only designed to run on RS4+ machines");
                    return;
                }
                
                CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                // On phone, the pane will initially be in the closed compact state, so open it before
                // proceeding with the test.
                if (isPaneOpenCheckBox.ToggleState == ToggleState.Off)
                {
                    using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                    {
                        isPaneOpenCheckBox.Toggle();
                        waiter.Wait();
                    }
                }

                Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                Verify.AreEqual(48, navButton.BoundingRectangle.Width);

                // In RS4 or late application, togglePaneTopPadding is 0 when ExtendViewIntoTitleBar=true, 
                // but for RS3 application, we expected it be not 0 because apps like Wallpaper make use of it
                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button button = new Button(FindElement.ById("GetTopPaddingHeight"));
                    button.Invoke();                    
                    waiter.Wait();
                }
                var togglePaneTopPadding = Convert.ToInt32(result.Value);
                Verify.AreNotEqual(0, togglePaneTopPadding);

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button button = new Button(FindElement.ById("GetToggleButtonRowHeight"));
                    button.Invoke();
                    waiter.Wait();
                }
                var toggleButtonHeight = Convert.ToInt32(result.Value);
                Verify.AreEqual(56, toggleButtonHeight);

                // TestFrame is disabled before the testcase. we should enable it and prepare for next test case
                var testFrame = new CheckBox(FindElement.ById("TestFrameCheckbox"));
                testFrame.Check();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        [TestProperty("Description", "Temporary bootstrapping test, can be retired once Horizontal Nav View is out of incubation")]
        public void EnsureNoCrashesInHorizontalFlipMenuItems() 
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    var button = new Button(FindElement.ByName("FlipOrientationButton"));
                    button.Invoke();
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        [TestProperty("Description", "VisualState DisplayModeGroup is decoupled from DisplayMode, and it has strong connection with PaneDisplayMode")]
        public void VerifyCorrectVisualStateWhenChangingPaneDisplayMode()
        {
            // We expect this mapping:
            //  Top, and LeftMinimal -> VisualState Minimal
            //  LeftCompact -> VisualState Compact
            //  Left -> VisualState Expanded

            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                var getActiveVisualStateButton = new Button(FindElement.ByName("GetActiveVisualState"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                var isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                Log.Comment("Set PaneDisplayMode to Top");
                panelDisplayModeComboBox.SelectItemByName("Top");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Minimal"));

                Log.Comment("Set PaneDisplayMode to Left");
                panelDisplayModeComboBox.SelectItemByName("Left");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Expanded"));

                Log.Comment("Set PaneDisplayMode to Top");
                panelDisplayModeComboBox.SelectItemByName("Top");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Minimal"));

                Log.Comment("Set PaneDisplayMode to LeftCompact");
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Compact"));

                Log.Comment("Set PaneDisplayMode to LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Minimal"));
                Log.Comment("Verify Pane is closed automatically in PaneDisplayMode is Minimal");
                Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState);

                Log.Comment("Set DisplayMode to Left");
                panelDisplayModeComboBox.SelectItemByName("Left");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Expanded"));
                Log.Comment("Verify Pane is opened automatically in PaneDisplayMode is changed from Minimal to Left");
                Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void EnsureTopSettingsRetainsFocusAfterOrientationChanges()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                var readSettingsSelectedButton = new Button(FindElement.ByName("ReadSettingsSelected"));
                var SettingsSelectionStateTextBlock = new TextBlock(FindElement.ByName("SettingsSelectedState"));

                var leftSettingsItem = new Button(FindElement.ByName("Settings"));
                leftSettingsItem.Invoke();

                Log.Comment("Verify the left settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Left -> Top.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the top settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Top -> Left.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the left settings item is still selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void EnsureDynamicSizeForPaneHeaderFooterAndCustomContent()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                page2 = new TestSetupHelper("NavigationView Stretch Test"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    // ContentAlignment Stretch is implemented by XAML, and there is not need to verify on phone
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                // NavigationViewCompactPaneLength is 40 or 48 in different release. This test case doesn't need an exactly number of width, so just choose 48 as the boundary
                // PaneHeader share the same row with ToggleButton, so it's width is not the same with other buttons
                var widthCompactBoundary = 48;
                var widthOpenPaneLength = 320;

                Button paneHeaderButton = new Button(FindElement.ById("PaneHeader"));
                Log.Comment("PaneHeader size actual width is " + paneHeaderButton.BoundingRectangle.Width);
                Verify.IsTrue(paneHeaderButton.BoundingRectangle.Width > widthCompactBoundary && paneHeaderButton.BoundingRectangle.Width < widthOpenPaneLength);

                Button paneFooterButton = new Button(FindElement.ById("PaneFooter"));
                Log.Comment("PaneFooter size actual width is " + paneFooterButton.BoundingRectangle.Width);
                Verify.IsTrue(paneFooterButton.BoundingRectangle.Width == widthOpenPaneLength);

                Button paneCustomContentButton = new Button(FindElement.ById("PaneCustomContent"));
                Log.Comment("paneCustomContentButton size actual width is " + paneCustomContentButton.BoundingRectangle.Width);
                Verify.IsTrue(paneCustomContentButton.BoundingRectangle.Width == widthOpenPaneLength);

                Log.Comment("Verify that clicking the navigation button closes the nav pane");
                navButton.Invoke();
                Wait.ForIdle();

                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    // It returns negative width on RS1 and cause test failure.
                    // Error System.ArgumentException: Size_WidthAndHeightCannotBeNegative at 
                    // Microsoft.Windows.Apps.Test.Automation.UiaConvert.ConvertPropertyValue(AutomationProperty property, Object propertyValue) 
                    // at Microsoft.Windows.Apps.Test.Automation.AutomationElement.GetCurrentPropertyValue(AutomationProperty property, Boolean ignoreDefaultValue)
                    Log.Comment("Skip PaneHeader verification");
                }
                else
                {
                    paneHeaderButton = new Button(FindElement.ById("PaneHeader"));
                    Log.Comment("PaneHeader is collapsed");
                    Verify.IsTrue(paneHeaderButton.BoundingRectangle.Width == 0);
                }
                Log.Comment("PaneFooter size actual width is " + paneFooterButton.BoundingRectangle.Width);
                Verify.IsTrue(paneFooterButton.BoundingRectangle.Width <= widthCompactBoundary && paneFooterButton.BoundingRectangle.Width > 0);

                Log.Comment("paneCustomContentButton size actual width is " + paneCustomContentButton.BoundingRectangle.Width);
                Verify.IsTrue(paneCustomContentButton.BoundingRectangle.Width <= widthCompactBoundary && paneCustomContentButton.BoundingRectangle.Width > 0);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyHeaderContentMarginOnTopNav()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                Log.Comment("Flipping orientation: Left -> Top.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Get HeaderContent Margin");
                var getHeaderContentMarginButton = new Button(FindElement.ByName("GetHeaderContentMargin"));
                getHeaderContentMarginButton.Invoke();
                Wait.ForIdle();

                var result = new TextBlock(FindElement.ByName("HeaderContentMarginResult"));
                Verify.AreEqual(result.GetText(), "12,5,0,11");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyTopNavigationMinimalVisualStateOnTopNav()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                Log.Comment("To Minimal mode");
                SetNavViewWidth(ControlWidth.Narrow);

                Log.Comment("Get NavView Active VisualStates");
                var getNavViewActiveVisualStatesButton = new Button(FindElement.ByName("GetNavViewActiveVisualStates"));
                getNavViewActiveVisualStatesButton.Invoke();
                Wait.ForIdle();

                var visualStateName = "TopNavigationMinimal";
                var result = new TextBlock(FindElement.ByName("NavViewActiveVisualStatesResult"));
                Verify.IsFalse(result.GetText().Contains(visualStateName), "active VisualStates doesn't include " + visualStateName);

                Log.Comment("To Wide mode");
                SetNavViewWidth(ControlWidth.Wide);

                Log.Comment("Flipping orientation: Left -> Top.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Get NavView Active VisualStates");
                getNavViewActiveVisualStatesButton.Invoke();
                Wait.ForIdle();

                Verify.IsTrue(result.GetText().Contains(visualStateName), "active VisualStates includes " + visualStateName);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void EnsureLeftSettingsRetainsFocusAfterOrientationChanges()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                var readSettingsSelectedButton = new Button(FindElement.ByName("ReadSettingsSelected"));
                var SettingsSelectionStateTextBlock = new TextBlock(FindElement.ByName("SettingsSelectedState"));

                Log.Comment("Flipping orientation: Left -> Top.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                var topSettingsItem = new Button(FindElement.ByName("SettingsTopNavPaneItem"));
                topSettingsItem.Invoke();

                Log.Comment("Verify the top settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Top -> Left.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the left settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Left -> Top.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the left top item is still selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        [TestProperty("Description", "Temporary bootstrapping test, can be retired once Horizontal Nav View is out of incubation")]
        public void EnsureNoCrashesInHorizontalFlipMenuItemsSource()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                                page2 = new TestSetupHelper("Top Nav Test"))  // This navigates through to our test page
            {
                var button = new Button(FindElement.ByName("FlipOrientationButton"));
                button.Invoke();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void VerifyMoreButtonIsOnlyReadOnce()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Test"))
            {
                UIObject moreButton = FindElement.ById("TopNavOverflowButton");
                moreButton.SetFocus();
                Wait.ForIdle();

                AutomationElement ae = AutomationElement.FocusedElement;
                Verify.AreEqual("More", ae.GetCurrentPropertyValue(AutomationElement.NameProperty).ToString());
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void CanDoSelectionChangedOfItemTemplate()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                                page2 = new TestSetupHelper("NavigationView ItemTemplate Test"))
            {
                // Go to the navview items.
                KeyboardHelper.PressKey(Key.Tab);
                // Select the first item.
                KeyboardHelper.PressKey(Key.Space);
                // Go to the second item.
                KeyboardHelper.PressKey(Key.Right);
                // Select the second item.
                KeyboardHelper.PressKey(Key.Space);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void EnsurePaneHeaderCanBeModifiedForLeftNav()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                EnsurePaneHeaderCanBeModifiedHelper(RegressionTestType.LeftNav);
            }
        }

        [TestMethod]
        [TestProperty("NavViewTestSuite", "D")]
        public void EnsurePaneHeaderCanBeModifiedForTopNav()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                             page2 = new TestSetupHelper("NavigationView Test"))
            {
                EnsurePaneHeaderCanBeModifiedHelper(RegressionTestType.TopNav);
            }
        }

        private void EnsurePaneHeaderCanBeModifiedHelper(RegressionTestType navviewMode)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on RS1 and earlier because Pane Header is on RS2.");
                return;
            }

            if (navviewMode == RegressionTestType.TopNav)
            {
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();
            }

            var changePaneHeaderbutton = new Button(FindElement.ByName("ChangePaneHeader"));
            changePaneHeaderbutton.Invoke();
            Wait.ForIdle();

            UIObject paneHeaderContent = null;

            if (navviewMode == RegressionTestType.TopNav)
            {
                paneHeaderContent = FindElement.ById("PaneHeaderOnTopPane");
            }
            else
            {
                paneHeaderContent = FindElement.ById("PaneHeaderContentBorder");
            }

            TextBlock text = new TextBlock(paneHeaderContent.FirstChild);
            Verify.AreEqual("Modified Pane Header", text.DocumentText);

            if(navviewMode == RegressionTestType.LeftNav)
            {
                // In Closed Compact mode, the PaneHeader should not be visible:

                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                EnsureNavViewClosed();

                ElementCache.Clear();
                VerifyElement.NotFound("PaneHeaderContentBorder", FindBy.Name);
            }
        }

        private void EnsureNavViewClosed()
        {
            CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
            if (isPaneOpenCheckBox.ToggleState == ToggleState.On)
            {
                using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                {
                    isPaneOpenCheckBox.Uncheck();
                    waiter.Wait();
                }
            }
            Wait.ForIdle();
        }

        private List<UIObject> GetTopNavigationItems(TopNavPosition position)
        {
            string hostId = position == TopNavPosition.Overflow ? "TopNavMenuItemsOverflowHost" : "TopNavMenuItemsHost";
            List<UIObject> collection = new List<UIObject>();

            var host = TryFindElement.ById(hostId);
            if (host != null)
            {
                collection.AddRange(host.Children);
            }
            else
            {
                Log.Warning("Can't find container " + hostId);
            }
            return collection;
        }

        private void InvokeOverflowButton()
        {
            Log.Comment("Invoke More button to open/close Overflow menu");
            var moreButton = TryFindElement.ById("TopNavOverflowButton");
            Verify.IsNotNull(moreButton, "Overflow button should exist");
            new Button(moreButton).InvokeAndWait();               
        }

        private string UIObjectToString(UIObject uIObject)
        {
            return (uIObject == null) ? "" :
                string.Join("/",
                    new[] { uIObject.Name, uIObject.ClassName, uIObject.AutomationId }.
                        Where(s => s != null));
        }

        private bool UIObjectContains(UIObject uIObject, string itemName)
        {
            return UIObjectToString(uIObject).Contains(itemName);
        }

        private void OpenOverflowMenuAndInvokeItem(string itemName)
        {
            InvokeOverflowButton();

            var host = TryFindElement.ById("TopNavMenuItemsOverflowHost");
            Verify.IsNotNull(host, "Overflow menu should be opened");

            var overflowItems = GetTopNavigationItems(TopNavPosition.Overflow);
            var items = overflowItems.
                Where(item => UIObjectContains(item, itemName));

            var count = items.Count();
            if (count == 0)
            {
                Log.Comment("Items in overflow: ", String.Join("@", overflowItems.Select(item => UIObjectToString(item))));
            }
            Verify.IsTrue(count > 0, "There should be at least one item match with " + itemName);

            if (count > 1)
            {
                Log.Warning("There is more than one item match with" + itemName + " and first item is invoked");
            }

            var itemToBeClicked = items.ElementAt(0);
            Log.Comment("Invoke the item " + UIObjectToString(itemToBeClicked));
            new Button(itemToBeClicked).Invoke();
            Wait.ForIdle();
            //When a overflow item is clicked, NavView depends on another UI ticket to update the layout.
            Wait.ForSeconds(1);
            Wait.ForIdle();
        }
    }

    [Flags]
    enum RegressionTestType
    {
        LeftNav = 1,
        TopNav = 2,
        LeftNavRS4 = 4
    }
    class RegressionTestScenario
    {
        private RegressionTestScenario(string testPagename, bool isLeftnavTest, bool isUsingRS4Style)
        {
            TestPageName = testPagename;
            IsLeftNavTest = isLeftnavTest;
            IsUsingRS4Style = isUsingRS4Style;
        }
        public string TestPageName { get; private set; }
        public bool IsLeftNavTest { get; private set; }
        public bool IsUsingRS4Style { get; private set; }
        public static List<RegressionTestScenario> BuildLeftNavRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.LeftNav | RegressionTestType.LeftNavRS4);
        }
        public static List<RegressionTestScenario> BuildAllRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.LeftNav | RegressionTestType.LeftNavRS4 | RegressionTestType.TopNav);
        }
        public static List<RegressionTestScenario> BuildTopNavRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.LeftNav | RegressionTestType.TopNav);
        }
        private static List<RegressionTestScenario> BuildTestScenarios(RegressionTestType types)
        {
            Dictionary<RegressionTestType, RegressionTestScenario> map =
                new Dictionary<RegressionTestType, RegressionTestScenario>
            {
                    { RegressionTestType.LeftNav, new RegressionTestScenario("NavigationView Test", isLeftnavTest: true, isUsingRS4Style: false)},
                    { RegressionTestType.LeftNavRS4, new RegressionTestScenario("NavigationView Regression Test", isLeftnavTest: true, isUsingRS4Style: true)},
                    { RegressionTestType.TopNav, new RegressionTestScenario("NavigationView TopNav Test", isLeftnavTest: false, isUsingRS4Style: false)},
            };

            List<RegressionTestScenario> scenarios = new List<RegressionTestScenario>();
            foreach (RegressionTestType type in Enum.GetValues(typeof(RegressionTestType)))
            {
                if (types.HasFlag(type))
                {
                    scenarios.Add(map[type]);
                }
            }
            return scenarios;
        }
    }
}
