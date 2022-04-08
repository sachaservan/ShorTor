package main

import (
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"path/filepath"
	"regexp"
	"sync"
	"time"
)

func isURL(url string) bool {
	match, _ := regexp.MatchString("^(ftp|http|https):\\/\\/(\\w+:{0,1}\\w*@)?(\\S+)(:[0-9]+)?(\\/|\\/([\\w#!:.?+=&%@!\\-\\/]))?$", url)
	return match
}

func downloadFileToDir(url string, dir string, redownload bool) (string, error) {

	path := filepath.Join(dir, filepath.Base(url))

	// Check if file is already downloaded
	if _, err := os.Stat(path); os.IsNotExist(err) || redownload {

		logger.Println(filepath.Base(url), "not found. Downloading.")

		resp, err := http.Get(url)
		if err != nil {
			return "", err
		}
		defer resp.Body.Close()

		if resp.StatusCode == 200 {
			out, err := os.Create(path)
			if err != nil {
				return "", err
			}
			defer out.Close()

			_, err = io.Copy(out, resp.Body)
			if err != nil {
				return "", err
			}
		} else {
			return "", nil
		}

	} else {
		logger.Println(filepath.Base(url), "found at temp dir. Using it.")
	}

	return path, nil
}

func prepareDescriptorsArchive(month time.Time, tmpDir string, source string, redownload bool) (string, error) {

	filename := fmt.Sprintf("server-descriptors-%d-%02d.tar.xz", month.Year(), month.Month())

	path, err := downloadFileToDir(CollectorEndpoint+filename, tmpDir, redownload)

	return path, err
}

func prepareRecentDescriptors(tmpDir string, source string, redownload bool) ([]string, error) {
	var (
		paths []string
		wg    sync.WaitGroup
	)

	base := "https://collector.torproject.org/recent/relay-descriptors/server-descriptors/"

	// Get urls for recent
	resp, err := http.Get(base)
	if err != nil {
		return paths, nil
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return paths, nil
	}

	re := regexp.MustCompile("<a href=\"(\\d{4}-(\\d{2}-){5}server-descriptors)\">")
	names := re.FindAllStringSubmatch(string(body), -1)

	// Download them
	paths = make([]string, len(names))
	for i := range names {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			paths[i], err = downloadFileToDir(base+names[i][1], tmpDir, redownload)
			if err != nil {
				fatal.Fatal(err)
			}
		}(i)
	}

	wg.Wait()
	return paths, nil
}
