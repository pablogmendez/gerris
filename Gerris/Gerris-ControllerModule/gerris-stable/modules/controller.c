#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "controller.h"
#include "output.h"
#include "init.h"
#include "config.h"
#include "pythonCon.h"

#ifdef HAVE_MPI
# include <mpi.h>
#endif /* HAVE_MPI */


/**
 * Forces and moments on the embedded solid boundaries.
 * \beginobject{GfsController}
 */

static gboolean vector_read (GtsFile * fp, FttVector * p)
{
  if (fp->type != GTS_INT && fp->type != GTS_FLOAT) {
    gts_file_error (fp, "expecting a number (p.x)");
    return FALSE;
  }
  p->x = atof (fp->token->str);
  gts_file_next_token (fp);

  if (fp->type != GTS_INT && fp->type != GTS_FLOAT) {
    gts_file_error (fp, "expecting a number (p.y)");
    return FALSE;
  }
  p->y = atof (fp->token->str);
  gts_file_next_token (fp);

  if (fp->type != GTS_INT && fp->type != GTS_FLOAT) {
    gts_file_error (fp, "expecting a number (p.z)");
    return FALSE;
  }
  p->z = atof (fp->token->str);
  gts_file_next_token (fp);
  return TRUE;
}

static void gfs_controller_solid_force_destroy (GtsObject * object)
{
  if (GFS_CONTROLLER_SOLID_FORCE (object)->weight)
    gts_object_destroy (GTS_OBJECT (GFS_CONTROLLER_SOLID_FORCE (object)->weight));

  (* GTS_OBJECT_CLASS (gfs_controller_solid_force_class ())->parent_class->destroy) (object);
  pyConnectorDestroy();
}

static void gfs_controller_solid_force_read (GtsObject ** o, GtsFile * fp)
{
  GfsControllerSolidForce * l = GFS_CONTROLLER_SOLID_FORCE (*o);

  (* GTS_OBJECT_CLASS (gfs_controller_solid_force_class ())->parent_class->read) (o, fp);
  if (fp->type == GTS_ERROR)
    return;

  if (fp->type != '\n') {
    if (!l->weight)
      l->weight = gfs_function_new (gfs_function_class (), 0.);
    gfs_function_read (l->weight, gfs_object_simulation (l), fp);
  }
}

static void gfs_controller_solid_force_write (GtsObject * o, FILE * fp)
{
  GfsControllerSolidForce * l = GFS_CONTROLLER_SOLID_FORCE (o);
  (* GTS_OBJECT_CLASS (gfs_controller_solid_force_class ())->parent_class->write) (o, fp);
  if (l->weight)
    gfs_function_write (l->weight, fp);
}

static gboolean gfs_controller_solid_force_event (GfsEvent * event,
					      GfsSimulation * sim)
{
  if ((* GFS_EVENT_CLASS (GTS_OBJECT_CLASS (gfs_controller_solid_force_class ())->parent_class)->event)
      (event, sim) &&
      sim->advection_params.dt > 0.) {
    pyConnectorInitSim(sim);
    GfsDomain * domain = GFS_DOMAIN (sim);
    FttVector pf, vf, pm, vm;
    gdouble L = sim->physical_params.L, Ln = pow (L, 3. + FTT_DIMENSION - 2.);

    gfs_domain_solid_force (domain, &pf, &vf, &pm, &vm, GFS_CONTROLLER_SOLID_FORCE (event)->weight);

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "step=%d t=%.3f - Sending force information", sim->time.i, sim->time.t);
    pyConnectorSendForce(pf,vf,pm,vm);
    return TRUE;
  }
  return FALSE;
}

static void gfs_controller_solid_force_class_init (GfsEventClass * klass)
{
  GTS_OBJECT_CLASS (klass)->read = gfs_controller_solid_force_read;
  GTS_OBJECT_CLASS (klass)->write = gfs_controller_solid_force_write;
  GTS_OBJECT_CLASS (klass)->destroy = gfs_controller_solid_force_destroy;
  GFS_EVENT_CLASS (klass)->event = gfs_controller_solid_force_event;
}

static void gfs_controller_solid_force_init (GfsControllerLocation * object)
{
//  pyConnectorInit();
}

GfsEventClass * gfs_controller_solid_force_class (void)
{
  static GfsEventClass * klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo gfs_controller_solid_force_info = {
      "GfsControllerSolidForce",
      sizeof (GfsControllerSolidForce),
      sizeof (GfsEventClass),
      (GtsObjectClassInitFunc) gfs_controller_solid_force_class_init,
      (GtsObjectInitFunc) gfs_controller_solid_force_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gfs_event_class ()),
				  &gfs_controller_solid_force_info);
  }

  return klass;
}

/** \endobject{GfsController} */


