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
