﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using MUXControlsTestAppForIslands;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MUXControlsTestAppForIslands
{
    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>();

            Tests.Add(new TestDeclaration("NavigationView Tests", typeof(NavigationViewTestPage)));
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}
