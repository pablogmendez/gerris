/* Gerris - The GNU Flow Solver
 * Copyright (C) 2001 National Institute of Water and Atmospheric Research
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifndef __INIT_H__
#define __INIT_H__

#include <gts.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef G_DEBUG
 #define G_DEBUG "info"
 #pragma message "warning: No G_DEBUG definition detected during compilation.\n\nDefault value set to: info. \n" \
    "You can change it by adding the proper compilation flag. i.e.: \n\n" \
    "make CFLAGS='-ggdb -Og -DG_DEBUG=\\\"debug\\\"'"
#endif

GtsObjectClass ** gfs_classes             (void);
void              gfs_init_log (const gchar *logDomain,
                                const gchar *logLevels);
void              gfs_init                (int * argc, 
					   char *** argv);
void gfs_catch_floating_point_exceptions   (void);
int  gfs_restore_floating_point_exceptions (void);
void gfs_disable_floating_point_exceptions (void);
void gfs_enable_floating_point_exceptions  (void);

#define gfs_restore_fpe_for_function(f) \
       { \
         if (gfs_restore_floating_point_exceptions ()) { \
           g_message ("floating-point exception in user-defined function:\n%s", \
	              gfs_function_description (f, FALSE)); \
           exit (1); \
         } \
       }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INIT_H__ */
