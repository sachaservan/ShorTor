package main

import (
	"database/sql"
	"os"
	"strings"

	"github.com/NullHypothesis/zoossh"
	_ "github.com/mattn/go-sqlite3"
)
//	"github.com/dmgawel/zoossh"

const dbSchema string = `CREATE TABLE nodes (
    id INTEGER PRIMARY KEY,
    fingerprint varchar
);

CREATE TABLE families (
    ida INT,
    idb INT,
    start_time DATETIME,
    end_time DATETIME
);

CREATE TABLE descriptors (
    node_id INT NOT NULL,
    nickname varchar(19) NOT NULL DEFAULT 'Unnamed',
    address varchar(32) NOT NULL,
    bandwidth_avg INT(12),
    bandwidth_burst INT(12),
    bandwidth_observed INT(12),
    platform varchar(20),
    tor_version varchar(20),
    hibernating BOOLEAN(19) NOT NULL DEFAULT 'false',
    exit_policy TEXT,
    start_time DATETIME NOT NULL,
	end_time DATETIME NOT NULL
);

CREATE TABLE geoip (
    ip varchar(32),
    country varchar(8),
    lat varchar(12),
    long varchar(12),
    as_number INTEGER,
    as_name varchar(128)
);`

type DbWrapper struct {
	DB         *sql.Tx
	node       chan zoossh.Fingerprint
	descriptor chan *DescriptorEntry
	family     chan *FamilyEntry
	geo        chan *GeoEntry
	done       chan bool
}

func DbSetupAndOpen(file string) (*DbWrapper, error) {
	// Remove existing databse
	if _, err := os.Stat(file); err == nil {
		err = os.Remove(file)
		if err != nil {
			return &DbWrapper{}, err
		}
	}

	// Open database connection
	db, err := sql.Open("sqlite3", file)
	if err != nil {
		return &DbWrapper{}, err
	}

	// Check if connection is OK
	err = db.Ping()
	if err != nil {
		return &DbWrapper{}, err
	}

	// Create tables
	_, err = db.Exec(dbSchema)
	if err != nil {
		return &DbWrapper{}, err
	}

	// Start transaction
	tx, err := db.Begin()
	if err != nil {
		return &DbWrapper{}, err
	}

	cn, cd, cf, cg, cdone := tableInserter(tx)

	return &DbWrapper{tx, cn, cd, cf, cg, cdone}, nil
}

func tableInserter(tx *sql.Tx) (chan zoossh.Fingerprint, chan *DescriptorEntry, chan *FamilyEntry, chan *GeoEntry, chan bool) {
	cNode := make(chan zoossh.Fingerprint)
	cDescriptor := make(chan *DescriptorEntry)
	cFamily := make(chan *FamilyEntry)
	cGeo := make(chan *GeoEntry)
	cDone := make(chan bool)

	stmtNode, err := tx.Prepare("INSERT INTO nodes(id, fingerprint) VALUES(?, ?)")
	if err != nil {
		fatal.Fatal(err)
	}

	stmtDescriptor, err := tx.Prepare("INSERT INTO descriptors(node_id, nickname, address, bandwidth_avg, bandwidth_burst, bandwidth_observed, platform, tor_version, hibernating, exit_policy, start_time, end_time) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
	if err != nil {
		fatal.Fatal(err)
	}

	stmtFamily, err := tx.Prepare("INSERT INTO families(ida, idb, start_time, end_time) VALUES(?, ?, ?, ?)")
	if err != nil {
		fatal.Fatal(err)
	}

	stmtGeo, err := tx.Prepare("INSERT INTO geoip(ip, country, lat, long, as_number, as_name) VALUES(?, ?, ?, ?, ?, ?)")
	if err != nil {
		fatal.Fatal(err)
	}

	go func() {
		for {
			err := error(nil)
			select {
			case n, ok := <-cNode:
				if ok {
					_, err = stmtNode.Exec(FingerprintToId(n), string(n))
				} else {
					cNode = nil
				}
			case d, ok := <-cDescriptor:
				if ok {
					rawExitPolicy := ""
					acceptlist := strings.Split(d.RawAccept," ")
					rejectlist := strings.Split(d.RawReject," ")
					for _,element := range acceptlist {
						rawExitPolicy = rawExitPolicy + "accept " + element + "\n"
					}
					for _,element := range rejectlist {
						rawExitPolicy = rawExitPolicy + "reject " + element + "\n"
					}
					_, err = stmtDescriptor.Exec(FingerprintToId(d.Fingerprint), d.Nickname, d.Address.String(), d.BandwidthAvg, d.BandwidthBurst, d.BandwidthObs, d.OperatingSystem, d.TorVersion, d.Hibernating, rawExitPolicy, d.Published, d.NextPublished)

					// Free memory
					d.Nickname = ""
					d.RawAccept = ""
					d.RawReject = ""
					d.OperatingSystem = ""
					d.TorVersion = ""
					d.Address = nil
					d.BandwidthAvg = 0
					d.BandwidthBurst = 0
					d.BandwidthObs = 0

				} else {
					cDescriptor = nil
				}
			case f, ok := <-cFamily:
				if ok {
					_, err = stmtFamily.Exec(f.ida, f.idb, f.start_time, f.end_time)
				} else {
					cFamily = nil
				}
			case g, ok := <-cGeo:
				if ok {
					_, err = stmtGeo.Exec(g.asn.IP.String(), g.geo.Country.IsoCode, g.geo.Location.Latitude, g.geo.Location.Longitude, g.asn.ASNumber, g.asn.ASName)
					g.asn = nil
					g.geo = nil
				} else {
					cGeo = nil
				}
			}

			if err != nil {
				fatal.Fatal(err)
			}

			if cNode == nil && cDescriptor == nil && cFamily == nil && cGeo == nil {
				tx.Commit()
				cDone <- true
				break
			}
		}
	}()

	return cNode, cDescriptor, cFamily, cGeo, cDone
}
