from math import *
import logging

actuationStartTime = 42.
velocityRef = 1.
kProp = 1.75
kInt = 0.023

def velocityAbsErrorIntegrate(velocityRef, samples, lastIterationTime):
    integral = 0;
    for time in samples.allTimes:
        if time < lastIterationTime:
            samplesAtTime = samples.samplesByTime(time)
            integral += velocityAbsError(velocityRef, samplesAtTime)
    return integral

def velocityAbsError(velocityRef, samples):
    error = 0
    uList = [s.data.value for s in samples if s.data.variable == 'U']
    vList = [s.data.value for s in samples if s.data.variable == 'V']
    for i in range(len(uList)):
        u = uList[i]
        v = vList[i]
        stepError = abs(sqrt(u*u + v*v) - velocityRef)
        error += stepError
    return error

def actuation(time, step, samples):
    global velocityRef, kProp, kInt, actuationStartTime
    act = 0.
    lastIterationTime = samples.getPreviousClosestTime(time)
    if lastIterationTime >= actuationStartTime:
        velError = velocityAbsError(velocityRef, samples.samplesByTime(lastIterationTime))
        velErrorInt = velocityAbsErrorIntegrate(velocityRef, samples, lastIterationTime)
        act = kProp*velError + kInt*velErrorInt
        logging.info('step=%d - t=%.3f - act=%.2f - e=%.2f - eInt=%.2f' % (step, time, act, velError, velErrorInt))
    return act

def init(proc_index):
    pass

def destroy(proc_index):
    pass
