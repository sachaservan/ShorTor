package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"os"
	"regexp"
	"runtime"
	"sync"
	"time"

	"github.com/NullHypothesis/zoossh"
	"github.com/docopt/docopt-go"
)

// Before we used: "github.com/dmgawel/zoossh"

const (
	DataDir           = "./"
	CollectorEndpoint = "https://collector.torproject.org/archive/relay-descriptors/server-descriptors/"
)

const Version string = "MATor Database Generator 0.1.0"
const Usage string = `MATor database generator.

Usage:
  mator-db <month> [options]

Options:
  -h, --help              Show this screen.
  --version               Show version.
  -q, --quiet             Quiet mode.
  -n, --no-clobber        Do not overwrite an existing database.
  -o FILE, --output FILE  Database output file path.
  -t DIR, --tmp-dir DIR   Specify directory for downloaded archives.
  -r, --redownload        Force redownloading of archives.

Arguments:
  <month>  Desired month for descriptors database in
           format: YYYY-MM`

// Globals
var (
	ids                map[zoossh.Fingerprint]int                        = make(map[zoossh.Fingerprint]int)
	nodes              map[zoossh.Fingerprint][]*zoossh.RouterDescriptor = make(map[zoossh.Fingerprint][]*zoossh.RouterDescriptor)
	complementaryNodes map[zoossh.Fingerprint][]*zoossh.RouterDescriptor = make(map[zoossh.Fingerprint][]*zoossh.RouterDescriptor)
	geoips             map[string]bool                                   = make(map[string]bool)
	ips                []net.IP
	GeoDB              *GeoReader
	mu                 sync.Mutex
)

// Logger
var (
	logger *log.Logger
	fatal  *log.Logger
)

func main() {
	// Make use of all procs
	runtime.GOMAXPROCS(runtime.NumCPU())

	// Get arguments
	arguments, _ := docopt.Parse(Usage, nil, true, Version, false)

	// Init loggers
	if arguments["--quiet"].(bool) {
		logger = log.New(ioutil.Discard, "", 0)
	} else {
		logger = log.New(os.Stdout, "", 0)
	}

	fatal = log.New(os.Stdout, "\033[1;31mFatal error: ", 0)

	// Validate month
	match, _ := regexp.MatchString("^\\d{4}-(0[1-9]|1[0-2])$", arguments["<month>"].(string))
	if !match {
		fmt.Println("Specified date is not in format YYYY-MM. Exiting now.")
		return
	}

	now := time.Now()

	month, err := time.Parse("2006-01", arguments["<month>"].(string))
	if err != nil {
		fmt.Println(err)
		return
	}
	if month.Year() < 2007 || (month.Year() >= now.Year() && month.Month() > now.Month()) {
		fmt.Println("Can't generate database for this month. Supported range: 2007-01 till current month.")
		return
	}

	// Setup database
	var dbPath string
	if arguments["--output"] != nil {
		dbPath = arguments["--output"].(string)
	} else {
		dbPath = fmt.Sprintf("./server-descriptors-%s.db", arguments["<month>"].(string))
	}
	logger.Println("Creating empty databse at", dbPath)

	// If db exists and --no-clobber flag provided
	if _, err := os.Stat(dbPath); err == nil && arguments["--no-clobber"].(bool) {
		logger.Println("Existing database found and --no-clobber flag set. Exiting now.")
		return
	}

	db, err := DbSetupAndOpen(dbPath)
	if err != nil {
		fatal.Fatal("Error while setting up database:", err)
	}
	logger.Println("Empty database created.")

	// Get necessary files
	var (
		pathMain    string
		pathComp    string
		pathsRecent []string
		wg          sync.WaitGroup
		tmpDir      string
	)

	if arguments["--tmp-dir"] != nil && arguments["--tmp-dir"].(string) != "" {
		tmpDir = arguments["--tmp-dir"].(string)
	} else {
		tmpDir = DataDir
	}

	if _, err := os.Stat(tmpDir); os.IsNotExist(err) {
		fatal.Fatal("Temporary directory ", tmpDir, " does not exists. Exiting now")
	}

	logger.Println("Getting all necessary descriptors...")
	wg.Add(2)
	go func() {
		pathMain, err = prepareDescriptorsArchive(month, tmpDir, "", arguments["--redownload"].(bool))
		if err != nil {
			fatal.Fatal(err)
		}
		wg.Done()
	}()
	go func() {
		pathComp, err = prepareDescriptorsArchive(month.AddDate(0, -1, 0), tmpDir, "", arguments["--redownload"].(bool))
		if err != nil {
			fatal.Fatal(err)
		}
		wg.Done()
	}()

	// Recent descriptors
	if month.Year() == now.Year() && month.Month() == now.Month() {
		wg.Add(1)
		go func() {
			pathsRecent, err = prepareRecentDescriptors(tmpDir, "", arguments["--redownload"].(bool))
			if err != nil {
				fatal.Fatal(err)
			}
			wg.Done()
		}()
	}

	wg.Wait()
	logger.Println("All descriptors prepared.")

	// Setup GeoIP databse
	geoPath := "./" + "GeoLite2-City.mmdb"
	logger.Println("Getting GeoIP database...")
	GeoReader, err := GeoOpen(geoPath)
	if err != nil {
		fatal.Fatal("Error while opening geo database:", err)
	}
	logger.Println("GeoIP databse prepared.")

	// Finally, parse it
	logger.Println("Now starting parsing...")
	ParseAndInsert(month, pathMain, pathComp, pathsRecent, db, GeoReader)
	logger.Println("I'm done. Bye.")
}
