External sort

1. 128GB dataset

Instance=i3.large -> 15.25GB memory
Number of partitions = 10000 
Each partition size = 12MB
Number of ASCII records in each file = 12MB/100Bytes = 120000 

   a) ./gensort -a 1280000000 inputfile_128GB.dat  //Generates 128GB dataset
   b) gcc externalSort.c -o sort -lpthread
   c) ./sort -t 2 -s 1

	<t> - Number threads
	<s> - Application type (128GB sort or 1TB sort) //'1' in this case

   d) ./valsort outputfile_128GB.dat

2. 1TB dataset

Instance=i3.4xlarge -> 122GB memory
Number of partitions = 10000
Each partition size = 100MB
Number of ASCII records in each file = 100MB/100Bytes = 1MB 

   a) ./gensort -a 10000000000 inputfile_1TB.dat //Generates 1TB dataset
   b) gcc externalSort.c -o sort -lpthread
   c) ./sort -t 10 -s 2

	<t> - Number threads
	<s> - Application type (128GB sort or 1TB sort) //'2' in this case

   d) ./valsort outputfile_1TB.dat





