# mage
iMAGE viewer


A re-write of sxiv which sucks less.


# TODO
- Thumbnail mode
- Top status bar
- Better variable/function names (specially for zoom)
- A patch that removes bar functionality (so no font loading are
  needed), which detaches mage from drw.c and drw.h and it's xft dep.
- Order functions alphabetically
- Sort directory entries whether by giving `mage` a directory as an argument or by using `-r`
- Handle external commands


# Questions
- global variables outside of the `Image` struct like `zoomlvl` and `aa` (antialias flag) because they are applied 'globally', which means it doesn't store this for every image, like it does for the width, height, x, and y..
- mouse support? (cursors)
