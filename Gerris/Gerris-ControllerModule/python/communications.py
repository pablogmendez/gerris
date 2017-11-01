import threading
import select
import struct
import logging
import re
from struct import *
from samples import Sample, ForceData, ProbeData
from enum import Enum

_readTimeoutSecs = 10
_normalizeRegex = re.compile('^[_\-\w\d]+');
def _normalizeKeyword(string):
    result = _normalizeRegex.match(string)
    if result:
        return result.group(0)
    else:
        raise SyntaxError('It was not possible to identify a valid keyword in the received ' \
                        'string. Try with only digits and letters. Received string: "%r"' % string)

class BlocksReader:
    def __init__(self, fileToRead):
        self.f = fileToRead
        self.fd = fileToRead.fileno()
        self.closed = False
        self.__clearBuffer()

    def readBlock(self, blockSize):
        while self.bufferReadBytes < blockSize:
            r, w, x = select.select([self.fd], [], [], _readTimeoutSecs)
            if self.fd not in r:
                return ''
            else:
                partResult = self.f.read(blockSize - self.bufferReadBytes)
                partReadBytes = len(partResult)
                if partReadBytes == 0:
                    self.closed = True
                    return ''
                else:
                    self.buffer += partResult
                    self.bufferReadBytes += partReadBytes
        result = self.buffer
        self.__clearBuffer()
        logging.debug("Receiving %d bytes block: %r" % (blockSize, result))
        return result

    def __clearBuffer(self):
        self.buffer = ''
        self.bufferReadBytes = 0

class ExecutionContext:
    def __init__(self, procIndex, callFifoPath, returnFifoPath, valuesFifoPath):
        self.procIndex = procIndex
        self.lock = threading.Lock()
        self.samplesCond = threading.Condition(self.lock)
        self.errorsDetected = False
        self.currentStep = 0
        self.currentStepVariables = 0
        self.currentStepPositions = 0
        self.totalVariablesQty = 0
        self.totalPositionsQty = 0
        self.variables = []
        self.positions = []
        self.processes = []
        self.callFifoPath = callFifoPath
        self.returnFifoPath = returnFifoPath
        self.valuesFifoPath = valuesFifoPath

    def __enter__(self):
        logging.info("Opening Gerris2Python FIFO at %s" % self.callFifoPath)
        self.callFifo = open(self.callFifoPath, 'r')
        logging.info("Opening Python2Gerris FIFO at %s" % self.returnFifoPath)
        self.returnFifo = open(self.returnFifoPath, 'w',0)
        logging.info("Opening Gerris2Python FIFO for actuation values at %s" % self.valuesFifoPath)
        self.valuesFifo = open(self.valuesFifoPath, 'r')

    def __exit__(self, type, value, traceback):
        if not self.errorsDetected:
            logging.info("Simulation finished. Closing FIFOs...")
            self.__closeFiles()

    def register(self, process):
        self.processes.append(process)

    def clearMetaOnNewStep(self, step):
        with self.lock:
            if self.currentStep < step:
                self.currentStep = step
                self.currentStepVariables = 0
                self.currentStepPositions = 0
                self.variables = []
                self.positions = []

    def nextVariableAndPosition(self):
        with self.lock:
            variable = self.variables[self.currentStepVariables]
            position = self.positions[self.currentStepPositions]
            self.currentStepVariables += 1
            if self.currentStepVariables == self.totalVariablesQty:
                self.currentStepVariables = 0
                self.currentStepPositions += 1
            return (variable, position)

    def addVariable(self, variable, totalVariablesQty):
        self.totalVariablesQty = totalVariablesQty
        if len(self.variables) < totalVariablesQty:
            self.variables.append(variable)
        else:
            msg = "Error detected in variables metadata. Variables exceed the total Qty.: %d" % totalVariablesQty
            raise ValueError(msg)

    def addPosition(self, position, totalPositionsQty):
        self.totalPositionsQty = totalPositionsQty
        if len(self.positions) < totalPositionsQty:
            self.positions.append(position)
        else:
            msg = "Error detected in positions metadata. Positions exceed the total Qty.: %d" % totalPositionsQty
            raise ValueError(msg)

    def notifyError(self, message, exception):
        if exception:
            logging.error(message, exc_info = True)
        else:
            logging.error(message)
        self.errorsDetected = True
        self.__closeFiles()

    def __closeFiles(self):
        self.valuesFifo.close()
        self.returnFifo.close()
        self.callFifo.close()

