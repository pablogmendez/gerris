#!/usr/bin/python

import os
import sys, getopt
import importlib
import threading
import logging
import collections
import re
from communications import ControllerThread, CollectorThread, ExecutionContext
from samples import SamplesData

samplesWindow = 1
procIndex = 0

#Communicate with another process through named pipes
#one for receive command, the other for send command, and the last one to receive values
callFifoPath = ''
returnFifoPath = ''
valuesFifoPath = ''


try:
    opts, args = getopt.getopt(sys.argv[1:],'', ['script=', 'samples=','mpiproc=', 'requestfifo=', 'returnfifo=', 'samplesfifo=', 'loglevel='])
except getopt.GetoptError:
    sys.stderr.write("Python **: Error invoking main.py.\nSample command line: ./main.py --script <scriptFilepath> --samples <numberOfSamplesInControllingWindow> --mpiproc <processId> --requestfifo <filepath> --returnfifo <filepath> --samplesfifo <filepath> --loglevel (debug|info|warning|error)")
    raise
if not opts or len(opts) < 6:
    sys.stderr.write("Python **: Error invoking main.py. Required arguments: --script --samples --mpiproc --requestfifo --returnfifo --samplesfifo --loglevel")
    sys.exit(1)
for opt, arg in opts:
    if opt == '--script':
        userScript = arg
    elif opt == '--samples':
        samplesWindow = int(arg)
    elif opt == '--mpiproc':
        procIndex = int(arg)
    elif opt == '--requestfifo':
        callFifoPath = arg
    elif opt == '--returnfifo':
        returnFifoPath = arg
    elif opt == '--samplesfifo':
        valuesFifoPath = arg
    elif opt == '--loglevel':
        logLevelStr = arg
try:
    logLevel = getattr(logging, logLevelStr.upper())
except AttributeError:
    sys.stderr.write("Python **: Error configuring logging level to: %s. Valid values are 'debug', 'info', 'warning', 'error'" % logLevelStr)
    raise
logging.basicConfig(format='%(asctime)s Python %(levelname)s **: PE=' + str(procIndex) + ' - %(message)s', level=logLevel)

# Load functions defined by user.
scriptMatch = re.match(r'^(.*)/(.*)\.py$', userScript)
if not scriptMatch:
    msg = "The given user script location is not valid. Provided path: %s. Expected pattern: <module-folder>/<filename>.py" % userScript
    logging.error(msg)
    raise ValueError(msg)

controllerFolder = scriptMatch.group(1)
controllerModuleName = scriptMatch.group(2)
sys.path.append(controllerFolder)
controlFunc = importlib.import_module(controllerModuleName)

samples = SamplesData(samplesWindow)
context = ExecutionContext(procIndex, callFifoPath, returnFifoPath, valuesFifoPath)
with context:
    # Create Values and Function threads.
    collector = CollectorThread(samples, context)
    controller = ControllerThread(samples, controlFunc, context)
    context.register(collector)
    context.register(controller)

    collector.start()
    controller.start()
    try:
        collector.join()
        controller.join()
        logging.info("Python server finished")
    except KeyboardInterrupt:
        logging.error("Keyboard signal detected. Aborting tasks to close the server...")
        context.notifyError('Keyboard signal detected.')
        context.terminateOnErrors()
    except Exception as e:
        logging.error("Closing with errors: %s" % e)
        context.notifyError('Closing with errors.', e)
        contxt.terminateOnErrors()


