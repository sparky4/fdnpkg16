# The actual FreeDOS package is the file fdnpkg16.zip
If you dont know how to install it in your FreeDOS system. here is the steps
use mtcp's htget to get it (works on 16 bit systems!)


```
htget -o fdnpkg16.zip http://4ch.su/freedos/fdnpkg16.zip
fdinst install fdnpkg16.zip
```


## Thats it! then you have it!

Just make sure you configure the repository system in fdnpkg16.cfg in the bin folder.

The program and config file can be placed anywhere. As long as they are together in same location! It supports that now!


### The config files included is

fdnpkg16.cfg for ibiblio repository

fdnpkg16.oem for my repository mirror with fdnpkg16 added

fdnpkg16.4 is just my config file for specifically my FreeDOS installs on my computers!

If u got a very old (1.0) FreeDOS install with all the stuff in fdos dir then use this one!


# The program itself is compiled with Open Watcom 2.0
watt32 is compiles with Open Watcom 1.9

zlib can be compiled with either! :D


# Stability report so far
Currently at 0.99.8252b we are pretty stable now with many Quality of Life features! (I cannot see any bugs.)

I am awaiting for fdrepo to be updated for fdnpkg16 then i will implement stable features and test it.
