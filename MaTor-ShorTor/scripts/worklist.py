from __future__ import print_function
import time
import sys
import math
from mator import *
from queue import Queue
from database import *

basedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
def formatDuration(duration, showms=True):
    if not isinstance(duration, int) and not isinstance(duration, float):
        return duration
    days, remainder = divmod(duration, 3600 * 24)
    hours, remainder = divmod(remainder, 3600)
    minutes, remainder = divmod(remainder, 60)
    seconds, ms = divmod(remainder, 1)
    ms = ms * 1000
    if days > 0:
        return '%sd %02d:%02d:%02d' % (int(days), int(hours), int(minutes), int(seconds))
    elif duration > 60 or not showms:
        return '%02d:%02d:%02d' % (int(hours), int(minutes), int(seconds))
    else:
        return '%02d:%02d:%02d.%03d' % (int(hours), int(minutes), int(seconds), int(ms))

def strTwoDigit(i):
    ks = str(i)
    if len(ks) == 1:
        ks = "0"+ks
    return ks


class MATorWorklist(object):
    def __init__(self, db):
        self.configs = []
        self.configSet = set()
        self.adversaries = []
        self.adversarySet = set()
        self.consensus = []
        self.consensusSet = set()
        self.preciseConsensusSet = set()
        self.networkConsensusSet = set()
        self.count = 0
        self.db = db
        self.currentConsensus = "-"
        self.currentConfig = "-"
        self.currentAdversary = "-"
        # init database
        self.db.addColumns(["SA", "RA", "RelA", "PreciseSA", "PreciseRA", "PreciseRelA"])

    def addConfig(self, name, config):
        if name not in self.configSet:
            self.configs.append((name, config))
            self.configSet.add(name)

    def addAdversary(self, name, adversary):
        if name not in self.adversarySet:
            self.adversaries.append((name, adversary))
            self.adversarySet.add(name)

    def addConsensus(self, name, consensus, precise=False, network=False):
        if name not in self.consensusSet:
            self.consensus.append((name, consensus))
            self.consensusSet.add(name)
            if network:
                self.networkConsensusSet.add(name)
            else:
                if precise:
                    self.preciseConsensusSet.add(name)

    def addNetworkConsensus(self, name, consensus):
        if name not in self.consensusSet:
            self.consensus.append((name, consensus))
            self.consensusSet.add(name)
            self.networkConsensusSet.add(name)

    def progress(self):
        print ("")
        print ("_" * 80)
        print ("")
        self.starttime = time.time()
        self.analyzeWorkload()
        if self.workcountAll == 0:
            print ("Nothing to do!")
            return
        self.update(True)
        for (conname, con) in self.consensus:
            # Load consensus
            self.currentConsensus = conname
            self.currentConfig = "-"
            self.currentAdversary = "-"
            self.update()
            exceptionOcc=1
            while exceptionOcc==1:
                try:
                    matorcon = con.load()
                    exceptionOcc=0
                except (RuntimeError, ValueError) as err:
                    print ("exception caught " + str(err))
                    parts= conname.split("-")
                    if int(parts[3])<=22:
                        conname= parts[0]+ "-" + parts[1] + "-" +  parts[2] + "-" + strTwoDigit(int(parts[3])+1)
                    #elif int(parts[2])<=30:
                        #conname= parts[0]+ "-" + parts[1] + "-" +   strTwoDigit(int(parts[2])+1) + "-" + "00"
                    else: 
                        break
                    print("Opening: " + basedir+"/mator-db/server-descriptors-"+parts[0]+ "-" + parts[1]+".db")
                    con=MATorConsensus(basedir+"/build/Release/data/consensuses-"+parts[0]+ "-" + parts[1]+"/"+ parts[2]+"/"+ conname+"-00-00-consensus", basedir+"/mator-db/server-descriptors-"+parts[0]+ "-" + parts[1]+".db")
            if not exceptionOcc:
                for (configname, config) in self.configs:
                    self.currentConfig = configname
                    mator = None
                    partial = False
                    starttime = time.time()
                    for (advname, adversary) in self.adversaries:
                        self.currentAdversary = advname
                        self.update()
                        dbkey = [conname, configname, advname]
                        if dbkey in self.db and (conname not in self.preciseConsensusSet or (
                                "PreciseSA" in self.db[dbkey] and self.db[dbkey]["PreciseSA"] != "")):
                            # skip
                            partial = True
                            continue
                        if mator is None:
                            # CREATE MATor
                            mator = config.createMATor(matorcon)
                            mator.prepare()
                        results = {}
                        if conname in self.networkConsensusSet:
                            mator.prepareNetworkCalculation(adversary.split(" "))
                            results["SA"] = mator.getNetworkSenderAnonymity()
                            results["RA"] = mator.getNetworkRecipientAnonymity(),
                            results["RelA"] = mator.getNetworkRelationshipAnonymity()
                            self.db.addData(dbkey, results)
                            self.db.save()
                            continue
                        mator.setAdversary(adversary)

                        # precise calculations?
                        if conname in self.preciseConsensusSet:
                            results["PreciseSA"] = mator.getGreedyPreciseSenderAnonymity()
                            results["PreciseRA"] = mator.getGreedyPreciseRecipientAnonymity()
                            results["PreciseRelA"] = mator.getGreedyPreciseRelationshipAnonymity()
                        else:
                            # Calculate "normal" anonymity guarantees
                            results["SA"] = mator.getSenderAnonymity()
                            results["RA"] = mator.getRecipientAnonymity(),
                            results["RelA"] = mator.getRelationshipAnonymity()

                        # Save results
                        self.db.addData(dbkey, results)
                        self.db.save()
                    # did any work?
                    if mator is not None:
                        self.finish(partial, conname in self.preciseConsensusSet, time.time() - starttime)
                        self.update(True)
        self.finishedAll()

    # Logs processing time of calculations
    def finish(self, partial, precise, dt):
        self.worktime[partial][precise] += dt
        self.worktimeAll += dt
        self.workdone[partial][precise] += 1
        self.workdoneAll += 1

    def finishedAll(self):
        self.printFullLine("")
        self.printFullLine("Finished!  Took:", formatDuration(time.time() - self.starttime))

    def getETA(self):
        if self.workcountAll == self.workdoneAll:
            return 0.0
        if self.workdoneAll == 0:
            return "?"
        avgtime = {True: {True: None, False: None}, False: {True: None, False: None}}
        for a in [True, False]:
            for b in [True, False]:
                if self.workcount[a][b] == 0:
                    avgtime[a][b] = 0.0
                elif self.workdone[a][b] > 0:
                    avgtime[a][b] = self.worktime[a][b] / self.workdone[a][b]
        # Interpolate missing values
        for b in [True, False]:
            if avgtime[True][b] is None:
                avgtime[True][b] = avgtime[False][b]
        # Sum up remaining time
        remaining = 0.0
        for a in [True, False]:
            for b in [True, False]:
                if avgtime[a][b] is None:
                    return (self.worktimeAll / self.workdoneAll) * (self.workcountAll - self.workdoneAll)
                remaining += avgtime[a][b] * (self.workcount[a][b] - self.workdone[a][b])
        return math.ceil(remaining * 1.1)  # overhead, consensus loading, ...

    # Initializes work time logging
    def analyzeWorkload(self):
        # Number of work to do
        self.workcount = {True: {True: 0, False: 0}, False: {True: 0, False: 0}}  # partial? => (precise? => count)
        self.workcountAll = 0
        nothingToDo = 0
        for (conname, con) in self.consensus:
            for (configname, config) in self.configs:
                work = False
                partial = False
                for (advname, adversary) in self.adversaries:
                    dbkey = [conname, configname, advname]
                    if dbkey in self.db and (conname not in self.preciseConsensusSet or (
                            "PreciseSA" in self.db[dbkey] and self.db[dbkey]["PreciseSA"] != "")):
                        partial = True
                        continue
                    work = True
                if work:
                    self.workcount[partial][conname in self.preciseConsensusSet] += 1
                    self.workcountAll += 1
                else:
                    nothingToDo += 1
        # Time taken for the already processed work
        self.worktime = {True: {True: 0.0, False: 0.0}, False: {True: 0.0, False: 0.0}}
        self.worktimeAll = 0.0
        # Number of already progressed items
        self.workdone = {True: {True: 0, False: 0}, False: {True: 0, False: 0}}
        self.workdoneAll = 0
        if nothingToDo > 0:
            print ("Nothing to do for", nothingToDo, "consensus/config combinations")

    def update(self, big=False):
        if big:
            #                  "________________________________________________________________________________"
            # non-partial takes 2x time of partial, precise takes 3x time of non-precise
            percent = 2 * (self.workcount[False][True] * 3 + self.workcount[False][False]) + self.workcount[True][
                                                                                                 True] * 3 + \
                      self.workcount[True][False]
            percent = (2 * (self.workdone[False][True] * 3 + self.workdone[False][False]) + self.workdone[True][
                True] * 3 + self.workdone[True][False]) / float(percent)
            bars = int(round(percent * 38))
            bar = "#" * bars + " " * (38 - bars)
            self.printFullLine("[%s] (%5.1f%%)    [%d/%d]" % (bar, percent * 100, self.workdoneAll, self.workcountAll))
            self.printFullLine(" " * 40, "Running:", formatDuration(time.time() - self.starttime, False), "  ETA:",
                               formatDuration(self.getETA(), False))
        # re-print last line
        self.printHalfLine(
            "Current: [" + self.currentConsensus + ", " + self.currentConfig + ", " + self.currentAdversary + "]")

    lastlinelen = 0

    def printFullLine(self, *args):
        msg = " ".join(map(str, args))
        if len(msg) < self.lastlinelen:
            msg += " " * (self.lastlinelen - len(msg))
        sys.stderr.write("\r" + msg + "\n")
        sys.stderr.flush()
        self.lastlinelen = 0

    def printHalfLine(self, *args):
        msg = " ".join(map(str, args))
        l = len(msg)
        if len(msg) < self.lastlinelen:
            msg += " " * (self.lastlinelen - len(msg))
        self.lastlinelen = l
        sys.stderr.write("\r" + msg)
        sys.stderr.flush()




