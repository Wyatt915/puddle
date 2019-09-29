# Puddle
![](demo.gif)

## Build and run
    $ make && make install
    $ puddle
If your terminal does not support 256 colors, use

    $ make nocolor

## Options
```
Usage: ./puddle [flags]
	-d	Set the damping factor. A smaller damping factor
		means ripples die out faster. Default is 0.95.
	-i	Set the rainfall intensity. A higher intensity means
		more raindrops per second. Default is 75.
	-p	Select the color palette to be used. 0 for monochrome
		and 1 for blue. Default is 0.
	-h	Show this message and exit
```
