package main

import (
	"github.com/NullHypothesis/zoossh"
)
//	"github.com/dmgawel/zoossh"

// Soritng descriptors
type descriptorsByTime []*zoossh.RouterDescriptor

func (s descriptorsByTime) Len() int {
	return len(s)
}

func (s descriptorsByTime) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s descriptorsByTime) Less(i, j int) bool {
	return s[i].Published.Before(s[j].Published)
}

// Sorting timeline entries
type timelineByTime []TimelineEntry

func (s timelineByTime) Len() int {
	return len(s)
}

func (s timelineByTime) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s timelineByTime) Less(i, j int) bool {
	return s[i].time.Before(s[j].time)
}
