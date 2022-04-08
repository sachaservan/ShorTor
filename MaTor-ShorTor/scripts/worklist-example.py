#!/usr/bin/env python
import os
from worklist import *
from mator import *
from database import *


# Just for debugging purposes - what C++ knows about our hardware
print("MATor detected",hardwareConcurrency(),"CPU Cores.")

# Sample code
basedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) #svn branch root
consensus = basedir + "/test/Data/2014-10-04-05-00-00-consensus-filtered-fast"
consensus2 = basedir + "/test/Data/2014-10-04-05-00-00-consensus-filtered-fast"
viaAllPairsFilename = basedir + "/test/Data/viaAllPairs.csv"

# Create Spec classes as in C++
s1 = SenderSpec("1.2.3.4")
s2 = SenderSpec("200.3.4.5")
r1 = RecipientSpec("8.8.8.8")
r2 = RecipientSpec("8.8.8.8")
ps = PSTorSpec()
# Access class elements as in C++
r1.ports.add(80)
r1.ports.add(443)
r2.ports.add(80)

# Create a MATor configuration (ideal to store in a worklist...)
config = MATorConfig(s1, s2, r1, r2, ps, ps)
config2 = MATorConfig(s1, s2, r2, r1, ps, ps)
config3 = MATorConfig(s2, s1, r2, r1, ps, ps)
config4 = MATorConfig(s2, s1, r1, r2, ps, ps)



# Example adversary cost function - inline example below
def addTwo(relay, costs):
	# debug message - you can access all relay's information here
	print("Found",relay.ip," ("+relay.name+")")
	return costs + 2 # => costs = 3 for all relays


# open csv-database (filename, "default keys")
# keys are the part of a row that have to be unique (and can be used to identify a specific calculation)
with Database("test.csv", ["consensus", "config", "adversary"]) as db:
	# Create the worklist
	wl = MATorWorklist(db)
	# Add all consensus files (including a short name, possibly the database, and whether precise mode should be used)
	wl.addConsensus("test1-fast", MATorConsensus(consensus, ""), True) # with precise calculation, "" is the database file (none in this case)
	wl.addConsensus("test2-fast", MATorConsensus(consensus2)) # database defaults to "" (none), precise defaults to False
	# Add your configurations
	wl.addConfig("tc1", config)
	wl.addConfig("tc2", config2)
	wl.addConfig("tc3", config3)
	wl.addConfig("tc4", config4)
	# Add your adversaries
	wl.addAdversary("10-of-n", MATorAdversary(10))
	wl.addAdversary("20-of-2n", MATorAdversary(20, "ANY ? SET 2"))
	wl.addAdversary("30-of-3n", MATorAdversary(30, addTwo))
	# Example for an inline python-cost-definition
	wl.addAdversary("40-of-4n", MATorAdversary(40, lambda relay, costs: 4))
	wl.addAdversary("bw-100000", MATorAdversary(100000, lambda relay, costs: relay.bandwidth))
	# Let the fun begin!
	wl.progress()




"""
=== RELAY INFORMATION ===
- Relay.name
- Relay.publishedDate
- Relay.fingerprint
- Relay.version
- Relay.country
- Relay.ASNumber
- Relay.ASName
- Relay.platform
- Relay.bandwidth
- Relay.averagedBandwidth
- Relay.flags
- Relay.latitude
- Relay.longitude
- Relay.registeredAt
- Relay.ip
- Relay.supportsConnection(ip, port)
- Relay.getSubnet()
"""