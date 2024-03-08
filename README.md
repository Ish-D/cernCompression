# ATLAS Project Google Summer of Code Compression Test

### Building
Requires C++20 compatible compiler, (for std::unordered_map.contains())
```
clang++ -std=c++20 main.cpp
./a.out
```

### Run length encoding
Sequences of length two or more are encoded with an escape sequence of that character and the number of occurrences. 
This naive implementation does not handle numbers, because there would be no way for the decompressor to distinguish numbers from the original file, and those inserted by the compression.<br>
For example:
  ```
   WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW ->  WW12BWW12BB3WW24BWW14
```

### Lempel–Ziv–Welch (LZW) Dictionary Encoding
The algorithm starts by initializing a dictionary with all one-character sequences. Then, as it goes through the remainder of the data, more codes are inserted into the dictionary as sequences of 
increasing length are encountered. If a sequence already within the dictionary is found, then only the dictionary code for that sequence is emitted. This implementation takes these codes and writes
them as chars into the output file.

### Results

**Input/brothers_k.txt <br>**
This file is 2.0mb of the full text of Fyodor Dostoevsky's 'The Brothers Karamazov', stripped of any numbers to be more compatible with this run-length encoding implementation.

| Algorithm | Compression Time | Decompression Time | Compression Ratio <br> (Compressed Size/Decompressed Size) | Peak Heap Consumption | Peak RSS |
|-----------|------------------|--------------------|-------------------|--------------------|-------------------|
| Run Length | 71ms | 68ms |1.01576 | 4.26Mb | 8.69Mb | 
| LZW | 860ms | 870ms | 0.432876 | 93.90Mb | 93.90Mb |

**Input/short_sequence.txt <br>**
This file contains the string "WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW".

| Algorithm | Compression Time | Decompression Time | Compression Ratio <br> (Compressed Size/Decompressed Size) | Peak Heap Consumption | Peak RSS |
|-----------|------------------|--------------------|-------------------|--------------------|-------------------|
| Run Length | 229ms | 122ms | 1.00542 |90.67Kb|4.59Mb|
| LZW | 713ms | 590ms | 0.158831 | 70.86Mb | 75.37Mb|

**Input/sequence.txt <br>**
This file is 2.5mb of a random sequence of the characters 'a' and 'b', generated with Linux's random sequence generation.

| Algorithm | Compression Time | Decompression Time | Compression Ratio <br> (Compressed Size/Decompressed Size) | Peak Heap Consumption | Peak RSS |
|-----------|------------------|--------------------|-------------------|--------------------|-------------------|
| Run Length | 0ms | 0ms | 0.313433 |6.82Mb|11.10Mb|
| LZW | 73ms | 55ms | 0.761194 |89.36Mb|94.81Mb|

**Input/random_data.txt <br>**
This file is 2.5mb of a random sequence of the characters 'a-z', and 'A-Z', generated with Linux's random sequence generation.

| Algorithm | Compression Time | Decompression Time | Compression Ratio <br> (Compressed Size/Decompressed Size) | Peak Heap Consumption | Peak RSS |
|-----------|------------------|--------------------|-------------------|--------------------|-------------------|
| Run Length | 100ms | 65ms | 1.01816 |6.86Mb|11.10Mb|
| LZW | 1945ms | 2410ms | 0.918471 |126.94Mb|138.26Mb|

**Input/repeated.txt <br>**
This file was generated to be favorable for run-length encoding, as it is essentially long repeated sequences of random repeated characters. The full file length was 1.3GB so it 
was too large to be uploaded to GitHub.

| Algorithm | Compression Time | Decompression Time | Compression Ratio <br> (Compressed Size/Decompressed Size) | Peak Heap Consumption | Peak RSS |
|-----------|------------------|--------------------|-------------------|--------------------|-------------------|
| Run Length | 21009ms | 8988ms | 0.0975164 | 3.41Gb | 2.52Gb |
| LZW | 297027ms | 59606ms | 0.0362995 |4.84Gb|4.00Gb|

### Analysis

While looking at this data, it is immediately apparent that the run length compressed data is often actually larger than the original/decompressed counterparts. This is because if we consider a sequence such as 'ABBAABBABBB', the runs of length
one will remain the same but the runs of length two will be transformed from 'AA' into 'AA2'. This adds a character over the original string and only breaks even for sequences of length three ('BBB' into 'BB3'). This is very apparent in the random
character tests sequence.txt and random_data.txt where the  runs are much more likely to be of length 1-2 than anything else, so the added characters for sequences of length two add up to increase the encoded file size.
<br><br>
We can also see that LZW proves considerably better than run length encoding in every test except for short_sequence.txt. This is because LZW starts with encoding shorter sequences, and then begins to encode longer ones. This means that for a file
as short as this one, there is less room for improvement than we see in the longer files. On the other hand, run length encoding always performs the same checking for runs of characters, so it can easily compress this sequence into something 
much shorter.
<br><br>
Where LZW falls behind is in speed and memory. While both files contain the entire file in a std::string (which could ideally be improved by reading the file in chunks), run length encoding always does a linear scan over the entire file, and only ever needs to store the length of the current run, and the corresponding character. 
LZW on the other hand has to maintain a dictionary of size $2^{bitWidth-1}$, where in this case bitWidth is 16. This is a considerable amount of additional memory overhead because we have to hold in memory the large unordered_map itself, as well as all of the strings that it contains by the end of the encoding or decoding sequence.
We can see this difference represented in the RSS and heap consumption values of each test, where in general the LZW will an order of magnitude more memory than the run length encoding. The notable exception to this is for the final test, where
the peak heap consumption is only 500mb higher, and the peak RSS 1.5GB higher. While the difference is still significant here, the difference is much smaller than that of the other tests. This is consistent with how LZW works in theory, where it should
get better at encoding the data in question the longer it runs for.
This is likely also the source of the algorithm's slower runtime compared to run-length encoding because there is lots of time associated with constantly searching and growing the values of the unordered_map. 

### Conclusion
In conclusion, for this test, I implemented LWZ and run length compression. We can see some very significant differences between the two compression methods as well as some positives and negatives to using either. I would say that
run-length encoding is only useful for particular types of data if we know that we will see long strings of repeated characters. LWZ is more general and can compress most files to a much larger degree, but it comes at a very noticeable
speed and memory overhead due to the nature of the implementation keeping an std::unordered_map of all of the compressed values in memory.
