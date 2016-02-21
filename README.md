Inotify Hide
============

inotify-hide deletes a file whenever a parent directory is accessed, but keeps
a handle to the file in order to restore it after some time has passed,
effectively hiding the file.

Original idea by *Metlstorm* as demonstrated in his talk [Ain't No Party Like
 A Unix Party](http://lca2013.linux.org.au/schedule/30244/view_talk) at
linux.conf.au in 2013. Download the video
[here](http://mirror.linux.org.au/linux.conf.au/2013/ogv/Aint_No_Party_Like_A_Unix_Party.ogv).

Copyright 2013 Thomas Trapp, http://www.thomastrapp.com

inotify-hide is free Software under the terms of the GNU General Public License
Version 2. See COPYING for full license text.

How it works:
-------------
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

