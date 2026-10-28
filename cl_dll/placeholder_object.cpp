// Placeholder, not Gunman's code.
//
// Gunman's cl_dll.dsp compiled one more C++ source than this project:
// retail's Rich header counts 78 C++ objects (Utc12 8168) where the files
// the client's code shows give 77, and LINK sizes the PE header by the
// number of objects it loads (0xe8 in retail, 0xd8 with one fewer). That
// source left no code and no data in client.dll, so its name and contents
// can't be recovered. This empty file stands in for it.
//
// Its place in the project comes from the precompiled header: a source with
// no #include writes an empty .pch that every later file reuses, so those
// files compile all their headers themselves and emit the headers' inline
// functions at the top of the object, and the inlines only a vtable needs
// right after the first function that needs them. Retail's
// vgui_CustomObjects.obj and the files before it show the other order (the
// headers in the .pch: inlines spread among the functions or last);
// vgui_HudID.obj, vgui_int.obj, vgui_MOTDWindow.obj,
// vgui_TeamFortressViewport.obj and vgui_teammenu.obj show this one. So the
// source came between vgui_CustomObjects.cpp and vgui_HudID.cpp
// (vgui_HudDeathNotice.cpp compiles the same either way).
