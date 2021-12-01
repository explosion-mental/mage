mage - image viewer
===================

mage is an image viewer focus on simplicity. It can be easily be extensible and
achieve some of other image viewers features such as `sxiv` (e.g. the key
handler).


Dependencies
------------

- libX11
- libImlib2
- other libsl dependencies such as `fontconfig`


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
	mage [FILE or directory]

See the manpage for further options.


# TODO
- custom (thumbnail) mode that has different layouts (e.g. grid)
- Sort directory entries whether by giving `mage` a directory as an argument or
  by using `-r` (respecting LC)
- Top status bar
- Merge everything into mage.c and order functions alphabetically