/**
 * Writing the values of variables at specified locations.
 * \beginobject{GfsControllerLocation}
 */

static gchar default_precision[] = "%g";
static gchar default_tmpfolder[] = "/tmp";

static void gfs_controller_location_destroy (GtsObject * object)
{
  GfsControllerLocation * l = GFS_CONTROLLER_LOCATION (object);
  g_array_free (l->p, TRUE);
  g_free (l->label);
  if (l->precision != default_precision)
    g_free (l->precision);
  if (l->tmp_folder != default_tmpfolder)
    g_free (l->tmp_folder);
  g_free (l->main_controller);
  g_free (l->user_script);

  (* GTS_OBJECT_CLASS (gfs_controller_location_class ())->parent_class->destroy) (object);
  pyConnectorDestroy();
}

static void gfs_controller_location_read (GtsObject ** o, GtsFile * fp)
{
  GfsControllerLocation * l = GFS_CONTROLLER_LOCATION (*o);

  if (GTS_OBJECT_CLASS (gfs_controller_location_class ())->parent_class->read)
    (* GTS_OBJECT_CLASS (gfs_controller_location_class ())->parent_class->read)
      (o, fp);
  if (fp->type == GTS_ERROR)
    return;

  if (fp->type == '{') {
    gchar * label = NULL, * precision = NULL;
    gchar * tmp_folder = NULL, * main_controller = NULL, * user_script = NULL;
    GtsFileVariable var[] = {
      {GTS_STRING, "label", TRUE, &label},
      {GTS_STRING, "precision", TRUE, &precision},
      {GTS_INT,    "interpolate", TRUE, &l->interpolate},
      {GTS_STRING, "tmp-folder", TRUE, &tmp_folder},
      {GTS_STRING, "python-main", TRUE, &main_controller},
      {GTS_STRING, "python-userscript", TRUE, &user_script},
      {GTS_INT,    "samples-window", TRUE, &l->samples_window},
      {GTS_NONE}
    };
    gts_file_assign_variables (fp, var);
    if (fp->type == GTS_ERROR) {
      g_free (label);
      g_free (precision);
      g_free (user_script);
      g_free (tmp_folder);
      g_free (main_controller);
      return;
    }

    if (precision != NULL) {
      if (l->precision != default_precision)
        g_free (l->precision);
      l->precision = precision;
    }

    if (label != NULL) {
      g_free (l->label);
      l->label = label;
    }

    if (tmp_folder != NULL) {
      if (l->tmp_folder != default_tmpfolder)
        g_free (l->tmp_folder);
      l->tmp_folder = tmp_folder;
    }

    if (main_controller == NULL) {
      gts_file_error (fp, "expecting \"python-main\" to locate the main Python script. i.e.: python-main=\"./python/main.py\"");
      return;
    }
    else {
      g_free (l->main_controller);
      l->main_controller = main_controller;
    }

    if (user_script == NULL) {
      gts_file_error (fp, "expecting \"python-userscript\" to locate the custom user script containing the controlling functions. i.e.: python-userscript=\"./python/user/controller.py\"");
      return;
    }
    else {
      g_free (l->user_script);
      l->user_script = user_script;
    }

    if (l->samples_window < 1) {
      gts_file_error (fp, "expecting \"samples-window\" equal or greater than 1");
      return;
    }
  }

  if (fp->type == GTS_STRING) {
    FILE * fptr = fopen (fp->token->str, "r");
    GtsFile * fp1;

    if (fptr == NULL) {
      gts_file_error (fp, "cannot open file `%s'", fp->token->str);
      return;
    }
    fp1 = gts_file_new (fptr);
    while (fp1->type != GTS_NONE) {
      FttVector p;
      if (!vector_read (fp1, &p)) {
	gts_file_error (fp, "%s:%d:%d: %s", fp->token->str, fp1->line, fp1->pos, fp1->error);
	return;
      }
      g_array_append_val (l->p, p);
      while (fp1->type == '\n')
	gts_file_next_token (fp1);
    }
    gts_file_destroy (fp1);
    fclose (fptr);
    gts_file_next_token (fp);
  }
  else if (fp->type == '{') {
    fp->scope_max++;
    do
      gts_file_next_token (fp);
    while (fp->type == '\n');
    while (fp->type != GTS_NONE && fp->type != '}') {
      FttVector p;
      if (!vector_read (fp, &p))
	return;
      g_array_append_val (l->p, p);
      while (fp->type == '\n')
	gts_file_next_token (fp);
    }
    if (fp->type != '}') {
      gts_file_error (fp, "expecting a closing brace");
      return;
    }
    fp->scope_max--;
    gts_file_next_token (fp);
  }
  else {
    FttVector p;
    if (!vector_read (fp, &p))
      return;
    g_array_append_val (l->p, p);
  }

  pyConnectorInit(l->tmp_folder, l->main_controller, l->user_script, l->samples_window);
}

