mage - image viewer
===================

mage is a simple image viewer. It should be very straightforward how it
works (thanks to Imlib2) and comes with useful configurable features.

**Wait for version 1.0**


Dependencies
------------

- libX11
- libImlib2
- fontconfig (status bar)


Installation
------------
Edit config.mk to match your local setup (mage is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install mage
(if necessary as root):

    make clean install


Running mage
------------
run

	mage [file]

See the manpage for further options.


# TODO
- custom (thumbnail) mode that has different layouts (e.g. grid)
- Top status bar
- Merge everything into mage.c and order functions alphabetically
- remove most of the function on cmd.c. They deserve a patch rather than to be
  in mainline
