#ifndef __GXSM4APPWIN_H
#define __GXSM4APPWIN_H

#include <gtk/gtk.h>
#include "gxsm_app.h"


#define GXSM4_APP_WINDOW_TYPE (gxsm4_app_window_get_type ())
#define GXSM4_APP_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GXSM4_APP_WINDOW_TYPE, Gxsm4appWindow))


typedef struct _Gxsm4appWindow         Gxsm4appWindow;
typedef struct _Gxsm4appWindowClass    Gxsm4appWindowClass;


GType                 gxsm4_app_window_get_type     (void);
Gxsm4appWindow       *gxsm4_app_window_new          (Gxsm4app *app);
gboolean              gxsm4_app_window_open         (Gxsm4appWindow *win,
						     GFile            *file,
						     gboolean in_active_channel);


#endif /* __GXSM4APPWIN_H */
