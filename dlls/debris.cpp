// debris.cpp
//
// sic: a stand-in for a file the image cannot show. Beta 3.1's Linux library
// links an object debris.cpp, between deagle.cpp and doors.cpp, with no
// function and no data (CDebris itself is in ggrenade.cpp); Beta 4.0's has
// none. In the Windows project a source whose first declaration comes before
// any #include leaves the shared /YX .pch empty (it holds what precedes the
// first declaration: nothing), and every later compile reuses that .pch as it
// is, which gives exactly the object a compile without precompiled headers
// gives. That is the form of every Beta 3.0 and 3.1 object that shows its /YX
// state. A file of comments only, or an empty one, also
// leaves an empty .pch, but a compile that reuses it counts one declaration
// more (VC5's operand order), which retail's combat.obj
// (DotProduct), animating.obj and maprules.obj do not have. What the file held
// beyond that the image cannot say; this one declaration is the least that
// gives retail's objects.
class CDebris;
