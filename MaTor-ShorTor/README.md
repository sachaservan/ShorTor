# MaTor tool modified for ShorTor analysis

See also:
- original MaTor source code: https://github.com/sebastianmeiser/MATor
- updated MaTor source code with bug fixes: https://github.com/sachaservan/MATor

## Installation

### Setting up the database

#### Download the GeoLite2 City database from MaxMind
1. Create an account with [MaxMind](https://www.maxmind.com) and download the GeoLite2 City database.
2. Place the `GeoLite2-City.mmdb` file in the `/mator-db/` directory. 

#### Download and Build the Database Tools
1. Navigate to the `mator-db` directory: `cd mator-db`
2. Run `bash installdb.sh` to:
   - Set up the Go environment and directory structure
   - Build the `mator-db` tool from source
   - This step requires Go to be installed on your system

#### Download the Tor Consensus Data 
1. While still in the `mator-db` directory, run `bash download-year.sh`
2. When prompted, enter the year (e.g., 2014) for which you want to download consensus data
   - This will automatically download data for all 12 months of the specified year
   - The process may take a while depending on your connection speed

#### Process the Consensus Data
Navigate to `scripts/` and run:
1. `python download_consensus.py -sm START_MONTH -sy START_YEAR -em END_MONTH -ey END_YEAR`
   Example: `python download_consensus.py -sm 01 -sy 2014 -em 12 -ey 2014`
2. `python construct_database.py -sm START_MONTH -sy START_YEAR -em END_MONTH -ey END_YEAR`
   Example: `python construct_database.py -sm 04 -sy 2014 -em 04 -ey 2014`

**Important Note:** Ensure all data is downloaded and processed *before* compiling MaTor, as the compilation process (cmake) will move the data directory to `/build/Release/`.

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

