#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pythonCon.h>
#ifdef HAVE_MPI
 #include <mpi.h>
#endif

#define FUNC_MAX_LENGTH 60
#define VAR_MAX_LENGTH 28
#define DEFAULT_SEND_FIFO_NAME "gerris2python_request"
#define DEFAULT_RECV_FIFO_NAME "python2gerris_response"
#define DEFAULT_VALUES_FIFO_NAME "gerris2python_values"

#define PKG_FORCE_VALUE 0
#define PKG_LOCATION_VALUE 1
#define PKG_META_VARIABLE 2
#define PKG_META_POSITION 3


static char connectorInitialized = 0;
static py_connector_t connector;


typedef struct {
    size_t* sizes;
    size_t sizes_qty;
    size_t total_size;
    size_t unpack_offset;
    size_t value_index;
    void** values;
    char* packed_values;
} packet_t;

void packet_create(packet_t* self, ...)
{
    self->total_size = 0;
    va_list list_sizes;
    va_start(list_sizes, self);
    bool more_args = true;
    size_t i = 0;
    for(i = 0; more_args; ++i)
    {
        size_t size = va_arg(list_sizes, size_t);
        more_args = (size > 0);
    }
    va_end(list_sizes);
    self->sizes_qty = i;

    self->sizes = (size_t*)malloc(sizeof(size_t) * self->sizes_qty);
    va_start(list_sizes, self);
    for(i = 0; i < self->sizes_qty; ++i)
    {
        size_t size = va_arg(list_sizes, size_t);
        self->total_size += size;
        self->sizes[i] = size;
    }
    va_end(list_sizes);
    self->unpack_offset= 0;
    self->value_index = 0;
    self->values = (void**)malloc(sizeof(void*) * self->sizes_qty);
    self->packed_values = NULL;
}

void packet_add(packet_t* self, void* value) {
    self->values[self->value_index++] = value;
}

const char* packet_get_pack(packet_t* self)
{
    if (!self->packed_values)
    {
        self->packed_values = (char*)malloc(self->total_size);
        size_t offset = 0;
        for(size_t i = 0; self->sizes[i] > 0; ++i) {
            memcpy(self->packed_values + offset, self->values[i], self->sizes[i]);
            offset += self->sizes[i];
        }
    }
    return self->packed_values;
}

size_t packet_get_pack_size(packet_t* self) {
    return self->total_size;
}

void packet_unpack(packet_t* self, void* value)
{
    if (self->packed_values)
    {
        if (self->value_index > 0)
            self->unpack_offset += self->sizes[self->value_index - 1];
        size_t size = self->sizes[self->value_index++];
        memcpy(value, self->packed_values + self->unpack_offset, size);
    }
}

void packet_send(packet_t* self, int FD)
{
    const char* toSend = packet_get_pack(self);
    size_t bytesToSend = packet_get_pack_size(self);

    if (bytesToSend >= 4)
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"Sending %zu bytes: [%02x %02x ... %02x %02x]", bytesToSend,
                toSend[0], toSend[1], toSend[bytesToSend - 2], toSend[bytesToSend - 1]);
    else
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"Sending %zu bytes", bytesToSend);

    size_t sentBytes = 0;
    while(sentBytes < bytesToSend)
    {
        int result = write(FD, toSend + sentBytes, bytesToSend - sentBytes);
        if (result < 1)
        {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,"Fail on PyConnector Send - SentBytes=%zu BytesToSend=%zu", sentBytes, bytesToSend);
            break;
        }
        else
            sentBytes += result;
    }
}

void packet_receive(packet_t* self, int FD)
{
    if (!self->packed_values)
        self->packed_values = (char*)malloc(self->total_size);

    char* toReceive = self->packed_values;
    size_t bytesToReceive = packet_get_pack_size(self);

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"Receiving %zu bytes", bytesToReceive);

    size_t receivedBytes = 0;
    while(receivedBytes < bytesToReceive)
    {
        int result = read(FD, toReceive + receivedBytes, bytesToReceive - receivedBytes);
        if (result < 1)
        {
            g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,"Fail on PyConnector Receive - ReceivedBytes=%zu BytesToReceive=%zu", receivedBytes, bytesToReceive);
            break;
        }
        else
            receivedBytes += result;
    }
}

void packet_destroy(packet_t* self) {
    free(self->values);
    if (self->packed_values)
        free(self->packed_values);
}


