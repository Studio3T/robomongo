How to generate ICO file from the set of PNGs
=============================================

Install ImageMagick (http://www.imagemagick.org/). Make sure that you
have added its utils to your PATH (installer will give you such an option)

In `icon-set` directory run:

    > convert icon_256x256.png icon_128x128.png icon_64x64.png icon_32x32.png robomongo.ico
    
File `robomongo.ico` will be generated.
