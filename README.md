# hash_table_service

To run
- compile  `make`
- run server  `./server N`
    - N - being number of buckets
- run client  `./client`

Measurements  - 50 buckets
- 2 threads - 754015.320083 requests/second
- 4 threads - 845871.975585 requests/second
- 8 threads - 527418.927800 requests/seconds
- 1 thread -  714872.420294 requests/seconds


Measurements  - 2 buckets -  more collision - more traversal to linked lists
- 2 threads - 709028.772388 requests/second
- 4 threads - 815088.926202 requests/second
- 8 threads - 521789.131445 requests/seconds
- 1 thread -  796341.289579 requests/seconds
