Interjector - C code instrumenting tool
---------------------------------------

Back around 2003 or so I had to do debugging on devices that did not
support any kind of debuggers, so printf debugging was the only way
to go. When things got hairy enough, I ended up writing this tool
that instruments source code completely automatically, so that when
(not if) the compiled program crashed, I could just check the log
to see where.

A few days ago I started porting some code that was unknown to me
over to emscripten, and found myself in a similar situation. I dusted
off this tool and hit the source code with it, and found the problems
rather quickly. I figured pushing this to github might help someone.

This is not a perfect tool by any means, and no guarantees that it
can parse your source code perfectly.

The logging is often too aggressive, and you'll find yourself editing
the instrumented code to remove logging from hotspots so that you don't
unnecessarily generate gigabytes of log files.

The basic idea is that code like this:

int foo(int bar)
{
  if (bar) return 7;
  return 3;
}

turns into:

int foo(int bar)
{
   INTERJECTOR_FUNCTION_ENTRY("foo");
   if (bar) { INTERJECTOR_FUNCTION_LEAVE("foo"); return 7; }
   { INTERJECTOR_FUNCTION_LEAVE("foo"); return 3; }
}

After that, you can just define the INTERJECTOR_FUNCTION_ENTRY and 
INTERJECTOR_FUNCTION_LEAVE macros to do whatever you want. There's 
an example interject.h that gives some idea what you can do with it.

Note that the code I interject is generally rather c-like but compiled
with a c++ compiler, so... your mileage may vary.

Original readme follows.

-- 8< -- 8< -- 8< --

Interjector 1.0
Copyright (c) 2007 Jari Komppa
http://iki.fi/sol/

This is a small tool to instrument C code with function entry/exit macros.
These macros can then be defined to perform different things, such as call
flow logging, performance analysis, and similar diagnostics. 

What you do with the macros is naturally up to you. 
A simple example interject.h is provided, though.

Earlier version of this tool was used successfully to instrument a very
large and complicated code base, so it should survive most of the things
thrown at it, but naturally there's no guarantee for this.

License (zlib/libpng)

Interjector - C code instrumenting tool
version 1.0, October 4th, 2007

Copyright (C) 2007 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
