#!/usr/bin/env python
import os
from worklist import *
from mator import *
from database import *
from calendar import monthrange

from moneyadv import costs_money

def costs_BW(relay, cost):
        return relay.averagedBandwidth

def costs_country(relay, cost, cc):
        if relay.country == cc:
                return 0
        else:
                return 1000

def costs_country_DE(relay,costs):
        return costs_country(relay,costs,"DE")
def costs_country_US(relay,costs):
        return costs_country(relay,costs,"US")
def costs_country_FR(relay,costs):
        return costs_country(relay,costs,"FR")
def costs_country_GB(relay,costs):
        return costs_country(relay,costs,"GB")
def costs_country_NL(relay,costs):
        return costs_country(relay,costs,"NL")

FIVEEYES = ["AU","CA","GB","NZ","US"]
def costs_country_FVEYES(relay,costs):
        if relay.country in FIVEEYES:
                return 0
        else:
                return 1000
        return costs_country(relay,costs,"DE")

# Just for debugging purposes - what C++ knows about our hardware
print("MATor detected",hardwareConcurrency(),"CPU Cores.")

basedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) #svn branch root
print("base dir: " + str(basedir))

# which year of consensus data to look at?
years = ["2014"]

consensus = basedir + "/build/2014-08-04-05-00-00-consensus"

# Create Spec classes as in C++
s1 = SenderSpec("144.118.66.83",39.9597,-75.1968)
s2 = SenderSpec("129.79.78.192",39.174729, -86.507890)
r1 = RecipientSpec("130.83.47.181",49.8719, 8.6484)
r2 = RecipientSpec("134.58.64.12",50.8796, 4.7009)
r6667 = RecipientSpec("134.58.64.12",50.8796, 4.7009)
psTor = PSTorSpec()
psD = PSDistribuTorSpec()
psU = PSUniformSpec()
psL = PSLASTorSpec()
psS = PSSelektorSpec()

# Access class elements as in C++
r1.ports = set([443])
r2.ports = set([443])
r6667.ports = set([443,6667])

# Create a MATor configuration (ideal to store in a worklist...)
Configs = {
	"PSTor-PSTor-443":MATorConfig(s1,s2,r1,r2,psTor,psTor)
	}

# Create the adversaries
Adversaries = {	"k-of-N-00":MATorAdversary(0),
		"k-of-N-01":MATorAdversary(1),
		"k-of-N-02":MATorAdversary(2),
		"k-of-N-03":MATorAdversary(3),
		"k-of-N-04":MATorAdversary(4),
		"k-of-N-05":MATorAdversary(5),
		"k-of-N-06":MATorAdversary(6),
		"k-of-N-07":MATorAdversary(7),
		"k-of-N-08":MATorAdversary(8),
		"k-of-N-09":MATorAdversary(9),
		"k-of-N-10":MATorAdversary(10),
		"k-of-N-11":MATorAdversary(11),
		"k-of-N-12":MATorAdversary(12),
		"k-of-N-13":MATorAdversary(13),
		"k-of-N-14":MATorAdversary(14),
		"k-of-N-15":MATorAdversary(15),
		"k-of-N-16":MATorAdversary(16),
		"k-of-N-17":MATorAdversary(17),
		"k-of-N-18":MATorAdversary(18),
		"k-of-N-19":MATorAdversary(19),
		"k-of-N-20":MATorAdversary(20),
		"k-of-N-21":MATorAdversary(21),
		"k-of-N-22":MATorAdversary(22),
		"k-of-N-23":MATorAdversary(23),
		"k-of-N-24":MATorAdversary(24),
		"k-of-N-25":MATorAdversary(25),
		"BW-10000":MATorAdversary(10000,costs_BW),
		"BW-100000":MATorAdversary(100000,costs_BW),
		"BW-1000000":MATorAdversary(1000000,costs_BW),
		"BW-10000000":MATorAdversary(10000000,costs_BW),
		"BW-100000000":MATorAdversary(100000000,costs_BW),
		"BW-1000000000":MATorAdversary(1000000000,costs_BW),
		"MONEY-1000":MATorAdversary(1000,costs_money),
		"MONEY-10000":MATorAdversary(10000,costs_money),
		"MONEY-100000":MATorAdversary(100000,costs_money),
		"MONEY-1000000":MATorAdversary(1000000,costs_money),
		"MONEY-10000000":MATorAdversary(10000000,costs_money),
		"MONEY-100000000":MATorAdversary(100000000,costs_money),
		"CC-DE":MATorAdversary(1,costs_country_DE),
		"CC-US":MATorAdversary(1,costs_country_US),
		"CC-FR":MATorAdversary(1,costs_country_FR),
		"CC-NL":MATorAdversary(1,costs_country_NL),
		"CC-GB":MATorAdversary(1,costs_country_GB),
		"FIVE-EYES": MATorAdversary(1, costs_country_FVEYES)
}

def strTwoDigit(i):
    ks = str(i)
    if len(ks) == 1:
        ks = "0"+ks
    return ks

with Database("deltas.csv", ["consensus", "config", "adversary"]) as db:
	wl = MATorWorklist(db)
	for Y in years:
		for M in range(1,13):
			YM=Y+"-"+strTwoDigit(M)
			days= monthrange(int(YM.split("-")[0]), int(YM.split("-")[1]))
			for D in range(1, days[1]+1):
				DAY=strTwoDigit(D)
				for H in range(12,24,12):
					HOUR=strTwoDigit(H)
					wl.addConsensus(
						YM+"-"+DAY+"-"+HOUR, 
						MATorConsensus(basedir+"/build/Release/data/consensuses-"+YM+"/"+DAY+"/"+YM+"-"+DAY+"-"+HOUR+"-00-00-consensus", basedir+"/mator-db/server-descriptors-"+YM+".db"),
						True) # True for precise calculation

	for CO in Configs.keys():
		wl.addConfig(CO, Configs[CO])

	for adv in Adversaries.keys():
		wl.addAdversary(adv, Adversaries[adv])

	wl.progress()

