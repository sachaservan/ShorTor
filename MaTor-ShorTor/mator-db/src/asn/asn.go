package asn
//"errors"
import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"net"
	"strconv"
	"strings"
)

const (
	WhoisEndpoint string = "whois.cymru.com:43"
)

// Logger
var (
	logger *log.Logger
	fatal  *log.Logger
)

type ASNRecord struct {
	ASNumber uint
	ASName   string
	IP       net.IP
}

func prepareQuery(ips []net.IP) string {
	query := "begin\n"

	for ip := range ips {
		query += ips[ip].String() + "\n"
	}

	query += "end\n"

	return query
}

func makeQuery(query string) (string, error) {
	conn, err := net.Dial("tcp", WhoisEndpoint)
	if err != nil {
		return "", err
	}
	defer conn.Close()

	fmt.Fprintf(conn, query)

	buf, err := ioutil.ReadAll(conn)
	if err != nil {
		return "", err
	}

	return string(buf), nil
}

func parseResponse(rawResponse string) ([]*ASNRecord, error) {
	var (
		records []*ASNRecord
		lastIP  net.IP
	)
	logger = log.New(os.Stdout, "", 0)

	lines := strings.Split(rawResponse, "\n")
	for _, line := range lines[1 : len(lines)-1] {

		columns := strings.Split(line, "|")

		if len(columns) == 3 {
			for i := range columns {
				columns[i] = strings.TrimSpace(columns[i])
			}

			record := &ASNRecord{}
			record.IP = net.ParseIP(columns[1])

			if record.IP.Equal(lastIP) {
				continue
			} else {
				lastIP = record.IP
			}

			number, _ := strconv.ParseUint(columns[0], 10, 64)
			record.ASNumber = uint(number)
			record.ASName = string(columns[2])

			records = append(records, record)
		} else {
			logger.Printf("Error: Number of columns: %d, but it should be 3; the number of lines is %d",len(columns),len(lines))
		}

	}

	return records, nil
}
//return records, errors.New("Error while parsing whois response (bad results)")

func Lookup(ips []net.IP) ([]*ASNRecord, error) {
	var output []*ASNRecord

	if len(ips) > 0 {

		response, err := makeQuery(prepareQuery(ips))
		if err != nil {
			return output, err
		}

		output, err = parseResponse(response)
		if err != nil {
			return output, err
		}
	}

	return output, nil
}
