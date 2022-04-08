package main

import (
	"archive/tar"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net"
	"os"
	"sort"
	"strings"
	"sync"
	"time"

	"mator-db/asn"

	"github.com/NullHypothesis/zoossh"
	"github.com/xi2/xz"
)

//	"github.com/dmgawel/zoossh"

type TimelineEntry struct {
	time  time.Time
	start bool
}

type FamilyEntry struct {
	ida        int
	idb        int
	start_time time.Time
	end_time   time.Time
}

type DescriptorEntry struct {
	*zoossh.RouterDescriptor
	NextPublished time.Time
}

type GeoEntry struct {
	geo *GeoRow
	asn *asn.ASNRecord
	IP  net.IP
}

func checkErr(err error) {
	if err != nil {
		panic(err)
	}
}

func daysIn(m time.Month, year int) int {
	// This is equivalent to time.daysIn(m, year).
	return time.Date(year, m+1, 0, 0, 0, 0, 0, time.UTC).Day()
}

func FingerprintToId(fingerprint zoossh.Fingerprint) int {
	mu.Lock()
	id := ids[fingerprint]
	mu.Unlock()
	return id
}

// extractDescriptor extracts the first server descriptor from the given string
// blurb.  If successful, it returns the descriptor as a string and true or
// false, depending on if it extracted the last descriptor in the string blurb
// or not.
// Note: copied from zooshh
func extractDescriptor(blurb string) (string, bool, error) {

	start := strings.Index(blurb, "\nrouter ")
	if start == -1 {
		return "", false, fmt.Errorf("Cannot find beginning of descriptor: \"\\nrouter \"")
	}

	marker := "\n-----END SIGNATURE-----\n"
	end := strings.Index(blurb[start:], marker)
	if end == -1 {
		return "", false, fmt.Errorf("Cannot find end of descriptor: \"\\n-----END SIGNATURE-----\\n\"")
	}

	// Are we at the end?
	done := false
	if len(blurb) == (start + end + len(marker)) {
		done = true
	}

	return blurb[start : start+end+len(marker)], done, nil
}

// Dissects the given file into string chunks by using the given string
// extraction function.  The resulting string chunks are then written to the
// given queue where the receiving end parses them.
func dissectFile(rawContent string, extractor zoossh.StringExtractor, queue chan zoossh.QueueUnit) {

	defer close(queue)

	for {
		unit, done, err := extractor(rawContent)
		if err != nil {
			log.Println("Error in extraction function: ", err)
			break
		}

		queue <- zoossh.QueueUnit{unit, nil}
		rawContent = rawContent[len(unit):]

		if done {
			break
		}
	}
}

func parseRecent(filename string) chan *zoossh.RouterDescriptor {
	// Make channel for sending parsed descriptors
	c := make(chan *zoossh.RouterDescriptor)

	// Open file for read
	f, err := os.Open(filename)
	if err != nil {
		fatal.Fatal(err)
	}

	// Read contents
	buf, err := ioutil.ReadAll(f)
	if err != nil {
		fatal.Fatal(err)
	}

	// Close file
	f.Close()

	fd := string(buf)

	go func() {
		// We will read raw router descriptors from this channel.
		queue := make(chan zoossh.QueueUnit)
		go dissectFile(fd, extractDescriptor, queue)

		// Parse incoming descriptors until the channel is closed by the remote
		// end.
		for unit := range queue {
			if unit.Err != nil {
				continue // Fail silently
			}

			_, getDescriptor, err := zoossh.ParseRawDescriptor(unit.Blurb)
			if err != nil {
				continue // Fail silently
			}

			c <- getDescriptor()
		}

		close(c)
	}()

	return c
}

