How to generate ICO file from the set of PNGs
=============================================

Install ImageMagick (http://www.imagemagick.org/). Make sure that you
have added its utils to your PATH (installer will give you such an option)

In `icon-set` directory run:

    > convert 256x256.png 128x128.png 64x64.png 48x48.png 40x40.png 32x32.png 24x24.png 20x20.png 16x16.png robomongo.ico
    
File `robomongo.ico` will be generated.


Which icon sizes are used on Windows?
=====================================

The most important sizes are the one that is small. Robo 3T should include 
16, 20, 24, 40, 48. The bigger sizes are usually scaled well, without notable
degrading in quality. 

The following statistic was provided in this answer:
http://stackoverflow.com/a/12283978/407599

> After some testing with an icon with 8, 16, 20, 24, 32, 40, 48, 64, 96, 128 
> and 256 pixels (256 in PNG) in Windows 7:
>
> At 100% resolution: 
>  Explorer uses 16, 40, 48, and 256. Windows Photo Viewer uses 96. Paint uses 256.
>  
> At 125% resolution: 
>  Explorer uses 20, 40, and 256. Windows Photo Viewer uses 96. Paint uses 256.
>  
> At 150% resolution: 
>  Explorer uses 24, 48, and 256. Windows Photo Viewer uses 96. Paint uses 256.
>  
> At 200% resolution: 
>  Explorer uses 40, 64, 96, and 256. Windows Photo Viewer uses 128. Paint uses 256.
>
> So 8, 32 were never used (it's strange to me for 32) and 128 only by Windows Photo 
>  Viewer with a very high dpi screen, i.e. almost never used.
> 
> It means your icon should at least provide 16, 48 and 256 for Windows 7. For supporting 
> newer screens with high resolutions, you should provide 16, 20, 24, 40, 48, 64, 96, and 256. 
> For Windows 7, all pictures can be compressed using PNG but for backward compatibility with 
> Windows XP, 16 to 48 should not be compressed.

