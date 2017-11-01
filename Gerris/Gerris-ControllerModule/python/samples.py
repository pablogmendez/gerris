import threading
import struct
from struct import *
import logging
import collections

# Class for storing a given force value from gerris.
class ForceData:
    def __init__(self, pf, vf, pm, vm):
        self.pf = pf
        self.vf = vf
        self.pm = pm
        self.vm = vm

    def __str__(self):
        return 'PF:%s, VF:%s, PM:%s, VM:%s' % (self.pf, self.vf, self.pm, self.vm)

    def __repr__(self):
        return 'ForceData(%s, %s, %s, %s)' % (self.pf, self.vf, self.pm, self.vm)

# Class for storing a given Location value from gerris
class ProbeData:
    def __init__(self, location, variable, value):
        self.location = location
        self.variable = variable
        self.value = value

    def __str__(self):
        return 'Var:%s, %s=%f' % (self.variable, self.location, self.value)

    def __repr__(self):
        return 'ProbeData(%s, %s, %f)' % (self.location, self.variable, self.value)

# Class for storing a given value from gerris. It can be either a location or a force value.
class Sample:
    def __init__(self, time, step, data):
        self.time = time
        self.step = step
        self.data = data

    def __str__(self):
        return '(step:%d, t:%.3f, %s)' % (self.step, self.time, self.data)

    def __repr__(self):
        return 'Sample(%.3f, %d, %s)' % (self.time, self.step, repr(self.data))

class SamplesData:
    class SamplesList:
        def __init__(self, time, step):
            self.time = time
            self.step = step
            self.samples = []

    def __init__(self, samplesWindow):
        self.currentStep = None
        self.currentTime = None
        self._lastSamples = None
        self.forces = collections.deque()
        self._allSamples = collections.deque()
        self.samplesWindow = samplesWindow
        self._probesByTime = collections.defaultdict(list)
        self._probesByVariable = collections.defaultdict(list)
        self._probesByLocation = collections.defaultdict(list)


    def addForce(self, sample):
        self.forces.append(sample)
        if len(self.forces) > self.samplesWindow:
            self.forces.popleft()

    def addProbe(self, sample):
        if self.currentStep is None or self.currentStep < sample.step:
            self._lastSamples = SamplesData.SamplesList(sample.time, sample.step)
            self._allSamples.append(self._lastSamples)
            self.currentStep = sample.step
            self.currentTime = sample.time
        elif self.currentStep > sample.step:
            raise ValueError('Sample to add was generated prior to the last registered timestamp. Sample step:%d - time: %f. Last registered step:%d - time: %f.'
                             % (sample.step, sample.time, self.currentStep, self.currentTime))

        self._lastSamples.samples.append(sample)

        self._probesByTime[sample.time].append(sample)
        self._probesByVariable[sample.data.variable].append(sample)
        self._probesByLocation[sample.data.location].append(sample)
        if len(self._allSamples) > self.samplesWindow:
            samplesList = self._allSamples.popleft()
            del self._probesByTime[samplesList.time]
            for sample in samplesList.samples:
                self._probesByVariable[sample.data.variable].remove(sample)
                self._probesByLocation[sample.data.location].remove(sample)

    @property
    def allTimes(self):
        return self._probesByTime.keys()

    def getClosestTime(self, time):
        if not self._probesByTime:
            return None
        elif time in self._probesByTime:
            return time
        else:
            keys = self._probesByTime.keys()
            times = sorted(keys, key=lambda t:abs(t-time))
            return times[0]

    def getPreviousClosestTime(self, time):
        if not self._probesByTime:
            return None
        else:
            keys = [k for k in self._probesByTime.keys() if k < time]
            if len(keys) == 0:
                return None
            else:
                times = sorted(keys, key=lambda t:abs(t-time))
                return times[0]

    @property
    def allVariables(self):
        return self._probesByVariable.keys()

    @property
    def allLocations(self):
        return self._probesByLocation.keys()

    def samplesByVariable(self, variable):
        if variable in self._probesByVariable:
            return self._probesByVariable[variable]
        else:
            return []

    def samplesByLocation(self, location):
        if location in self._probesByLocation:
            return self._probesByLocation[location]
        else:
            return []

    def samplesByTime(self, time):
        if time in self._probesByTime:
            return self._probesByTime[time]
        else:
            return []

    @property
    def samples(self):
        result = []
        for s in self._allSamples:
            result.extend(s.samples)
        return result

    @property
    def currentSamples(self):
        if self.currentStep is None:
            return None
        else:
            return self._lastSamples.samples
