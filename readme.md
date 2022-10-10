properties of build scripts:
- one of the _most run_ programs on your computer
: should be very fast to boot up/run/shut down
- one of the most _changed/rebuilt_ programs on your computer over time (except for the program it's currently building, of course)
: should be very fast to rebuild
- don't want to struggle to configure/build (that's _its_ job!)
: should build and run with essentially NO dependencies
- almost always has trivial flow graph
- - a single, sequential trunk. maybe a few branches that return to the trunk, maybe a few forks that don't. few structural loops
: "memory leaks" are not an important threat
- _not_ a situation where "types" often change minute to minute throughout development (though I'm unsure what situations _do_ express that...)
: no reason to not be strongly typed
- structural simplicity works well with simple, flat API
: no need for classes, but _also_ does not call for the API being "everything is a string"
- string parsing often important
: totally achievable with few "language features"
- if you're working on a build script, you should have strong programming foundations
: need not prioritize ease of use specifically for non-programmers
- repl valuable
: ok this one hurts...

meta build system:
- the goal is to be able to run a global "csbuild.exe":
- - either compile, or refer to an already-compiled cscript_platform.c as a library
- - find local script, build.c
- - compile build.c with cscript_platform.c as a library (and automatically find cscript.h as include?), using env BBCC (fallback CC), into _local_ build.exe
- - csbuild.exe builds your project
- build.c builds local project files (such as cscript_test) (currently a batch file, until cscript mature enough to self-host)
- buildbuild aims to build global csbuild.exe (to be distributed)
- buildbuildbuild builds buildbuild (a cscript file); breaks the self-hosting loop by falling back to a 1-or-so line batch file

todo:
x separate win/mac/linux impls
x env variable modification
- pipe multiple programs
- simple read/write config file api
- testing coverage
- global "silence" flag (delete/copy/etc... don't log)
- global "merely try" flag (delete/copy/etc... won't crash on fnf)
- pointer hashmap-based "push"/"pop" values (for "silence"/"try" globals, and others)
- buildbuild (abbr "bb")
- - looks for build.exe; if exists (and newer than build.c), runs/exits
- - looks for build.c
- - prepends contents of cscript.h
- - compiles and links with compiled contents of cscript.c
- - uses env BBCC as compiler, falls back to CC
- - runs
- buildbuildbuild
- - batch file that builds buildbuild
- - compile contents of cscript.c for current arch
- - somehow includes it in buildbuild executable for easy linking
