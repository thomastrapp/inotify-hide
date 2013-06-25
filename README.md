Inotify hide
====================

Copyright 2013 Thomas Trapp, http://www.thomastrapp.com

inotify-hide is free Software under the terms of the GNU General Public License
Version 2. See COPYING for full license text.

inotify-hide lets you hide a file from nosy eyes. inotify-hide does this by 
deleting the file (but keeping a handle to prevent the data from being freed)
whenever the parent directories get accessed. After a fixed time interval has
passed with no further access, the file is restored by coping its contents
back to the original location.

`Warning:` Don't use this on files you cannot afford to lose. In case of a system
crash (or -of course- an error on my part) your file will be lost!
I was just curious to see how well this idea works in practice.

Original idea by *Metlstorm* as demonstrated in his talk [Ain't No Party Like 
A Unix Party](http://lca2013.linux.org.au/schedule/30244/view_talk) at 
linux.conf.au in 2013. Download the video [here](http://mirror.linux.org.au/
linux.conf.au/2013/ogv/Aint_No_Party_Like_A_Unix_Party.ogv).

Detailed description:
---------------------
- Add an inotify watch for three immediate parent directories of the file if 
  available (excluding /)
- If one of those directories is accessed:
  - If there's already a child process: Tell it to wait some more
  - Else 
    - open the file, keep file descriptor, unlink the file
    - create a child process that waits a few seconds and then restores the
      file
- Handle termination gracefully

Usage:
-------
  Delete file on access, maybe restore:

    ./inotify-hide secretfile

  Hit `CTRL+C` to stop execution. If the file is currently hidden, it will be 
  restored.

Dependencies:
--------------
  Linux >= 2.6.13 (inotify)

Compilation:
-------------
  Create build directory:

    mkdir build
    cd build

  Run cmake:

    cmake ..

  Run make:

    make

  If all goes well there's now a binary called `inotify-hide` in the current 
  directory. 

  There's also a build script in `scripts/build.sh`, that creates a debug,
  release and a scan-build (clang static analyzer). Use it a your own risk.

