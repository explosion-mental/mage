# mage
iMAGE viewer


mage is a re-write of sxiv which goal is to be less complex in order for you to
hack on, which means patches! (which means features..)


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
