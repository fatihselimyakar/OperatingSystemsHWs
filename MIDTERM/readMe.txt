****PART1****

There is a design report for file system in Part_1.pdf

****PART2****

-Part2 compiles with "makefile"

-You can run with "makeFileSystem [BLOCK SIZE] [# OF INODES] [OUTPUT FILE]"
 If there is not an output file, then the program creates new one.

-If your entered # of inodes parameter is too much, then the program will warn you and show the maximum # of inode you can enter. 

-The program creates a 1MB file as empty file system disk.

-You can enter the 1KB, 2KB, 4KB, 8KB and 16KB as block size. But optimally it runs 4KB block size.

****PART3****

-Part3 compiles with "makefile"

-You can run with "./fileSystemOper [PART2'S OUTPUT FILE] [OPERATION] [PARAMETER 1] [PARAMETER 2]"

-If there is not file system disk(part2's output file), then it does not run.

-You can do these operations in file system: mkdir,rmdir, write, read, list, del, dumpe2fs.

CAUTION  : The input paths or name will be ASCII decoded strings. So it does not supports extended ASCII characters.
CAUTION2 : If the created files or directories have the same name and different paths, it may cause the program to run incorrectly.
CAUTION3 : File or directory names must not exceed 20 characters.
CAUTION4 : Addresses must not exceed 7 characters (adjusted by million).
CAUTION5 : Some operations may take up to 1 second.
CAUTION6 : the "list" command gets path with normal path+ '/'. For example to list the contents of directory 2 under directory 1, you must type "/directory1/directory2/" as path.
