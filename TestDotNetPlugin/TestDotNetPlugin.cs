using System;
using System.Diagnostics;
using System.Runtime.InteropServices;


[StructLayout(LayoutKind.Sequential)]
public struct LibArgs
{
    //[MarshalAs(UnmanagedType.LPWStr)] public string Message;
    public IntPtr Message;
    public int Number;
}

public static class User32
{
    public enum MB : uint
    {
        OK = 0x00000000
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public extern static int MessageBox(IntPtr hWnd, string text, string caption, MB options);
}

public class TestDotNetPlugin
{
    // UNMANAGEDCALLERSONLY_METHOD で呼べるようになる（ちょっぴり速い？）
    [UnmanagedCallersOnly]
    // struct lib_args { const char_t* message; int number; }; void (* custom) (lib_args args)
    public static void CustomEntryPointUnmanaged(LibArgs libArgs) => PrintLibArgs(libArgs);


    // coreclr_delegates.h に component_entry_point_fn として定義されている
    public static int EntryPoint(IntPtr arg, int _) => PrintLibArgs(Marshal.PtrToStructure<LibArgs>(arg));


    private static int PrintLibArgs(LibArgs libArgs)
        => User32.MessageBox(IntPtr.Zero, $"どおおおおおおとねっと！({libArgs.Number})", Marshal.PtrToStringAuto(libArgs.Message), User32.MB.OK);

}