static void py_connector_init_fifos(py_connector_t* self);
static void py_connector_init_controller(py_connector_t* self);
static void py_connector_get_step_time(py_connector_t* self, gint* step, double* time);
static void py_connector_check();

void py_connector_init(py_connector_t* self,
                       gchar * tmpFolder, gchar * mainController, gchar * userScript, guint samplesWindow ) {
    self->worldRank = 0;
    self->sim = NULL;
    self->tmpFolder = tmpFolder;
    self->mainController = mainController;
    self->userScript = userScript;
    self->samplesWindow = samplesWindow;

#ifdef HAVE_MPI
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    self->worldRank = world_rank;
#endif

    GDateTime* now = g_date_time_new_now_local();
    gchar* nowStr = g_date_time_format(now, "%Y%m%d_%H%M%S");
    g_date_time_unref(now);
    pid_t pid = getpid();
    size_t nowPidRankSize = strlen(nowStr) + 1 + 7 + 1 + 2;
    size_t folderSize = strlen(self->tmpFolder);
    size_t sendNameSize = folderSize + 1 + sizeof(DEFAULT_SEND_FIFO_NAME) + 1 + nowPidRankSize + 1;
    size_t recvNameSize = folderSize + 1 + sizeof(DEFAULT_RECV_FIFO_NAME) + 1 + nowPidRankSize + 1;
    size_t valuesNameSize = folderSize + 1 + sizeof(DEFAULT_VALUES_FIFO_NAME) + 1 + nowPidRankSize + 1;
    self->sendFifoName = (char*)malloc(sendNameSize);
    self->recvFifoName = (char*)malloc(recvNameSize);
    self->valuesFifoName = (char*)malloc(valuesNameSize);
    snprintf(self->sendFifoName, sendNameSize, "%s/%s_%s_%07d_%02d", self->tmpFolder, DEFAULT_SEND_FIFO_NAME, nowStr, pid, self->worldRank);
    snprintf(self->recvFifoName, recvNameSize, "%s/%s_%s_%07d_%02d", self->tmpFolder, DEFAULT_RECV_FIFO_NAME, nowStr, pid, self->worldRank);
    snprintf(self->valuesFifoName, valuesNameSize, "%s/%s_%s_%07d_%02d", self->tmpFolder, DEFAULT_VALUES_FIFO_NAME, nowStr, pid, self->worldRank);
    self->sendFifoName[sendNameSize-1] = '\0';
    self->recvFifoName[recvNameSize-1] = '\0';
    self->valuesFifoName[valuesNameSize-1] = '\0';
    g_free (nowStr);

    self->cache = g_hash_table_new(g_str_hash, g_str_equal);

    py_connector_init_controller(self);
    py_connector_init_fifos(self);
}

void py_connector_init_simulation(py_connector_t* self, GfsSimulation* sim) {
    if (!self->sim && sim) {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d - t=%.3f - Defining Simulation for py_connector", sim->time.i, sim->time.t);
        self->sim = sim;
    }
}
static void py_connector_init_controller(py_connector_t* self) { 
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Starting Python controller at '%s'. TmpFolder='%s' UserScript='%s' SamplesWindow=%d", 
              self->mainController, self->tmpFolder, self->userScript, self->samplesWindow);
    pid_t pid = fork();
    if (pid) {
        self->pythonControllerPID = pid;
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Python controller started.");
    }
    else {
        char worldRankStr[3];
        snprintf(worldRankStr, 3, "%02d", self->worldRank);
        char samplesWindowStr[VAR_MAX_LENGTH];
        snprintf(samplesWindowStr, VAR_MAX_LENGTH, "%d", self->samplesWindow);
        execl(self->mainController, "main", "--script", self->userScript, "--samples", samplesWindowStr, "--mpiproc", worldRankStr,
              "--requestfifo", self->sendFifoName, "--returnfifo", self->recvFifoName, "--samplesfifo", self->valuesFifoName,
              "--loglevel", G_DEBUG, NULL);
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Python controller couldn't start properly.");
        exit(EXIT_FAILURE);
    }
}

