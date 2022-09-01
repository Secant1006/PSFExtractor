# PSFExtractor
Extract patch storage file for Windows 2000+.

Usage: PSFExtractor.exe &lt;CABFile&gt;

PSFExtractor.exe -v[N] &lt;PSFFile&gt; &lt;description file&gt; &lt;destination&gt;

  You need to put CAB file alongside with its corresponding PSF file. After that you'll get a folder which contains extracted full update. You can either use it with dism /online /add-package directly or repack the files inside the folder into a CAB file.

The tool requires Windows 2000 or above.

Support any PSF file with PSM/XML description file.

A Chinese version of usage can be found on https://blog.betaworld.cn/32.