class ControllerMode(Enum):
    usingPreviousClosestTimeSamples = 1
    usingCurrentSamples = 2

# Thread for waiting for a call from gerris, execute the controller and return the result.
class ControllerThread(threading.Thread):
    Request = Struct('di60s')
    Response = Struct('d')

    def __init__(self, samples, controlModule, context):
        super(ControllerThread,self).__init__()
        self.samples = samples
        self.controlModule = controlModule
        self.context = context
        self.mode = ControllerMode.usingPreviousClosestTimeSamples
        self.defaultActuation = 0.0

    def run(self):
        reader = BlocksReader(self.context.callFifo)
        try:
            self._initControlModule()
            while not reader.closed and not self.context.errorsDetected:
                logging.debug("Controller - Waiting for call request... %d bytes" % self.Request.size)
                query = reader.readBlock(self.Request.size)
                if query:
                    self._processRequest(query)
            logging.info('Call requests FIFO closed. Finishing controller...')
        except Exception as e:
            self.context.notifyError('Controller - Closing with errors: %s' % e, e)
        finally:
            self._destroyControlModule()

    def _initControlModule(self):
        init = self._getFunction('init')
        if init:
            init(self.context.procIndex)

    def _destroyControlModule(self):
        try:
            destroy = self._getFunction('destroy')
        except AttributeError:
            destroy = None
        if destroy:
            try:
                logging.info('Closing python user controller module...')
                destroy(self.context.procIndex)
            except Exception as e:
                logging.error('Error detected during python user controller module closure: %s' % e)

    def _processRequest(self, query):
        try:
            querySt = self.Request.unpack(query)
            time = querySt[0]
            step = querySt[1]
            funcName = _normalizeKeyword(querySt[2])

            skipControl = False
            with self.context.lock:
                expectedSamples = self.context.totalVariablesQty * self.context.totalPositionsQty
                if expectedSamples > 0:
                    allTimesQty = len(self.samples.allTimes)
                    if allTimesQty == 0:
                        skipControl = True
                        logging.debug('Controller - Skipping control actuation because of lack of sampling information or expected variables. SamplesQty=%d ExpectedSamples=%d'
                                      % (allTimesQty, expectedSamples))
                    else:
                        if self.mode is ControllerMode.usingPreviousClosestTimeSamples:
                            samplesTime = self.samples.getPreviousClosestTime(time)
                            if samplesTime is None:
                                skipControl = True
                                logging.debug('Controller - Skipping control actuation because of lack of sampling information prior to current step was detected. Time=%.3f'
                                              % time)
                            else:
                                samplesQty = len(self.samples.samplesByTime(samplesTime))
                        else: #ControllerMode.usingCurrentSamples
                            samplesTime = time
                            if self.samples.currentTime != time:
                                samplesQty = 0
                            else:
                                samplesQty = len(self.samples.currentSamples)
                        if not skipControl and samplesQty < expectedSamples:
                            logging.info('Controller - Waiting for pending samples to be received. CurrentTime=%.3f SamplesTime=%.3f ControllerMode=%s Received=%d Expected=%d'
                                         % (time, samplesTime, self.mode.name, samplesQty, expectedSamples))
                            self.context.samplesCond.wait()

            if skipControl:
                result = self.defaultActuation
            else:
                func = self._getFunction(funcName)
                with self.context.lock:
                    result = func(time, step, self.samples)
            s = self.Response.pack(result)

            logging.debug("Controller - Returning controller result - step=%d - t=%.3f - function=%s - result=%f"
                            % (step, time, funcName, result))
            self.context.returnFifo.write(s)
        except struct.error as e:
            logging.error('Parsing error. Invalid struct data received. Data=%s', query)
            raise

    def _getFunction(self, funcName, failOnError=True):
        logging.debug("Controller - Calling controller - Module=%s Function=\"%r\"" % (self.controlModule, funcName))
        try:
            return getattr(self.controlModule, funcName)
        except AttributeError:
            logging.error("Function \"%r\" not found at \"%s\". DetectedFunctions=%s" % (funcName, self.controlModule, dir(self.controlModule)))
            if failOnError:
                raise
            else:
                return None

