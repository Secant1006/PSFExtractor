# PSFExtractor
Extract CAB+PSF update for Windows 10/11.

Usage: PSFExtractor.exe &lt;CABFILE&gt;

  You need to put CAB file alongside with its corresponding PSF file. After that you'll get a folder which contains extracted full update. You can either use it with dism /online /add-package directly or repack the files inside the folder into a CAB file.

The tool requires .NET Framework 2.0 - 4.8 and Windows 7 or above.

Supports any CU updates from 21382.1000 (KB5003837). Windows 11 updates are also supported.

A Chinese version of usage can be found on https://blog.betaworld.cn/32.
