# MaTor(s) Tool
This is the latest version of the MaTor tool used for generating the plots of the [Your Choice MaTor(s) paper](https://petsymposium.org/2016/files/papers/Your_Choice_MATor(s).pdf). 

## Installation

### Setting up the database

#### Download the GeoLite2 City database from MaxMind
1. Create an account with [MaxMind](https://www.maxmind.com) and download the GeoLite2 City database.
2. Place the ```GeoLite2-City.mmdb``` file in the ```/mator-db/``` directory. 

#### Download the Tor data 
1. Run ```cd mator-db``` and run ```bash install-db.sh``` (this may take a while). 
2. Run ```bash download-year.sh``` and enter the year (e.g., 2014) you would like to download consensus data for. 
Note: step 2 will take a while. 

Navigate to ```scripts/``` and run:
1. ```python download_consensus.py -sm STAR_MONTH -sy START_YEAR -em END_MONTH -ey END_YEAR``` (e.g., ```python download_consensus.py -sm 01 -sy 2014 -em 12 -ey 2014```). 
2. ```python construct_database.py -sm STAR_MONTH -sy START_YEAR -em END_MONTH -ey END_YEAR``` (e.g., ```python construct_database.py -sm 04 -em 04 -sy 2014 -ey 2014```).

Steps 1 & 2 will populate the ```/data``` directory with the necessary Tor consensus data. 
*NOTE:* You must have all the necessary data ready *before* you compile MaTor since the compilation process (cmake) will move the data directory to ```/build/Release/```. 

### Compiling MaTor 

To compile and build the MaTor executable (tested on macOS): 

1. Install dependencies: 
	- boost
	- sqlite3
	- glpk

2. To enable optimization type: 
	  ```export CXXFLAGS=-O2```

3. go to the build directory: 
    ```
    cd build
    cd [your_path]/MATor/build
    cmake ..
    make
    ```

At this point the ```runtest``` executable should be located in ```[your_path]/MATor/build/Release/bin/```

### Running the plot analysis code
1. ```cd [your_path]/MATor/scipts```
2. ```python worklist-example.py``` or ```python worklist-example-complex.py```

### Publications
Links to the academic publications relevant for MATor:
- The framework: [AnoA framework (Journal Version)](https://pdfs.semanticscholar.org/2327/8ffba48f5b4e3aed479497cb6dcfbad80cc3.pdf)
- Introduction of MATor [Nothing else MATor(s) (CCS'14)](https://dl.acm.org/citation.cfm?id=2660371)
- Extension of MATor (as in this repo) [Your Choice MATor(s) (PETS'16)](https://www.degruyter.com/downloadpdf/j/popets.2015.2016.issue-2/popets-2016-0004/popets-2016-0004.xml)
- Comprehensive, self-contained, and improved writeup of AnoA and MATor [Quantitative Anonymity Guarantees for Tor (PhD Thesis)](https://publikationen.sulb.uni-saarland.de/handle/20.500.11880/26754)