static void gfs_controller_location_write (GtsObject * o, FILE * fp)
{
  GfsControllerLocation * l = GFS_CONTROLLER_LOCATION (o);
  guint i;

  (* GTS_OBJECT_CLASS (gfs_controller_location_class ())->parent_class->write) (o, fp);

  fputs (" {\n", fp);
  if (l->precision != default_precision)
    fprintf (fp, "  precision = %s\n", l->precision);
  if (l->label)
    fprintf (fp, "  label = \"%s\"\n", l->label);
  if (!l->interpolate)
    fputs ("  interpolate = 0\n", fp);
  if (l->tmp_folder != default_tmpfolder)
    fprintf( fp, "  tmp_folder = \"%s\"\n", l->tmp_folder);
  fprintf (fp, "  python-main= \"%s\"\n", l->main_controller);
  fprintf (fp, "  python-userscript = \"%s\"\n", l->user_script);
  fprintf (fp, "  samples-window = %d\n", l->samples_window);
  fputc ('}', fp);

  fputs (" {\n", fp);
  gchar * format = g_strdup_printf ("%s %s %s\n", l->precision, l->precision, l->precision);
  for (i = 0; i < l->p->len; i++) {
    FttVector p = g_array_index (l->p, FttVector, i);
    fprintf (fp, format, p.x, p.y, p.z);
  }
  g_free (format);
  fputc ('}', fp);
}

static void do_send_locations(GfsSimulation* sim,
                GfsVariable* nonEmptyVariables, guint variablesQty,
                FttVector* locations, guint locationsQty,
                double* valuesPerLocation)
{
    pyConnectorSendLocationsMetadata(nonEmptyVariables, variablesQty, locations, locationsQty);
    gchar** msgLocsAndVars = malloc(sizeof(gchar*) * (locationsQty + variablesQty + 3));
    gint iMsg = 0;
    msgLocsAndVars[iMsg++] = g_strdup("Locations:");
    for(guint i = 0; i < locationsQty; ++i) {
        FttVector p = locations[i];
        msgLocsAndVars[iMsg++] = g_strdup_printf("(%.2f, %.2f, %.2f)",  p.x, p.y, p.z);
    }
    msgLocsAndVars[iMsg++] = g_strdup("- Variables:");
    for (guint i = 0; i < variablesQty; ++i) {
        GfsVariable* v = &nonEmptyVariables[i];
        msgLocsAndVars[iMsg++] = g_strdup_printf("%s", v->name);
    }
    msgLocsAndVars[iMsg++] = NULL;
    gchar* msg = g_strjoinv(" ", msgLocsAndVars);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "step=%d t=%.3f - Sending probes information to controller. %s", sim->time.i, sim->time.t, msg);

    for (guint i = 0; i < (iMsg-1); ++i)
        g_free(msgLocsAndVars[i]);
    free(msgLocsAndVars);
    g_free(msg);

    for(guint iLocation = 0; iLocation < locationsQty; ++iLocation){
        FttVector p = locations[iLocation];
        for (guint iVariable = 0; iVariable < variablesQty; ++iVariable) {
            GfsVariable* v = &nonEmptyVariables[iVariable];
            double d = valuesPerLocation[iLocation * variablesQty + iVariable];
            pyConnectorSendLocation(v->name, d, p);
        }
    }
}

