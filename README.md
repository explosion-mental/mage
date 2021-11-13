# mage
iMAGE viewer


A re-write of sxiv which sucks less.


# TODO
- Thumbnail mode, requires to cache images to work 'smooth'
- Top status bar
- Order functions alphabetically
- Sort directory entries whether by giving `mage` a directory as an argument or by using `-r`
- Handle external commands


# Questions
- global variables outside of the `Image` struct like `zoomlvl` and `aa` (antialias flag) because they are applied 'globally', which means it doesn't store this for every image, like it does for the width, height, x, and y..
- mouse support? (cursors)
