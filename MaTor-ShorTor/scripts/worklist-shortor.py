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
def costs_country_CA(relay,costs):
        return costs_country(relay,costs,"CA")

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

# TODO(shortor): set consensusFileName
consensusFileName = basedir + "/data/consensuses-2021-07/2021-07-01-00-00-00-consensus"
# TODO(shortor): set correct DB of server descriptions  
serverDBFileName = basedir+"/mator-db/server-descriptors-2021-07.db"
# TODO(shortor): set viaAllPairsFilename 
viaAllPairsFilename = basedir + "/data/viaAllPairs.csv" 

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
	"PSTor":MATorConfig(s1,s2,r1,r2,psTor,psTor),
	"PSLASTor":MATorConfig(s1,s2,r1,r2,psL,psL),
}

# Create the adversaries
Adversaries = {	
		"BW-1000":MATorAdversary(1000,costs_BW),
		"BW-2500":MATorAdversary(2500,costs_BW),
		"BW-5000":MATorAdversary(50000,costs_BW),
		"BW-10000":MATorAdversary(10000,costs_BW),
		"BW-25000":MATorAdversary(25000,costs_BW),
		"BW-50000":MATorAdversary(500000,costs_BW),
		"BW-100000":MATorAdversary(100000,costs_BW),
		"BW-250000":MATorAdversary(250000,costs_BW),
		"BW-500000":MATorAdversary(500000,costs_BW),
		"BW-1000000":MATorAdversary(1000000,costs_BW),
		"BW-2500000":MATorAdversary(2500000,costs_BW),
		"BW-5000000":MATorAdversary(5000000,costs_BW),
		"BW-100000000":MATorAdversary(100000000,costs_BW),
		"BW-250000000":MATorAdversary(250000000,costs_BW),
		"BW-500000000":MATorAdversary(500000000,costs_BW),
		"BW-7500000000":MATorAdversary(7500000000,costs_BW),
		"BW-1000000000":MATorAdversary(1000000000,costs_BW),
		"BW-2500000000":MATorAdversary(2500000000,costs_BW),
		"BW-5000000000":MATorAdversary(5000000000,costs_BW),
		"BW-75000000000":MATorAdversary(75000000000,costs_BW),
		"BW-10000000000":MATorAdversary(10000000000,costs_BW),
		"BW-25000000000":MATorAdversary(25000000000,costs_BW),
		"BW-50000000000":MATorAdversary(50000000000,costs_BW),
		"BW-75000000000":MATorAdversary(75000000000,costs_BW),
		"BW-100000000000":MATorAdversary(100000000000,costs_BW),
		"MONEY-100":MATorAdversary(100,costs_money),
		"MONEY-500":MATorAdversary(500,costs_money),
		"MONEY-1000":MATorAdversary(1000,costs_money),
		"MONEY-5000":MATorAdversary(5000,costs_money),
		"MONEY-10000":MATorAdversary(10000,costs_money),
		"MONEY-50000":MATorAdversary(50000,costs_money),
		"MONEY-100000":MATorAdversary(100000,costs_money),
		"MONEY-500000":MATorAdversary(500000,costs_money),
		"MONEY-1000000":MATorAdversary(1000000,costs_money),
		"MONEY-5000000":MATorAdversary(5000000,costs_money),
		"MONEY-10000000":MATorAdversary(10000000,costs_money),
		"MONEY-50000000":MATorAdversary(50000000,costs_money),
		"MONEY-100000000":MATorAdversary(100000000,costs_money),
		"MONEY-500000000":MATorAdversary(500000000,costs_money),
		"CC-DE":MATorAdversary(1,costs_country_DE),
		"CC-US":MATorAdversary(1,costs_country_US),
		"CC-FR":MATorAdversary(1,costs_country_FR),
		"CC-NL":MATorAdversary(1,costs_country_NL),
		"CC-GB":MATorAdversary(1,costs_country_GB),
		"CC-CA":MATorAdversary(1,costs_country_CA),
		"FIVE-EYES": MATorAdversary(1, costs_country_FVEYES),
		"k-of-N-0":MATorAdversary(0),
		"k-of-N-1":MATorAdversary(1),
		"k-of-N-2":MATorAdversary(2),
		"k-of-N-3":MATorAdversary(3),
		"k-of-N-4":MATorAdversary(4),
		"k-of-N-5":MATorAdversary(5),
		"k-of-N-6":MATorAdversary(6),
		"k-of-N-7":MATorAdversary(7),
		"k-of-N-8":MATorAdversary(8),
		"k-of-N-9":MATorAdversary(9),
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
		"k-of-N-50":MATorAdversary(50),
		"k-of-N-100":MATorAdversary(100),
		"k-of-N-150":MATorAdversary(150),
		"k-of-N-200":MATorAdversary(200),
		"k-of-N-250":MATorAdversary(250),
		"k-of-N-300":MATorAdversary(300),
		"k-of-N-350":MATorAdversary(350),
		"k-of-N-400":MATorAdversary(400),
		"k-of-N-450":MATorAdversary(450),
		"k-of-N-500":MATorAdversary(500),
		"k-of-N-550":MATorAdversary(550),
		"k-of-N-600":MATorAdversary(600),
		"k-of-N-650":MATorAdversary(650),
		"k-of-N-700":MATorAdversary(700),
		"k-of-N-750":MATorAdversary(750),
		"k-of-N-800":MATorAdversary(800),
	}


with Database("deltas.csv", ["consensus", "config", "adversary"]) as db:
	wl = MATorWorklist(db)

	wl.addConsensus(
		'vanilla-tor', 
		MATorConsensus(consensusFileName, serverDBFileName, viaAllPairsFilename, False), # False: Don't use via relays
		False,
	)
	
	wl.addConsensus(
		'shortor-tor', 
		MATorConsensus(consensusFileName, serverDBFileName, viaAllPairsFilename, True), # True: use via relays
		False, 
	)

	for CO in Configs.keys():
		wl.addConfig(CO, Configs[CO])

	for adv in Adversaries.keys():
		wl.addAdversary(adv, Adversaries[adv])

	wl.progress()