static gboolean gfs_controller_location_event (GfsEvent * event,
					   GfsSimulation * sim)
{
   if ((* GFS_EVENT_CLASS (GTS_OBJECT_CLASS (gfs_controller_location_class ())->parent_class)->event)
      (event, sim)) {
    pyConnectorInitSim(sim);
    GfsDomain * domain = GFS_DOMAIN (sim);
    GfsControllerLocation * location = GFS_CONTROLLER_LOCATION (event);
    guint i;

    guint maxVariables = g_slist_length(domain->variables);
    GfsVariable* nonEmptyVariables = (GfsVariable*)malloc(sizeof(GfsVariable) * maxVariables);

    guint locationsQty = location->p->len;
    FttVector* locations = malloc(sizeof(FttVector) * locationsQty);
    for(i = 0; i < locationsQty; ++i) {
        locations[i] = g_array_index (location->p, FttVector, i);
    }
    guint variablesQty = 0;
    GSList* vars = domain->variables;
    while (vars) {
        GfsVariable* v = vars->data;
        if (v->name)
            nonEmptyVariables[variablesQty++] = *v;
        vars = vars->next;
    }

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int* currentIndexes = (int*)malloc(sizeof(int) * locationsQty);
    int* allIndexes = (int*)malloc(sizeof(int) * locationsQty * world_size);
    double* currentValues = (double*)malloc(sizeof(double) * variablesQty * locationsQty);
    double* allValues = (double*)malloc(sizeof(double) * variablesQty * locationsQty * world_size);

    for(i = 0; i < locationsQty; ++i) {
        currentIndexes[i] = -1;
    }

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d t=%.3f - Collecting probes information. ProbesQty=%d VariablesQty=%d", sim->time.i, sim->time.t, locationsQty, variablesQty);
    gchar* collectedLocationsStr = g_strdup("");

    gint iIndex = -1;

    for (i = 0; i < locationsQty; i++) {
      FttVector p = g_array_index (location->p, FttVector, i);
      FttVector pm = p;

      gfs_simulation_map (sim, &pm);
      FttCell * cell = gfs_domain_locate (domain, pm, -1, NULL);

      if (cell != NULL) {
          ++iIndex;
          currentIndexes[iIndex] = i;
          gchar* aux = collectedLocationsStr;
          collectedLocationsStr = g_strdup_printf("%s Loc%d=(%.2f,%.2f,%.2f)", aux, i, p.x, p.y, p.z);
          g_free(aux);

          for(gint iVariable = 0; iVariable < variablesQty; ++iVariable) {
              GfsVariable * v = &nonEmptyVariables[iVariable];
              double d = gfs_dimensional_value(v,
                                 location->interpolate ?
                                 gfs_interpolate (cell, pm, v) : GFS_VALUE (cell, v)
                             );
              currentValues[iIndex * variablesQty + iVariable] = d;
          }
       }
    }
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "step=%d t=%.3f - Sharing probes information with sibling processes. ProbesQty=%d ProbesCollectedInProcess=%d - %s",
            sim->time.i, sim->time.t, locationsQty, iIndex + 1, collectedLocationsStr);
    g_free(collectedLocationsStr);

    MPI_Allgather(currentIndexes, locationsQty, MPI_INT, allIndexes, locationsQty, MPI_INT, MPI_COMM_WORLD);
    MPI_Allgather(currentValues, locationsQty * variablesQty, MPI_DOUBLE, allValues, locationsQty * variablesQty, MPI_DOUBLE, MPI_COMM_WORLD);

    double* valuesPerLocation = (double*)malloc(sizeof(double) * variablesQty * locationsQty);
    for(i = 0; i < locationsQty * world_size; ++i) {
      int iLocation = allIndexes[i];
      if (iLocation >= 0) {
        for(guint iVariable = 0; iVariable < variablesQty; ++iVariable) {
          double value = allValues[i * variablesQty + iVariable];
          valuesPerLocation[iLocation * variablesQty + iVariable] = value;
        }
      }
    }

    do_send_locations(sim, nonEmptyVariables, variablesQty, locations, locationsQty, valuesPerLocation);

    free(locations);
    free(nonEmptyVariables);
    free(currentIndexes);
    free(currentValues);
    free(allIndexes);
    free(allValues);
    free(valuesPerLocation);
  }
  return FALSE;
}

static void gfs_controller_location_class_init (GfsEventClass * klass)
{
  GFS_EVENT_CLASS (klass)->event = gfs_controller_location_event;
  GTS_OBJECT_CLASS (klass)->destroy = gfs_controller_location_destroy;
  GTS_OBJECT_CLASS (klass)->read = gfs_controller_location_read;
  GTS_OBJECT_CLASS (klass)->write = gfs_controller_location_write;
}

static void gfs_controller_location_init (GfsControllerLocation * object)
{
  object->p = g_array_new (FALSE, FALSE, sizeof (FttVector));
  object->precision = default_precision;
  object->interpolate = TRUE;
  object->tmp_folder = default_tmpfolder;
  object->samples_window = 1;
}

GfsEventClass * gfs_controller_location_class (void)
{
  static GfsEventClass * klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo gfs_controller_location_info = {
      "GfsControllerLocation",
      sizeof (GfsControllerLocation),
      sizeof (GfsEventClass),
      (GtsObjectClassInitFunc) gfs_controller_location_class_init,
      (GtsObjectInitFunc) gfs_controller_location_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gfs_event_class ()),
				  &gfs_controller_location_info);
  }

  return klass;
}

/** \endobject{GfsControllerLocation} */

/* Initialize module */

/* only define gfs_module_name for "official" modules (i.e. those installed in
   GFS_MODULES_DIR) */
const gchar gfs_module_name[] = "controller";
const gchar * g_module_check_init (void);

const gchar * g_module_check_init (void)
{
  gfs_init_log(G_LOG_DOMAIN, G_DEBUG);
  gfs_controller_solid_force_class ();
  gfs_controller_location_class ();
  return NULL;
}
