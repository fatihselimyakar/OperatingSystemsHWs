**Part2**

-Because of the bubble sort and page replacement algorithms for part2, it works slowly for large inputs. I tried my experiments with commands like "./sortArrays 5 2 5 FIFO global 10000 diskFileName.dat", "./sortArrays 4 3 5 WSClock local 10000 diskFileName.dat" and "./sortArrays 5 3 5 LRU global 10000 diskFileName.dat". There is no problem in this way.

-On the other hand, local allocation policy slows down the program's operation because it limits the space used in physical memory. It is faster because it uses all of the global allocation policy physical memory.

-As can be seen from the output printed in the index sort section, it is sorted according to virtual memory addresses/indexes. Since there is no need to access physical memory or disk while doing this sequence, no page replacement, page fault, disk writes or disk reads occur. This is the reason for the 0's seen in the plots.

you can run like this:

$makefile
$./sortArrays 4 3 5 FIFO global 10000 diskFileName.dat

**Part3**

-I tried to find the most optimal size that can be applied for the part3 part. It was unfortunately very long in the form of 128k-16k (due to Bubble sort). Currently, 4k virtual memory size is set to 1k physical memory size per quarter. When it works like this, it takes about 2.5-3 minutes. You can already see that the slowdowns are in the bubble sort when it runs.

-The best frames it finds can vary from computer to computer. It found the optimum page size for quick sort 4 in the virtual machine and 8 in the original machine, with very little difference.

you can run like this:

$makefile
$./part3
