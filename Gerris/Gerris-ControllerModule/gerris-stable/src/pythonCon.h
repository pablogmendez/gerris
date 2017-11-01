#ifndef __PYTHON_CONTROLLER_CONECTION_H__
#define __PYTHON_CONTROLLER_CONECTION_H__
#include "ftt.h"
#include <stdint.h>
#include <glib.h>
#include "simulation.h"
#include "init.h"

typedef struct {
  gchar * tmpFolder;
  gchar * mainController;
  gchar * userScript;
  guint samplesWindow;
  int worldRank;
  char* sendFifoName;
  char* recvFifoName;
  char* valuesFifoName;
  int sendFD;
  int recvFD;
  int valuesFD;
  GHashTable* cache;
  pid_t pythonControllerPID;
  GfsSimulation * sim;
} py_connector_t;


void py_connector_init(py_connector_t* self, gchar * tmpFolder, gchar * mainController, gchar * userScript, guint samplesWindow);
void py_connector_init_simulation(py_connector_t* self, GfsSimulation* sim);
void py_connector_destroy(py_connector_t* self);
double py_connector_get_value(py_connector_t* self, char* function);
void py_connector_send_force(py_connector_t* self, FttVector pf, FttVector vf, FttVector pm, FttVector vm);
void py_connector_send_location(py_connector_t* self, char* var, double value, FttVector p);
void py_connector_send_locations_metadata(py_connector_t* self, GfsVariable* variables, size_t variablesQty, FttVector* locations, size_t locationsQty);


/**
* Python Communicator initialization routine. Creates pipes and open file descriptors for external communication.
*/
void pyConnectorInit(gchar * tmpFolder, gchar * mainController, gchar * userScript, guint samplesWindow);

/**
* Defines the current simulation for Python Communicator usage.
*/
void pyConnectorInitSim(GfsSimulation* sim);

/**
* Finalizes the Python Communicator and handles open resources. Destroy pipes and close file descriptors.
*/
void pyConnectorDestroy();

/**
* Get a control value from external scripts via the Python Communicator connection. Called by the C code compiled dinamically from the simulation file.
*/
double controller(char* function);

/**
* Send force values to external scripts via Python Communicator connection. Values are read by ControllerSolidForce in the external scripts.
*/
void pyConnectorSendForce(FttVector pf, FttVector vf, FttVector pm, FttVector vm);

/**
* Send locations and variables metadata to prepare the receiver controller to collect the values properly.
*/
void pyConnectorSendLocationsMetadata(GfsVariable* variables, size_t variablesQty, FttVector* locations, size_t locationsQty);

/**
* Send location values to external scripts via Python Communicator connection. Values are read by ControllerLocation in the external scripts.
*/
void pyConnectorSendLocation(char* var, double value, FttVector p);

#endif
