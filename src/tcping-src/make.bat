cls
del tcping.exe
del version.h

copy ..\tcping-32\version.h .

nmake


del version.h
del tcping.obj
del ws-util.obj
del main.obj

dir tcping.exe

echo "REMEMBER - Doublecheck that it still works statically in XP if VS2012 or above."
