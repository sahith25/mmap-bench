## Disk Information
```
Model :                         SAMSUNG MZNLN256
Interface                       SATA 3.1 6.0 Gbps
Density                         256 GB
Advertised Sequential Read      Up to 540 MB/s  
Advertised Sequential Write     Up to 280 MB/s
Advertised Random Read          Up to 97 KIOPS
Advertised Random Write         Up to 74 KIOPS
```

## Reading Benchmark Setup

The benchmark takes as input a file, and divides it into `NUM_B` blocks of size `B_SIZE`. In each experiment, all the blocks are read the same number of times. Only the order they are read in differs (sequential or random). We calculate the total time it takes to completely read through / fetch all the blocks, and use that to calculate how many blocks it could read per second.

While iterating over the blocks (in whichever order), we maintain a sum of the values of the first byte in each block in a `bogus` variable. The two pseudo-code for the two benchmarked methods are :

#### `read`
```
for block number `b` in block_ordering:     

    lseek(file, b * B_SIZE, SEEK_SET);      // seek block `b`
    int nr = read(fd, buf, B_SIZE);         // read B_SIZE bytes into `buf`
    bogus += buf[0];                        // add first byte to `bogus`
```

#### `mmap`
```
data = mmap(NUM_B * B_SIZE, PROT_READ, MAP_SHARED, file, 0);

for block number `b` in block_ordering:

    bogus += data[b * B_SIZE];              // add first byte of block `b` to `bogus`

munmap(data, NUM_B*B_SIZE);
```

## Results & Observations

Have used `B_SIZE = 4KB` and a single large file of size 2GB. Which makes `NUM_B = 524288`. The results (averaged over 3 runs) :

```
                                        Rates in KIOPS                      
                                    Sequential      Random
 
            read                    130.92          14.39
            mmap                    130.79          71.33
Method      mmap with adv_random    42.23           9.89
            mmap with adv_seq       129.96          57.21
```

#### Sequential

For sequential block access, `read` and `mmap` have comparable performance as expected. `mmap` with the wrong advice of random, does poorly as expected.

`mmap` with sequential advice should have performed better than standalone `mmap`. Also indicated [here](http://stackoverflow.com/a/2895799). The possible reason that there is no observable speedup is that, the processing we are doing with the actual read data from a block is minimal, hence there is no real time between two different page accesses. Thus pre-faulting pages cannot really occur (or provides no benefits).

#### Random

As we see `read` performs poorly, whereas `mmap` does a much better job. However, these values do not agree much with the advertised value (in contrast to sequential reading, where the advertised and benchamrked values are close).

I have **no clue** as to what is happening in the case where we are advising the kernel. Apparently, the perforance drops (even lower than `read`) when we advise the kernel that the data access is going to be random. An advise of sequential read performs better, which it should not have. Maybe I am doing [this](https://github.com/srajangarg/mmap-bench/blob/master/read.c#L109) wrong?

## Future Work

- Figure out how the internals of `madvice` work, to understand the benchmark results
- Experiment with other `madvice` options like `MADV_HUGEPAGE`
- Experiment with other `mmap` options like `MAP_HUGETLB` and `MAP_POPULATE`
- Perform similar benchmarks for write operations, and mixed operations
