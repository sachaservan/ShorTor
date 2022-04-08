#!/usr/bin/env python

# Helper stuff
import os
import time

def formatDuration(duration):
	days, remainder    = divmod(duration,  3600*24)
	hours, remainder   = divmod(remainder, 3600)
	minutes, remainder = divmod(remainder, 60)
	seconds, ms        = divmod(remainder, 1)
	ms = ms*1000
	if days > 0:
		return '%sd %02d:%02d:%02d' % (int(days), int(hours), int(minutes), int(seconds))
	else:
		return '%02d:%02d:%02d.%03d' % (int(hours), int(minutes), int(seconds), int(ms))


# Import pyMATor
from mator import *

basedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
#consensusFilename = basedir + "/test/Data/2014-10-04-05-00-00-consensus-filtered-fast"
consensusFilename = basedir + "/test/Data/2014-10-04-05-00-00-consensus-filtered"
viaAllPairsFilename = basedir + "/test/Data/test_viaAllPairs.csv"

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
config = MATorConfig(s1, s2, r1, r2, ps, ps, viaAllPairsFilename)
consensus = MATorConsensus(consensusFilename)

# Create MATor from config (saves "commit" handling)
m = config.createMATor(consensus)

# Define adversary inline (saves "commit" handling)
m.setAdversary(MATorAdversary(10))

start = time.process_time()
sa   = m.getSenderAnonymity()
ra   = m.getRecipientAnonymity()
rela = m.getRelationshipAnonymity()
print("SA   =",sa)
print("RA   =",ra)
print("RelA =",rela)
print("Took: ",formatDuration(time.process_time()-start))



# Define adversary with pcf
adv = MATorAdversary(20, "ANY ? SET 2")
m.setAdversary(adv)
sa   = m.getSenderAnonymity()
ra   = m.getRecipientAnonymity()
rela = m.getRelationshipAnonymity()
print("SA   =",sa)
print("RA   =",ra)
print("RelA =",rela)
