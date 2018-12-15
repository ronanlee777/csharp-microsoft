using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;

namespace HelixTestHelpers
{
    public static class TestResultParser
    {
        public class TestPass
        {
            public TimeSpan TestPassExecutionTime { get; set; }
            public List<TestResult> TestResults { get; set; }
        }

        public class TestResult
        {
            public string Name { get; set; }
            public bool Passed { get; set; }
            public bool CleanupPassed { get; set; }
            public TimeSpan ExecutionTime { get; set; }
            public string Details { get; set; }
        }

        public static void ConvertWttLogToXUnitLog(string wttInputPath, string xunitOutputPath)
        {
            var testPass = TestResultParser.ParseTestWttFile(wttInputPath, true);
            var results = testPass.TestResults;

            int resultCount = results.Count;
            int passedCount = results.Where(r => r.Passed).Count();
            int failedCount = resultCount - passedCount;

            var root = new XElement("assemblies");

            var assembly = new XElement("assembly");
            assembly.SetAttributeValue("name", "MUXControls.Test.dll");
            assembly.SetAttributeValue("test-framework", "TAEF");
            assembly.SetAttributeValue("run-date", DateTime.Now.ToString("yyyy-mm-dd"));

            // This doesn't need to be completely accurate since it's not exposed anywhere.
            // If we need accurate an start time we can probably calculate it from the te.wtl file, but for
            // now this is fine.
            assembly.SetAttributeValue("run-time", (DateTime.Now - testPass.TestPassExecutionTime).ToString("hh:mm:ss"));
            
            assembly.SetAttributeValue("total", resultCount);
            assembly.SetAttributeValue("passed", passedCount);
            assembly.SetAttributeValue("failed", failedCount);
            assembly.SetAttributeValue("skipped", 0);
            assembly.SetAttributeValue("time", (int)testPass.TestPassExecutionTime.TotalSeconds);
            assembly.SetAttributeValue("errors", 0);
            root.Add(assembly);

            var collection = new XElement("collection");
            collection.SetAttributeValue("total", resultCount);
            collection.SetAttributeValue("passed", passedCount);
            collection.SetAttributeValue("failed", failedCount);
            collection.SetAttributeValue("skipped", 0);
            collection.SetAttributeValue("name", "Test collection");
            collection.SetAttributeValue("time", (int)testPass.TestPassExecutionTime.TotalSeconds);
            assembly.Add(collection);

            foreach (var result in results)
            {
                var test = new XElement("test");
                test.SetAttributeValue("name", result.Name);

                var className = result.Name.Substring(0, result.Name.LastIndexOf('.'));
                var methodName = result.Name.Substring(result.Name.LastIndexOf('.') + 1);
                test.SetAttributeValue("type", className);
                test.SetAttributeValue("method", methodName);

                test.SetAttributeValue("time", result.ExecutionTime.TotalSeconds);
                test.SetAttributeValue("result", result.Passed ? "Pass" : "Fail");

                if (!result.Passed)
                {
                    var failure = new XElement("failure");
                    failure.SetAttributeValue("exception-type", "Exception");

                    var message = new XElement("message");
                    message.Add(new XCData(result.Details));
                    failure.Add(message);

                    test.Add(failure);
                }

                collection.Add(test);
            }

            File.WriteAllText(xunitOutputPath, root.ToString());
        }
        public static TestPass ParseTestWttFile(string fileName, bool cleanupFailuresAreRegressions)
        {
            using (var stream = System.IO.File.OpenRead(fileName))
            {
                var doc = XDocument.Load(stream);
                var testResults = new List<TestResult>();
                var testExecutionTimeMap = new Dictionary<string, List<double>>();

                TestResult currentResult = null;
                long frequency = 0;
                long startTime = 0;
                long stopTime = 0;
                bool inTestCleanup = false;

                bool shouldLogToTestDetails = false;

                long testPassStartTime = 0;
                long testPassStopTime = 0;

                Func<XElement, bool> isScopeData = (elt) =>
                {
                    return
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                        (
                            elt.Element("Data").Element("WexContext").Value == "Cleanup" ||
                            elt.Element("Data").Element("WexContext").Value == "TestScope" ||
                            elt.Element("Data").Element("WexContext").Value == "TestScope" ||
                            elt.Element("Data").Element("WexContext").Value == "ClassScope" ||
                            elt.Element("Data").Element("WexContext").Value == "ModuleScope"
                        );
                };

                Func<XElement, bool> isModuleOrClassScopeStart = (elt) =>
                {
                    return
                        elt.Name == "Msg" &&
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("StartGroup") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                            (elt.Element("Data").Element("WexContext").Value == "ClassScope" ||
                            elt.Element("Data").Element("WexContext").Value == "ModuleScope");
                };

                Func<XElement, bool> isModuleScopeEnd = (elt) =>
                {
                    return
                        elt.Name == "Msg" &&
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("EndGroup") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                        elt.Element("Data").Element("WexContext").Value == "ModuleScope";
                };

                Func<XElement, bool> isClassScopeEnd = (elt) =>
                {
                    return
                        elt.Name == "Msg" &&
                        elt.Element("Data") != null &&
                        elt.Element("Data").Element("EndGroup") != null &&
                        elt.Element("Data").Element("WexContext") != null &&
                        elt.Element("Data").Element("WexContext").Value == "ClassScope";
                };

                int testsExecuting = 0;
                foreach (XElement element in doc.Root.Elements())
                {
                    // Capturing the frequency data to record accurate
                    // timing data.
                    if (element.Name == "RTI")
                    {
                        frequency = Int64.Parse(element.Attribute("Frequency").Value);
                    }

                    // It's possible for a test to launch another test. If that happens, we won't modify the
                    // current result. Instead, we'll continue operating like normal and expect that we get two
                    // EndTests nodes before our next StartTests. We'll check that we've actually got a stop time
                    // before creating a new result. This will result in the two results being squashed
                    // into one result of the outer test that ran the inner one.
                    if (element.Name == "StartTest")
                    {
                        testsExecuting++;
                        if (testsExecuting == 1)
                        {
                            string testName = element.Attribute("Title").Value;
                            const string xamlNativePrefix = "Windows::UI::Xaml::Tests::";
                            const string xamlManagedPrefix = "Windows.UI.Xaml.Tests.";
                            if (testName.StartsWith(xamlNativePrefix))
                            {
                                testName = testName.Substring(xamlNativePrefix.Length);
                            }
                            else if (testName.StartsWith(xamlManagedPrefix))
                            {
                                testName = testName.Substring(xamlManagedPrefix.Length);
                            }

                            currentResult = new TestResult() { Name = testName, Passed = true, CleanupPassed = true };
                            testResults.Add(currentResult);
                            startTime = Int64.Parse(element.Descendants("WexTraceInfo").First().Attribute("TimeStamp").Value);
                            inTestCleanup = false;
                            shouldLogToTestDetails = true;
                            stopTime = 0;
                        }
                    }
                    else if (currentResult != null && element.Name == "EndTest")
                    {
                        testsExecuting--;

                        // If any inner test fails, we'll still fail the outer
                        currentResult.Passed &= element.Attribute("Result").Value == "Pass";

                        // Only gather execution data if this is the outer test we ran initially
                        if (testsExecuting == 0)
                        {
                            stopTime = Int64.Parse(element.Descendants("WexTraceInfo").First().Attribute("TimeStamp").Value);
                            if (!testExecutionTimeMap.Keys.Contains(currentResult.Name))
                                testExecutionTimeMap[currentResult.Name] = new List<double>();
                            testExecutionTimeMap[currentResult.Name].Add((double)(stopTime - startTime) / frequency);
                            currentResult.ExecutionTime = TimeSpan.FromSeconds(testExecutionTimeMap[currentResult.Name].Average());

                            startTime = 0;
                            inTestCleanup = true;
                        }
                    }
                    else if (currentResult != null &&
                            (isModuleOrClassScopeStart(element) || isModuleScopeEnd(element) || isClassScopeEnd(element)))
                    {
                        shouldLogToTestDetails = false;
                        inTestCleanup = false;
                    }

                    // Log-appending methods.
                    if (currentResult != null && element.Name == "Error")
                    {
                        if (shouldLogToTestDetails)
                        {
                            currentResult.Details += "\r\n[Error]: " + element.Attribute("UserText").Value;
                            if (element.Attribute("File") != null && element.Attribute("File").Value != "")
                            {
                                currentResult.Details += (" [File " + element.Attribute("File").Value);
                                if (element.Attribute("Line") != null)
                                    currentResult.Details += " Line: " + element.Attribute("Line").Value;
                                currentResult.Details += "]";
                            }
                        }


                        // The test cleanup errors will often come after the test claimed to have
                        // 'passed'. We treat them as errors as well. 
                        if (inTestCleanup)
                        {
                            currentResult.CleanupPassed = false;
                            currentResult.Passed = false;
                            // In stress mode runs, this test will run n times before cleanup is run. If the cleanup
                            // fails, we want to fail every test.
                            if (cleanupFailuresAreRegressions)
                            {
                                foreach (var result in testResults.Where(res => res.Name == currentResult.Name))
                                {
                                    result.Passed = false;
                                    result.CleanupPassed = false;
                                }
                            }
                        }
                    }

                    if (currentResult != null && element.Name == "Warn")
                    {
                        if (shouldLogToTestDetails)
                        {
                            currentResult.Details += "\r\n[Warn]: " + element.Attribute("UserText").Value;
                        }

                        if (element.Attribute("File") != null && element.Attribute("File").Value != "")
                        {
                            currentResult.Details += (" [File " + element.Attribute("File").Value);
                            if (element.Attribute("Line") != null)
                                currentResult.Details += " Line: " + element.Attribute("Line").Value;
                            currentResult.Details += "]";
                        }
                    }
                }

                testPassStartTime = Int64.Parse(doc.Root.Descendants("WexTraceInfo").First().Attribute("TimeStamp").Value);
                testPassStopTime = Int64.Parse(doc.Root.Descendants("WexTraceInfo").Last().Attribute("TimeStamp").Value);

                var testPassTime = TimeSpan.FromSeconds((double)(testPassStopTime - testPassStartTime) / frequency);

                var testpass = new TestPass
                {
                    TestPassExecutionTime = testPassTime,
                    TestResults = testResults
                };

                return testpass;
            }
        }
    }
}
