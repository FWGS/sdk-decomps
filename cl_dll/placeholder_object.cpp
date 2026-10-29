// Placeholder, not POV's code.
//
// POV's cl_dll.dsp compiled one more C++ source than this project: retail's
// Rich header counts 62 C++ objects (Utc12 8168) where the files POV's code
// shows give 61, and LINK sizes the PE header by the number of objects it
// loads (with one fewer, the PE header moves from 0xd8 to 0xe8). That
// source left no code and no data in client.dll (an empty object changes
// neither .text nor LINK's kept copies), so its name and contents can't be
// recovered; SDK 2.0's project has one more C++ file than ours,
// ..\dlls\wpn_shared\hl_wpn_glock.cpp, but nothing proves it was that one.
// This empty file stands in for it. It comes last in cl_dll.dsp, where the
// empty precompiled header it writes can't change the files compiled before
// it.
