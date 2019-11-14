﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml.Controls;

using ItemsSourceView = Microsoft.UI.Xaml.Controls.ItemsSourceView;
using NonVirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.NonVirtualizingLayoutContext;
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks
{
    class MockVirtualizingLayout : VirtualizingLayout
    {
        public Func<Size, VirtualizingLayoutContext, Size> MeasureLayoutFunc { get; set; }
        public Func<Size, VirtualizingLayoutContext, Size> ArrangeLayoutFunc { get; set; }

        public new void InvalidateMeasure()
        {
            base.InvalidateMeasure();
        }

        protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
        {
            return MeasureLayoutFunc != null ? MeasureLayoutFunc(availableSize, context) : default(Size);
        }

        protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
        {
            return ArrangeLayoutFunc != null ? ArrangeLayoutFunc(finalSize, context) : default(Size);
        }
    }
}
