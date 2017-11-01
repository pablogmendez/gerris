#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "event.h"

/* GfsControllerSolidForce: Header */

typedef struct _GfsControllerSolidForce         GfsControllerSolidForce;

struct _GfsControllerSolidForce {
  /*< private >*/
  GfsEvent parent;

  /*< public >*/
  GfsFunction * weight;
};

#define GFS_CONTROLLER_SOLID_FORCE(obj)            GTS_OBJECT_CAST (obj,\
                                                 GfsControllerSolidForce,\
                                                 gfs_controller_solid_force_class ())

GfsEventClass * gfs_controller_solid_force_class (void);


/* GfsControllerLocation: Header */

typedef struct _GfsControllerLocation         GfsControllerLocation;

struct _GfsControllerLocation {
  /*< private >*/
  GfsEvent parent;

  /*< public >*/
  GArray * p;
  gchar * precision, * label;
  gchar * user_script, * tmp_folder, * main_controller;
  gint samples_window;
  gboolean interpolate;
};

#define GFS_CONTROLLER_LOCATION(obj)            GTS_OBJECT_CAST (obj,\
					         GfsControllerLocation,\
					         gfs_controller_location_class ())
#define GFS_IS_CONTROLLER_LOCATION(obj)         (gts_object_is_from_class (obj,\
						 gfs_controller_location_class ()))

GfsEventClass * gfs_controller_location_class  (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONTROLLER_H__ */
