mage - image viewer
===================

mage is a simple image viewer. It should be very straightforward how it
works (thanks to Imlib2) and comes with useful configurable features.


Dependencies
------------

- libX11
- libImlib2
- fontconfig (status bar)


Installation
------------
Please install the lastest **stable** version.
Edit config.mk to match your local setup (mage is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install mage
(if necessary as root):

    make clean install


Running mage
------------
run

	mage [file or directory]

See the manpage for further options.


# TODO
- custom (thumbnail) mode that has different layouts (e.g. grid)
- Sort directory entries whether by giving `mage` a directory as an argument or
  by using `-r` (respecting LC)
- Top status bar
- Merge everything into mage.c and order functions alphabetically