static void py_connector_init_fifos(py_connector_t* self) {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Creating FIFOs: %s, %s", self->sendFifoName, self->recvFifoName);
    struct stat st;
    if (stat(self->sendFifoName, &st) == 0)
        unlink(self->sendFifoName);
    if (stat(self->recvFifoName, &st) == 0)
        unlink(self->recvFifoName);
	if (stat(self->valuesFifoName, &st) == 0)
        unlink(self->valuesFifoName);

    mkfifo(self->sendFifoName, 0666);
    mkfifo(self->recvFifoName, 0666);
    mkfifo(self->valuesFifoName, 0666);

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "FIFOs created. Opening FDs");
    self->sendFD = open(self->sendFifoName, O_WRONLY);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "FIFO created at %s.", self->sendFifoName);
    self->recvFD = open(self->recvFifoName, O_RDONLY);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "FIFO created at %s.", self->recvFifoName);
    self->valuesFD = open(self->valuesFifoName, O_WRONLY);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "FIFO created at %s.", self->valuesFifoName);
}

static void py_connector_clear_cache(py_connector_t* self) {
    GList* values = g_hash_table_get_values(self->cache);
    while (values) {
        g_free(values->data);
        values = values->next;
    }
    g_hash_table_remove_all (self->cache);
}

void py_connector_destroy(py_connector_t* self) {
    free(self->sendFifoName);
    free(self->recvFifoName);
    free(self->valuesFifoName);
    py_connector_clear_cache(self);
    g_hash_table_destroy(self->cache);

    close(self->sendFD);
    close(self->recvFD);
    close(self->valuesFD);
    unlink(self->sendFifoName);
    unlink(self->recvFifoName);
    unlink(self->valuesFifoName);
}

double py_connector_get_value(py_connector_t* self, char* function) {
    double value = 0;
	char* cachedValue = g_hash_table_lookup(self->cache, function);
	if(cachedValue == NULL){
        gint step;
        double time;
        py_connector_get_step_time(self, &step, &time);

        packet_t pkg;
        packet_create(&pkg, sizeof(double), sizeof(int32_t), sizeof(char) * FUNC_MAX_LENGTH, 0);
        packet_add(&pkg, &time);
        packet_add(&pkg, &step);
        packet_add(&pkg, function);
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "step=%d - t=%.3f - Sending call request for function=%s.", step, time, function);
        packet_send(&pkg, self->sendFD);
        packet_destroy(&pkg);

        packet_t result;
        packet_create(&result, sizeof(double), 0);
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d - t=%.3f - Receiving actuation response...", step, time);
        packet_receive(&result, self->recvFD);
        packet_unpack(&result, &value);
        packet_destroy(&result);

        gchar* newCachedValue = g_strdup_printf("%f", value);
        g_hash_table_insert(self->cache, function, newCachedValue);
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,"step=%d - t=%.3f - Actuation response received - Function=%s Result=%f", step, time, function, value);
	}
    else
        value = atof(cachedValue);

    return value;
}

void py_connector_send_force(py_connector_t* self, FttVector pf, FttVector vf, FttVector pm, FttVector vm) {
    gint step;
    double time;
    py_connector_get_step_time(self, &step, &time);

    packet_t pkg;
    packet_create(&pkg, sizeof(char), sizeof(double), sizeof(int32_t),
                            sizeof(double), sizeof(double), sizeof(double),
                            sizeof(double), sizeof(double), sizeof(double),
                            sizeof(double), sizeof(double), sizeof(double),
                            sizeof(double), sizeof(double), sizeof(double), 0);
    char type = PKG_FORCE_VALUE;
    packet_add(&pkg, &type);
    packet_add(&pkg, &time);
    packet_add(&pkg, &step);
    packet_add(&pkg, &pf.x);
    packet_add(&pkg, &pf.y);
    packet_add(&pkg, &pf.z);
    packet_add(&pkg, &vf.x);
    packet_add(&pkg, &vf.y);
    packet_add(&pkg, &vf.z);
    packet_add(&pkg, &pm.x);
    packet_add(&pkg, &pm.y);
    packet_add(&pkg, &pm.z);
    packet_add(&pkg, &vm.x);
    packet_add(&pkg, &vm.y);
    packet_add(&pkg, &vm.z);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d - t=%.3f - Sending pf=(%.3f,%.3f,%.3f), vm=(%.3f,%.3f,%.3f)", 
                                            step, time, pf.x, pf.y, pf.z, vm.x, vm.y, vm.z);
    packet_send(&pkg, self->valuesFD);
    packet_destroy(&pkg);
    py_connector_clear_cache(self);
}

