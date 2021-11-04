# mage
iMAGE viewer


A re-write of sxiv which sucks less.


# TODO
- Get to a similiar but simpler (and only functional) state of sxiv
- Top status bar
- Man page
- Better variable/function names (specially for zoom)
- Simpler code
- `zoom` function to take zoom steps on the argument (positive or negative), if
  the arg it's NULL then use zoom_lvl. Should the zoom steps be infinite or the
  same size as the `zoom_lvl` or does this needs a separate variable to store
  this value?
- scalemode with `-s 'fit,down,zoom'`


# decisions
- global variables outside of the `Image` struct like `zoomlvl` and `aa` (antialias flag) because they are applied 'globally', which means it doesn't store this for every image, like it does for the width, height, x, and y..