func parseArchive(filename string) chan *zoossh.RouterDescriptor {
	// Make channel for sending parsed descriptors
	c := make(chan *zoossh.RouterDescriptor)

	// Open archive with server descriptors
	f, err := os.Open(filename)
	if err != nil {
		fatal.Fatal(err)
	}

	// Create a xz Reader
	r, err := xz.NewReader(f, 0)
	if err != nil {
		fatal.Fatal(err)
	}

	// Create a tar Reader
	tr := tar.NewReader(r)

	// Parse archive
	go func() {
		defer f.Close()
		for {
			// Read next file in tar
			hdr, err := tr.Next()
			if err == io.EOF {
				// end of tar archive
				break
			}
			if err != nil {
				fatal.Fatal(err)
			}

			// Is it a regular file?
			if hdr.Typeflag == tar.TypeReg {
				// Read file contents
				buf, err := ioutil.ReadAll(tr)
				if err != nil {
					fatal.Fatal(err)
				}

				_, getDescriptor, err := zoossh.ParseRawDescriptor(string(buf))
				recover()
				if err != nil {
					continue
				}

				c <- getDescriptor()
			}
		}
		close(c)
	}()

	// Return channel
	return c
}

func ParseAndInsert(month time.Time, mainFile string, complementaryFile string, recentFiles []string, db *DbWrapper, geo *GeoReader) {

	lastID := 1

	var (
		wg sync.WaitGroup
	)

	logger.Println("Retrieving descriptors from archives.")

	// Parse newest complementary descriptors
	wg.Add(1)
	go func() {
		defer wg.Done()

		complementaryDescriptors := parseArchive(complementaryFile)
		for descriptor := range complementaryDescriptors {
			if len(complementaryNodes[descriptor.Fingerprint]) == 0 {
				complementaryNodes[descriptor.Fingerprint] = append(complementaryNodes[descriptor.Fingerprint], descriptor)
			} else if descriptor.Published.After(complementaryNodes[descriptor.Fingerprint][0].Published) {
				complementaryNodes[descriptor.Fingerprint][0] = descriptor
			}
		}
		complementaryDescriptors = nil
	}()

	// Parse all main descriptors
	wg.Add(1)
	go func() {
		defer wg.Done()

		if len(mainFile) > 0 {
			mainDescriptors := parseArchive(mainFile)
			for descriptor := range mainDescriptors {
				nodes[descriptor.Fingerprint] = append(nodes[descriptor.Fingerprint], descriptor)
			}
			mainDescriptors = nil
		}
	}()

	wg.Wait()

	// Merge descriptors
	for keyNode := range complementaryNodes {
		nodes[complementaryNodes[keyNode][0].Fingerprint] = append(nodes[complementaryNodes[keyNode][0].Fingerprint], complementaryNodes[keyNode][0])
	}
	// Free it
	complementaryNodes = make(map[zoossh.Fingerprint][]*zoossh.RouterDescriptor)

	// Parse recent descriptors
	var (
		mutex = &sync.Mutex{}
	)
	for i := range recentFiles {
		wg.Add(1)
		go func() {
			defer wg.Done()
			recent := parseRecent(recentFiles[i])
			for descriptor := range recent {
				mutex.Lock()
				nodes[descriptor.Fingerprint] = append(nodes[descriptor.Fingerprint], descriptor)
				mutex.Unlock()
			}
			recent = nil
		}()
	}
	wg.Wait()

	logger.Println("All descriptors parsed. Processing now.")

	logger.Println("Sorting, assigning IDs, getting unique IPs")

	// Prepare descriptors
	for keyNode := range nodes {

		mu.Lock()
		ids[keyNode] = lastID
		mu.Unlock()
		lastID++
		db.node <- keyNode

		// Sort descriptors
		sort.Sort(descriptorsByTime(nodes[keyNode]))

		// Make first descriptor fresher (publish it one day before)
		tmp := nodes[keyNode][0].Published
		loci := tmp.Location()
		myyear, mymonth, myday := tmp.Date()
		myhour, mymin, mysec := tmp.Clock()
		//nodes[keyNode][0].Published = nodes[keyNode][0].Published.AddDate(0, 0, -1)
		// Trying to fix a weird bug in AddDate
		nodes[keyNode][0].Published = time.Date(myyear, mymonth, myday-1, myhour, mymin, mysec, 0, loci)

		loci = tmp.Location()

		myyear, mymonth, myday = tmp.Date()
		myhour, mymin, mysec = tmp.Clock()

		//nodes[keyNode][0].Published = nodes[keyNode][0].Published.AddDate(0, 0, -1)
		// Trying to fix a weird bug in AddDate
		nodes[keyNode][0].Published = time.Date(myyear, mymonth, myday-1, myhour, mymin, mysec, 0, loci)
		l := len(nodes[keyNode])
		for keyDescriptor := range nodes[keyNode] {

			// Get unique IPs
			if _, exists := geoips[nodes[keyNode][keyDescriptor].Address.String()]; exists == false {
				geoips[nodes[keyNode][keyDescriptor].Address.String()] = true
				ips = append(ips, nodes[keyNode][keyDescriptor].Address)
			}

			// Insert descriptor into database
			var d *DescriptorEntry
			if keyDescriptor < l-1 {
				d = &DescriptorEntry{nodes[keyNode][keyDescriptor], nodes[keyNode][keyDescriptor+1].Published}
			} else {
				t, _ := time.Parse("2006-01-02 15:04:05", fmt.Sprintf("%d-%02d-%02d 23:59:59", month.Year(), month.Month(), daysIn(month.Month(), month.Year())))
				d = &DescriptorEntry{nodes[keyNode][keyDescriptor], t}
			}
			db.descriptor <- d
		}
	}

	logger.Println("Making families. Getting Geo data.")

	wg.Add(1)
	go func() {
		defer wg.Done()

		// Process descriptors
		for n := range nodes {

			// Process node descriptors
			for d := range nodes[n] {

				// Make families
				for declaration := range nodes[n][d].Family {

					timeline := []TimelineEntry{}
					timeline = append(timeline, TimelineEntry{nodes[n][d].Published, true})

					for dd := range nodes[n][d:] {
						if nodes[n][dd].Family[declaration] {
							delete(nodes[n][dd].Family, declaration)
						} else {
							timeline = append(timeline, TimelineEntry{nodes[n][dd].Published, false})
						}
					}

					// Check if interval is closed
					if timeline[len(timeline)-1].start {
						t, _ := time.Parse("2006-01-02 15:04:05", fmt.Sprintf("%d-%02d-%02d 23:59:59", month.Year(), month.Month(), daysIn(month.Month(), month.Year())))
						timeline = append(timeline, TimelineEntry{t, false})
					}

					// Get timeline for declared node
					started := false
					for dd := range nodes[declaration] {
						if nodes[declaration][dd].Family[nodes[n][d].Fingerprint] {
							if !started {
								started = !started
								timeline = append(timeline, TimelineEntry{nodes[declaration][dd].Published, started})
							}
						} else {
							if started {
								started = !started
								timeline = append(timeline, TimelineEntry{nodes[declaration][dd].Published, started})
							}
						}

						delete(nodes[declaration][dd].Family, nodes[n][d].Fingerprint)
					}

					// Check if last interval is closed
					if timeline[len(timeline)-1].start {
						t, _ := time.Parse("2006-01-02 15:04:05", fmt.Sprintf("%d-%02d-%02d 23:59:59", month.Year(), month.Month(), daysIn(month.Month(), month.Year())))
						timeline = append(timeline, TimelineEntry{t, false})
					}

					// Sort timeline entries
					sort.Sort(timelineByTime(timeline))

					// Get intersection of intervals
					counter := 0
					var startTime time.Time
					for t := range timeline {
						if timeline[t].start {
							counter++
							if counter == 2 {
								startTime = timeline[t].time
							}
						} else {
							if counter == 2 {
								db.family <- &FamilyEntry{ids[nodes[n][d].Fingerprint], ids[declaration], startTime, timeline[t].time}
							}
							counter--
						}
					}
				}

				nodes[n][d].Family = make(map[zoossh.Fingerprint]bool)
			}
		}
		close(db.family)
		close(db.node)
		close(db.descriptor)

		nodes = nil
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		// Process IPs (get Geo and ASN information)
		asns, err := asn.Lookup(ips)
		if err != nil {
			fatal.Fatal(err)
		}

		for a := range asns {
			res, err := geo.Lookup(asns[a].IP)
			if err != nil {
				fatal.Fatal(err)
			}

			db.geo <- &GeoEntry{
				geo: res,
				asn: asns[a],
			}

		}

		close(db.geo)
	}()

	wg.Wait()
	logger.Println("Descriptors processed. Waiting for database to complete queries...")
	<-db.done
}