void py_connector_send_location(py_connector_t* self, char* var, double value, FttVector p){
    gint step;
    double time;
    py_connector_get_step_time(self, &step, &time);

    packet_t pkg;
    packet_create(&pkg, sizeof(char), sizeof(double), sizeof(int32_t), sizeof(double), 0);
    char type = PKG_LOCATION_VALUE;
    packet_add(&pkg, &type);
    packet_add(&pkg, &time);
    packet_add(&pkg, &step);
    packet_add(&pkg, &value);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d - t=%.3f - Sending %s=%f - (x,y,z)=(%f, %f, %f)", step, time, var, value, p.x, p.y, p.z);
    packet_send(&pkg, self->valuesFD);
    packet_destroy(&pkg);
    g_hash_table_remove_all(connector.cache);
}

void py_connector_send_locations_metadata(py_connector_t* self, GfsVariable* variables, size_t variablesQty, FttVector* locations, size_t locationsQty){
    gint step;
    double time;
    py_connector_get_step_time(self, &step, &time);

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d - t=%.3f - Sending locations metadata. VariablesQty=%zu - LocationsQty=%zu", step, time, variablesQty, locationsQty);
    for(size_t i = 0; i < variablesQty; ++i) {
        packet_t pkg;
        packet_create(&pkg, sizeof(char), sizeof(double), sizeof(int32_t), sizeof(uint32_t), sizeof(char) * VAR_MAX_LENGTH, 0);
        char type = PKG_META_VARIABLE;
        packet_add(&pkg, &type);
        packet_add(&pkg, &time);
        packet_add(&pkg, &step);
        packet_add(&pkg, &variablesQty);
        packet_add(&pkg, variables[i].name);
        packet_send(&pkg, self->valuesFD);
        packet_destroy(&pkg);
    }

    for(size_t i = 0; i < locationsQty; ++i) {
        packet_t pkg;
        packet_create(&pkg, sizeof(char), sizeof(double), sizeof(int32_t), sizeof(uint32_t), sizeof(double), sizeof(double), sizeof(double), 0);
        char type = PKG_META_POSITION;
        packet_add(&pkg, &type);
        packet_add(&pkg, &time);
        packet_add(&pkg, &step);
        packet_add(&pkg, &locationsQty);
        packet_add(&pkg, &locations[i].x);
        packet_add(&pkg, &locations[i].y);
        packet_add(&pkg, &locations[i].z);
        packet_send(&pkg, self->valuesFD);
        packet_destroy(&pkg);
    }
}

static void py_connector_get_step_time(py_connector_t* self, gint* step, double* time) {
    if (!self->sim)
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"No simulation defined before calling py_connector_get_value. System will resume and use controller results anyway assuming t=0.");
    *step = self->sim ? self->sim->time.i : 0;
    *time = self->sim ? self->sim->time.t : 0;
}

static void py_connector_check() {
    if (!connectorInitialized)
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Python connector wasn't initialized. Further processing and controller actions may depend in unitialized components.");
}

void pyConnectorInit(gchar * tmpFolder, gchar * mainController, gchar * userScript, guint samplesWindow) {
    if (!connectorInitialized) {
        py_connector_init(&connector, tmpFolder, mainController, userScript, samplesWindow);
        connectorInitialized = 1;
    }
}

void pyConnectorInitSim(GfsSimulation* sim){
    py_connector_check();
    py_connector_init_simulation(&connector, sim);
}

void pyConnectorDestroy(){
    if (connectorInitialized) {
        py_connector_destroy(&connector);
        connectorInitialized = 0;
    }
}

double controller(char* function){
    py_connector_check();
    return py_connector_get_value(&connector, function);
}

void pyConnectorSendForce(FttVector pf, FttVector vf, FttVector pm, FttVector vm){
    py_connector_check();
    py_connector_send_force(&connector, pf, vf, pm, vm);
}

void pyConnectorSendLocation(char* var, double value, FttVector p){
    py_connector_check();
    py_connector_send_location(&connector, var, value, p);
}

void pyConnectorSendLocationsMetadata(GfsVariable* variables, size_t variablesQty, FttVector* locations, size_t locationsQty) {
    py_connector_check();
    py_connector_send_locations_metadata(&connector, variables, variablesQty, locations, locationsQty);
}
