using System;
using System.Runtime.InteropServices;

namespace PSFExtractor.SplitPSF
{
    internal static class NativeMethods
    {
        [DllImport("msdelta.dll", SetLastError = true, EntryPoint = "ApplyDeltaW")]
        [return: MarshalAsAttribute(UnmanagedType.Bool)]
        internal static extern bool ApplyDeltaW(long ApplyFlags, [MarshalAs(UnmanagedType.LPWStr)] string lpSourceName, [MarshalAs(UnmanagedType.LPWStr)] string lpDeltaName, [MarshalAs(UnmanagedType.LPWStr)] string lpTargetName);

    }
}