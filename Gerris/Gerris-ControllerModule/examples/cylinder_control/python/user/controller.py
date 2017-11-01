from math import *
import logging

actuationStartTime = 42.
velocityRef = 1.
kProp = 1.75
kInt = 0.023

def velocityAbsError(velocityRef, samples, completedTime):
    uValues = samples.search().byTime(completedTime).byVariable('U').asValues()
    vValues = samples.search().byTime(completedTime).byVariable('V').asValues()
    return computeVelocityAbsError(velocityRef, uValues, vValues)

def velocityAbsErrorIntegrate(velocityRef, samples, completedTime):
    integral = 0;
    for time in samples.allTimes:
        if time < completedTime:
            uValues = samples.search().byTime(time).byVariable('U').asValues()
            vValues = samples.search().byTime(time).byVariable('V').asValues()
            integral += computeVelocityAbsError(velocityRef, uValues, vValues)
    return integral

def computeVelocityAbsError(velocityRef, uValues, vValues):
    error = 0
    for i in range(len(uValues)):
        u = uValues[i]
        v = vValues[i]
        stepError = abs(sqrt(u*u + v*v) - velocityRef)
        error += stepError
    return error

def actuation(time, step, samples):
    act = 0.
    completedTime = samples.completedTime
    if completedTime >= actuationStartTime:
        velError = velocityAbsError(velocityRef, samples, completedTime)
        velErrorInt = velocityAbsErrorIntegrate(velocityRef, samples, completedTime)
        act = kProp*velError + kInt*velErrorInt
        logging.info('step=%d - t=%.3f - act=%.2f - e=%.2f - eInt=%.2f' % (step, time, act, velError, velErrorInt))
    return act

def init(proc_index):
    pass

def destroy(proc_index):
    pass
