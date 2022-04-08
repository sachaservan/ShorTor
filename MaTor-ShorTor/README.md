# MaTor tool modified for ShorTor analysis

See also:
- original MaTor source code: https://github.com/sebastianmeiser/MATor
- updated MaTor source code with bug fixes: https://github.com/sachaservan/MATor

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

To compile and build the MaTor executable (tested on macOS and Ubuntu LTS 20.04): 

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

