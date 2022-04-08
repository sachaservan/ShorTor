import sys
import os

# Locate the mator library
def findPyMATor():
	libname = "pymator.pyd" if sys.platform.startswith('win') else "pymator.so"
	if not os.path.isfile(libname):
		scriptdir = os.path.dirname(os.path.realpath(__file__))
		basedir = os.path.dirname(os.path.dirname(scriptdir))
		# is 32 or 64bit interpreter
		vsDir = "x64" if sys.maxsize > pow(2, 32) else "Win32"
		for d in [basedir, scriptdir, basedir + "/build/Release/lib", os.path.dirname(basedir) + "/" + vsDir + "/Release_Python", os.path.dirname(basedir) + "/" + vsDir + "/Debug_Python"]:
			if os.path.isfile(d + "/" + libname):
				sys.path.append(d)
				print("[pyMATor] Found library in path:",d)
				return
		raise Exception("[pyMATor] Did not find pymator. Please copy the pymator.so or pymator.pyd in this directory!")

findPyMATor()
from pymator import *
print("hi")

class MATorConfig(object):
	"""Configuration storage for a MATor instance"""
	def __init__(self, senderSpec1, senderSpec2, recipientSpec1, recipientSpec2, psSpec1, psSpec2, fast = False, useViaRelays = False):
		self.senderSpec1 = senderSpec1
		self.senderSpec2 = senderSpec2
		self.recipientSpec1 = recipientSpec1
		self.recipientSpec2 = recipientSpec2
		self.psSpec1 = psSpec1
		self.psSpec2 = psSpec2
		self.fast = fast
		self.useViaRelays = useViaRelays;

	def createMATor(self, consensus):
		if isinstance(consensus, MATorConsensus):
			m = MATor(self.senderSpec1, self.senderSpec2, self.recipientSpec1, self.recipientSpec2, self.psSpec1, self.psSpec2, consensus.consensus, consensus.database, consensus.viaAllPairs, consensus.useViaRelays, self.fast)
		elif isinstance(consensus, Consensus):
			m = MATor(self.senderSpec1, self.senderSpec2, self.recipientSpec1, self.recipientSpec2, self.psSpec1, self.psSpec2, consensus, self.fast)
		else:
			raise Exception("Invalid arg! "+str(consensus))
		m.commitSpecification()
		return m

	def __eq__(self, other):
		if isinstance(other, self.__class__):
			return self.__dict__ == other.__dict__
		else:
			return False
	def __ne__(self, other):
		return not self.__eq__(other)


class MATorConsensus(object):
	def __init__(self, consensus, database = "", viaAllPairs = "", useVias=False):
		self.consensus = consensus
		self.database = database
		self.viaAllPairs = viaAllPairs
		self.useVias = useVias
	def __eq__(self, other):
		if isinstance(other, self.__class__):
			return self.__dict__ == other.__dict__
		else:
			return False
	def __ne__(self, other):
		return not self.__eq__(other)
	def load(self):
		return Consensus(self.consensus, self.database, self.viaAllPairs, self.useVias)

class MATorAdversary(object):
	"""Adversary configuration, consisting of pcf cost function and budget"""
	def __init__(self, budget, pcf = ""):
		self.budget = budget
		self.pcf = pcf
	def __eq__(self, other):
		if isinstance(other, self.__class__):
			return self.__dict__ == other.__dict__
		else:
			return False
	def __ne__(self, other):
		return not self.__eq__(other)



# === MATor extensions ===
def MATor__setAdversary(self, adversary):
	# check if we have a pcf callback
	if hasattr(adversary.pcf, '__call__'):
		self.setPCFCallback(adversary.pcf)
	else:
		self.setPCFGIL(adversary.pcf)
	self.setAdversaryBudget(adversary.budget)
	# self.commitPCFs()

MATor.setAdversary = MATor__setAdversary
