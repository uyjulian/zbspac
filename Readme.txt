ZBSPAC Update: Milestone 2
March 29th, 2010

Added (un)packing support for PAC Variant 1, the one used in Baldr Force EXE.
Added support for script text extraction.
Added experimental support for script text repacking. (There are still encoding issues.)

--------------------------------------------

ZBSPAC README (or Q & A) [encoding: UTF-8]

February 20th, 2010 
-------------------------

zbspac, a resource (un)packer for Baldr Sky.
Copyright 2010, CloudiDust (cloudidust@gmail.com)

Covered by 2-clause BSD license, please refer to License.txt.
Source available at http://code.google.com/p/zbspac.

-------------------------
Q: What is zbspac?

zbspac is a resource file (un)packer for Giga's 2009 Game Baldr Sky,
a Cyberpunk Visual Novel / 2D Mecha ACT hybrid which is a wonderful piece
of work.

-------------------------
Q: How is zbspac?

zbspac can unpack the PAC files that come with the game, and pack any file
with a name shorter than 64 bytes (when encoded in Shift-JIS) into a PAC
file. Some tests are done and the generated PAC files aren't crashing the
game, but if you encounter any problems, please let me know.

zbspac may work with PAC files used in other recent GIGA games, but I do
not have those games for testing. Thus if you do so, you are on your own.

-------------------------
Q: How to use zbspac?

zbspac runs in your command prompt.
Put the compiled binary zbspac.exe anywhere you want,
and use the command 'zbspac help' to obtain the help page.

You may also refer to Instructions.txt.

-------------------------
Q: Who inspired zbspac?

The analysis of the resource package format (*.pac) is mainly done by the
two guys following, independently:

痴汉公贼(jzhang0): The author of crass, a resource extractor that supports
numerous games though plugins. He has withdrawn from the reverse engineering
field and vanished into the linux kernel source codes.

asmodean: The author of exchpac, who is actively updating his resource
extractors for various games at http://asmodean.reverse.net

When jzhang0 left, he opened the full source of crass. (Though he didn't
explicitly cover it under any license.) And asmodean also offers part of his
source codes in his extractor distributions. 

I learned about the package format mainly from the source codes, but zbspac
itself is built from ground up.

Hats off to you both!

-------------------------
Q: So it seems that others have already done some unpackers and you owe a lot
to them, right? Then the real question: Why is zbspac?

Well, I just want to translate the game into Chinese (or at least grant myself
such a possibility, as the game may just be too long for a single person to
translate, and my Japanese is at "ABC" level now. ;), so I need a resource
packer / unpacker for it. AFAIK, there are two unpackers floating around in the
"cyberspace" (you know, one that is not as exciting as the one in the game), 
but they either cannot unpack some of the packages (crass) or cannot handle
Shift-JIS correctly in my Simplified Chinese system (exchpac). And there are
no packers available, so I decided to have a try. Meanwhile, it is a good
exercise for me to familiarize myself with locale and bit operations in C.

-------------------------
Q: How to compile zbspac?

zbspac is written in ANSI C99('s syntax) using MinGW with Eclipse/CDT and
depends on zlib and Windows C Runtime (Mainly for file system manipulations).

Please make sure you are running windows and have MinGW and zlib installed.

Then there is a makefile for mingw32-make. Just 'make' will do the trick. 
And if you have 7za.exe in your path, you can make source and binary distros
with "make src_dist" and "make bin_dist". As always, "make clean" will clean
up for you.

I am using nuwen's MinGW distro that includes zlib and 7za and works out of
box. It can be downloaded from http://nuwen.net/mingw.html.

You may also import the project into CDT.
(I am using Eclipse 3.5.1 / CDT 6.0.0.)
 