# Thread for reading the sent values from gerris.
class CollectorThread(threading.Thread):
    PKG_FORCE_VALUE = 0
    PKG_LOCATION_VALUE = 1
    PKG_META_VARIABLE = 2
    PKG_META_POSITION = 3

    ForceValue = Struct('=di3d3d3d3d')
    LocationValue = Struct('=did')
    LocationMetaVariable = Struct('=dii28s')
    LocationMetaPosition = Struct('=dii3d')

    def __init__(self, samples, context):
        super(CollectorThread,self).__init__()
        self.samples = samples
        self.context = context

        self.pkgTypesMap = {
              self.PKG_FORCE_VALUE: {'struct': self.ForceValue, 'handler': self.handleForce},
              self.PKG_LOCATION_VALUE: {'struct': self.LocationValue, 'handler': self.handleLocation},
              self.PKG_META_VARIABLE: {'struct': self.LocationMetaVariable, 'handler': self.handleMetaVariable},
              self.PKG_META_POSITION: {'struct': self.LocationMetaPosition, 'handler': self.handleMetaPosition}
            }

    def run(self):
        valuesReader = BlocksReader(self.context.valuesFifo)
        try:
            while not valuesReader.closed and not self.context.errorsDetected:
                logging.debug("Collector- Waiting for samples information...")
                pkgType = valuesReader.readBlock(1)
                if pkgType:
                    pkgTypeId = ord(pkgType)
                    logging.debug("Collector- Receiving package type: %d - %r" % (pkgTypeId, pkgType))
                    self._processPacket(valuesReader, pkgTypeId)
            logging.info('Collector - Samples information FIFO closed. Finishing controller...')
        except Exception as e:
            self.context.notifyError('Collector - Closing with errors. %s' % e, e)

    def _processPacket(self, valuesReader, pkgTypeId):
        try:
            typeMap = self.pkgTypesMap[pkgTypeId]
            structType = typeMap['struct']
            handler = typeMap['handler']

            query = ''
            while not query and not valuesReader.closed:
                query = valuesReader.readBlock(structType.size)
            querySt = structType.unpack(query)
            handler(querySt)
        except KeyError:
                msg = 'Invalid struct type. Type=%d is not recognized as a valid package. Available types: %s'\
                       % (pkgTypeId, self.pkgTypesMap.keys())
                logging.error(msg)
                raise SyntaxError(msg)
        except struct.error as e:
            logging.error('Parsing error. Invalid struct data received. Data=%r', query)
            raise


    def handleForce(self, querySt):
        time = querySt[0]
        step = querySt[1]
        pf = (querySt[2],querySt[3],querySt[4])
        vf = (querySt[5],querySt[6],querySt[7])
        pm = (querySt[8],querySt[9],querySt[10])
        vm = (querySt[11],querySt[12],querySt[13])
        sample = Sample(time, step, ForceData(pf, vf, pm, vm))
        logging.debug("Collector- Handling force value. step=%d - time=%.3f - pf=%s - ..."
                      % (step, time, pf))
        with self.context.lock:
            self.samples.addForce(sample)

    def handleLocation(self, querySt):
        time = querySt[0]
        step = querySt[1]
        value = querySt[2]
        (variable, position) = self.context.nextVariableAndPosition()

        sample = Sample(time, step, ProbeData(position, variable, value))
        logging.debug("Collector- Handling position value. step=%d - time=%.3f - position=%s - variable=%s - value=%f"
                      % (step, time, position, variable, value))

        with self.context.lock:
            self.samples.addProbe(sample)
            expectedSamples = self.context.totalVariablesQty * self.context.totalPositionsQty
            if len(self.samples.currentSamples) == expectedSamples:
                logging.info("Collector - Notifying all samples received for time: %.3f. Qty: %d" % (time, expectedSamples))
                self.context.samplesCond.notify()

    def handleMetaPosition(self, querySt):
        time = querySt[0]
        step = querySt[1]
        totalPositionsQty = querySt[2]
        position = (querySt[3], querySt[4], querySt[5])

        self.context.clearMetaOnNewStep(step)
        self.context.addPosition(position, totalPositionsQty)
        logging.info("Collector- Handling meta position. Position=(%.3f, %.3f, %.3f) - TotalPositions=%d" % (position[0], position[1], position[2], totalPositionsQty))

    def handleMetaVariable(self, querySt):
        time = querySt[0]
        step = querySt[1]
        totalVariablesQty = querySt[2]
        variable = _normalizeKeyword(querySt[3])

        self.context.clearMetaOnNewStep(step)
        self.context.addVariable(variable, totalVariablesQty)
        logging.info("Collector- Handling meta variable. Variable=%s - TotalVariables=%d" % (variable, totalVariablesQty))

