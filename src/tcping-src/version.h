// Ok.  "version.h" is in the specific project file instead of in the generic tcping-src, so I can have a compiled_in
// value for what bit version it is (in case it gets renamed or whatever.   Had to add...
// $(MSBuildProjectDirectory);
// .. into the include path so visual studio doesn't get confused.

const int compiled_bitversion = 32;