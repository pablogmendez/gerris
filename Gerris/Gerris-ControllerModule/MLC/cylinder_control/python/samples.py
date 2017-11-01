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
        self.currentTimeCompleted = False
        self.forces = collections.deque()
        self.samplesWindow = samplesWindow
        self._index = SamplesIndex()

    def addForce(self, sample):
        self.forces.append(sample)
        if len(self.forces) > self.samplesWindow:
            self.forces.popleft()

    def addProbe(self, sample):
        if self.currentStep is None or self.currentStep < sample.step:
            self._index.nextSamplesTime(sample.time, sample.step)
            self.currentStep = sample.step
            self.currentTime = sample.time
            self.currentTimeCompleted = False
        elif self.currentStep > sample.step:
            raise ValueError('Sample to add was generated prior to the last registered timestamp. Sample step:%d - time: %f. Last registered step:%d - time: %f.'
                             % (sample.step, sample.time, self.currentStep, self.currentTime))

        self._index.addProbe(sample)
        if len(self._index.allTimes) > self.samplesWindow:
            self._index.removeFirstSamplesTime()

    @property
    def allTimes(self):
        return self._index.allTimes

    @property
    def completedTime(self):
        if self.currentTimeCompleted:
            return self.currentTime
        elif len(self._index.allTimes) > 1:
            return self._index.allTimes[-1]
        else:
            return None

    @property
    def currentSamples(self):
        if self.currentStep is None:
            return None
        else:
            return self._index.currentSamples

    @property
    def allVariables(self):
        return self._index.allVariables

    @property
    def allLocations(self):
        return self._index.allLocations

    @property
    def all(self):
        return self._index.samples

    def search(self):
        return SamplesSearcher(self._index)

class SamplesSearcher:
    def __init__(self, index):
        self.__index = index
        self.__times = []
        self.__variables = []
        self.__locations = []

    def byTime(self, time):
        return self.byTimes([time])

    def byTimes(self, times):
        self.__times = times
        return self

    def byVariable(self, variable):
        return self.byVariables([variable])

    def byVariables(self, variables):
        self.__variables = variables
        return self

    def byLocation(self, location):
        return self.byLocations([location])

    def byLocations(self, locations):
        self.__locations = locations
        return self

    def asSamples(self):
        byTimes = self.__filter(self.__index, self.__times, 
                                lambda index,t: index.samplesByTime(t))
        byVariables = self.__filter(byTimes, self.__variables, 
                                lambda index,v: index.samplesByVariable(v))
        byLocations = self.__filter(byVariables, self.__locations, 
                                lambda index,l: index.samplesByLocation(l))
        return byLocations.samples

    def asValues(self):
        return [s.data.value for s in self.asSamples()]

    def __filter(self, prevIndex, searchItems, funcSearch):
        newIndex = SamplesIndex()
        if len(searchItems) == 0:
            newIndex.extendAllSamples(prevIndex.samples)
        else:
            for i in searchItems:
                newIndex.extendAllSamples(funcSearch(prevIndex, i))
        return newIndex

class SamplesIndex:
    def __init__(self):
        self._lastSamples = None
        self._allSamples = collections.deque()
        self._probesByTime = collections.defaultdict(list)
        self._probesByVariable = collections.defaultdict(list)
        self._probesByLocation = collections.defaultdict(list)

    def nextSamplesTime(self, time, step):
        self._lastSamples = SamplesData.SamplesList(time, step)
        self._allSamples.append(self._lastSamples)

    def extendAllSamples(self, samples):
        timesAndSteps = sorted(set(map(lambda s:(s.time,s.step), samples)))
        for time,step in timesAndSteps:
            samplesInTime = [s for s in samples if s.time == time]
            self.nextSamplesTime(time,step)
            for s in samplesInTime:
                self.addProbe(s)

    def addProbe(self, sample):
        self._lastSamples.samples.append(sample)

        self._probesByTime[sample.time].append(sample)
        self._probesByVariable[sample.data.variable].append(sample)
        self._probesByLocation[sample.data.location].append(sample)

    def removeFirstSamplesTime(self):
        samplesList = self._allSamples.popleft()
        del self._probesByTime[samplesList.time]
        for sample in samplesList.samples:
            self._probesByVariable[sample.data.variable].remove(sample)
            self._probesByLocation[sample.data.location].remove(sample)

    @property
    def allTimes(self):
        return self._probesByTime.keys()

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
        return self._lastSamples.samples

