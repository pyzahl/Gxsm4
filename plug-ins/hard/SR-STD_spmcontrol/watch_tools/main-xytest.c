/*
 * SPM-DU-meter -- A DAC units meter for SRanger
 * Copyright (C) 1998 Gregory McLean
 * Copyright (C) 2003 Percy Zahl, added bidirectional meter and SRanger DSP watch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 
 * 02139, USA.
 *
 * Modified April 9 1999, by Dave J. Andruczyk to fix time sync problems
 * now it should be pretty much perfectly in sync with the audio.  Adjust
 * the "lag" variable below to taste if you don't like it...  Utilized
 * code from "extace" to get the desired effects..
 * 
 * Small additions April 14th 1999, by Dave J. Andruczyk to fix the missing 
 * peaks problem that happened with quick transients not showin on the vu-meter.
 * Now it will catch pretty much all of them.
 * 
 * Eye candy!
 */
#include <config.h>
#include <gnome.h>
//#include <esd.h> //unused, removed on 31.08.2011 stm
#include <gtkledbar.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#include <gtkdatabox.h>

#define POINTS 1000
#define PI M_PI

static gfloat *srdataX = NULL;
static gfloat *srdataY[4] = { NULL, NULL };
gint sound = -1;
gint sranger = -1;

gint scope_mode = 1;
gfloat watch, watch_xy[2];

unsigned short magic[64];

/* A fairly good size buffer to keep resource (cpu) down */
#define NSAMP  2048
#define BUFS  8
#define NUM_BARS 10
short        aubuf[BUFS][NSAMP];
GtkWidget   *ledbar_pos[NUM_BARS];
GtkWidget   *ledbar_neg[NUM_BARS];
GtkWidget   *ledbar_dig[NUM_BARS];
GtkDatabox  *xyscope;
gchar       *labels[NUM_BARS] = { "X0", "Y0", "Z0", "XS", "YS", "ZS", "BI", "FB", "al", "F%" };
//gchar       *labels[NUM_BARS] = { "X", "Y", "Z", "B", "F", "a", "%" };
GtkWidget   *window;
//gchar       *esd_host = NULL; // ununsed, removed on 31.08.2011 stm
gint 	    curbuf = 0;
gint 	    lag = 2;
gint 	    locount = 0;
gint 	    plevel_l = 0;
gint 	    plevel_r = 0;
gint        maxlevel = 32767.;

gboolean    need_swap = FALSE;

/* function prototypes to make gcc happy: */
void update (void);
char *itoa (int i);
void open_dsp (void);
gint update_display (gpointer data);


void swapshort(short *addr)
{
        unsigned short temp1,temp2;
 
        temp1 = temp2 = *addr;
        *addr = ((temp2 & 0xFF) << 8) | ((temp1 >> 8) & 0xFF);
}
 
void swaplong (long *addr){
    long temp1, temp2, temp3, temp4;
 
    temp1 = (*addr)       & 0xFF;
    temp2 = (*addr >> 8)  & 0xFF;
    temp3 = (*addr >> 16) & 0xFF;
    temp4 = (*addr >> 24) & 0xFF;
 
    *addr = (temp1 << 24) | (temp2 << 16) | (temp3 << 8) | temp4;
}

char 
*itoa (int i)
{
    static char ret[ 30 ];
    sprintf (ret, "%d", i);
    return ret;
}

/* byte swap big/little endian */
void swap (short *addr){
	unsigned short temp1, temp2;
	temp1 = temp2 = *addr;
	*addr = ((temp2 & 0xFF) << 8) | ((temp1 >> 8) & 0xFF);
}

static gint
save_state (GnomeClient *client, gint phase, GnomeRestartStyle save_style,
	    gint shutdown, GnomeInteractStyle inter_style, gint fast,
	    gpointer client_data)
{
    gchar *prefix = gnome_client_get_config_prefix (client);
    gchar *argv[]= { "rm", "-r", NULL };  
    gint   xpos;
    gint   ypos;

    gnome_config_push_prefix (prefix);

    gdk_window_get_geometry (window->window, &xpos, &ypos, NULL);
    gnome_config_set_int ("Geometry/x", xpos);
    gnome_config_set_int ("Geometry/y", ypos);

    gnome_config_pop_prefix ();
    gnome_config_sync();

    argv[2]= gnome_config_get_real_path (prefix);
    gnome_client_set_discard_command (client, 3, argv);

    return TRUE;
}

void 
open_dsp (void)
{
	int i;
	sranger = open ("/dev/sranger0", O_RDWR);
	if (sranger < 0)
	{
			GtkWidget *box;
			box = gnome_error_dialog(N_("Cannot connect to sranger.\nPlease connect USB and load module."));
			gnome_dialog_run(GNOME_DIALOG(box));
			exit(1);
	}

#define SR_MAGIC_ADR 0x4000
#define SR_MAGIC     0xEE01
#define SRANGER_SEEK_DATA_SPACE 1

#define SR_SPM_SOFT_ID   0x1001 /* FB_SPM_SOFT_ID */

  // now read magic struct data
  lseek (sranger, SR_MAGIC_ADR, SRANGER_SEEK_DATA_SPACE);
  read (sranger, magic, sizeof (magic)); 
  
  need_swap = FALSE;
  if (magic[0] != SR_MAGIC){
	  swapshort (&magic[0]);
	  if (magic[0] == SR_MAGIC){
		  need_swap = TRUE;
		  for (i=1; i<19; ++i)
			  swapshort (&magic[i]);
	  }
  }

  if (magic[0] != SR_MAGIC || magic[4] != SR_SPM_SOFT_ID)
  {
		  GtkWidget *box;
		  box = (GtkWidget*) gnome_error_dialog(N_("Error, DSP Magic or SoftID is wrong or host is not PPC."));
		  gnome_dialog_run(GNOME_DIALOG(box));
		  exit(1);
  }
}

static gboolean
srdata_idle_func (GtkDatabox * box)
{
   gfloat freq;
   gfloat off;
   gchar label[10];
   gint i;

   if (!GTK_IS_DATABOX (box))
      return FALSE;

   for (i = 0; i < POINTS; i++)
   {
	   if (i < POINTS-1){
		   srdataY[0][i] = srdataY[0][i+1];
		   srdataY[1][i] = srdataY[1][i+1];
		   srdataY[2][i] = srdataY[2][i+1];
	   }
	   else{
		   srdataY[0][i] = watch;
		   srdataY[1][i] = watch_xy[0];
		   srdataY[2][i] = watch_xy[1];
	   }
   }


   gtk_databox_redraw (GTK_DATABOX (box));

   return TRUE;
}

GtkDatabox* create_srdata (void)
{
   GtkWidget *window = NULL;
   GtkWidget *box1;
   GtkWidget *close_button;
   GtkWidget *box;
   GtkWidget *label;
   GtkWidget *separator;
   GdkColor color;
   gint i,j;

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_size_request (window, 300, 300);

   g_signal_connect (G_OBJECT (window), "destroy",
		     G_CALLBACK (gtk_main_quit), NULL);

   gtk_window_set_title (GTK_WINDOW (window), "SR-Data Z-Watch");
   gtk_container_set_border_width (GTK_CONTAINER (window), 0);

   box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_container_add (GTK_CONTAINER (window), box1);

   box = gtk_databox_new ();
   g_signal_connect (G_OBJECT (box), "destroy",
		     G_CALLBACK (gtk_databox_data_destroy_all), NULL);

   srdataX = g_new0 (gfloat, POINTS);

   for (j=0; j<4; ++j){
	   srdataY[j] = g_new0 (gfloat, POINTS);

	   for (i = 0; i < POINTS; i++)
	   {
		   srdataX[i] = (gfloat) i;
		   srdataY[j][i] = 0.;
	   }
	   srdataY[j][0] = 32767.;
	   srdataY[j][1] = -32767.;
   }
   color.red = 65535;
   color.green = 65535;
   color.blue = 0;

   gtk_databox_data_add_x_y (GTK_DATABOX (box), POINTS,
			     srdataX, srdataY[0], color,
			     GTK_DATABOX_LINES, 0);

   color.red = 65535;
   color.green = 30000;
   color.blue = 0;
   gtk_databox_data_add_x_y (GTK_DATABOX (box), POINTS,
			     srdataX, srdataY[1], color,
			     GTK_DATABOX_LINES, 0);

   color.red = 30000;
   color.green = 65535;
   color.blue = 0;
   gtk_databox_data_add_x_y (GTK_DATABOX (box), POINTS,
			     srdataX, srdataY[2], color,
			     GTK_DATABOX_LINES, 0);

   color.red = 0;
   color.green = 0;
   color.blue = 32768;
   gtk_databox_set_background_color (GTK_DATABOX (box), color);

   gtk_databox_rescale (GTK_DATABOX (box));

   gtk_box_pack_start (GTK_BOX (box1), box, TRUE, TRUE, 0);

   gtk_widget_show_all (window);
   return (box);
}

void setup_level (int no, short value, double max){
	gchar *dig = g_strdup_printf (" %6d", value);
	gdouble level = abs(value)/(double)max;
	if (value > 0){
		led_bar_light_percent (ledbar_pos[no], level);
		led_bar_light_percent (ledbar_neg[no], 0.);
	} else {
		led_bar_light_percent (ledbar_neg[no], level);
		led_bar_light_percent (ledbar_pos[no], 0.);
	}
	
	gtk_label_set_text ((GtkLabel*)ledbar_dig[no], dig);
	g_free (dig);
}

gint
update_display (gpointer data)
{
	int i, bar=0;
	short dspdata[8];

	// read DSP AIC-out data
	lseek (sranger, magic[7], 1);
	read (sranger, dspdata, sizeof (dspdata)); 

	if (need_swap)
		for (i=0; i<8; ++i)
			swapshort (&dspdata[i]);

//	printf("X%6d Y%6d Z%6d\n",dspdata[3],dspdata[4],dspdata[5]);

	setup_level (bar++, dspdata[0], maxlevel); // X0
	setup_level (bar++, dspdata[1], maxlevel); // Y0
	setup_level (bar++, dspdata[2], maxlevel); // Z0
	setup_level (bar++, dspdata[3], maxlevel); // XS
	setup_level (bar++, dspdata[4], maxlevel); // YS
	setup_level (bar++, dspdata[5], maxlevel); // ZS

	watch = (gfloat) dspdata[5];
	watch_xy[0] = (gfloat) dspdata[3];
	watch_xy[1] = (gfloat) dspdata[4];

	setup_level (bar++, dspdata[6], maxlevel); // Bias

	// read DSP AIC-in data
	lseek (sranger, magic[6], 1);
	read (sranger, dspdata, sizeof (dspdata)); 

	if (need_swap)
		for (i=0; i<8; ++i)
			swapshort (&dspdata[i]);

	setup_level (bar++, dspdata[5], maxlevel); // Feedback-src
	setup_level (bar++, dspdata[1], maxlevel); // a

	// read DSP FIFO-fill data
	lseek (sranger, magic[15], 1);
	read (sranger, dspdata, sizeof (dspdata)); 

	if (need_swap)
		for (i=0; i<5; ++i)
			swapshort (&dspdata[i]);

	i = dspdata[1] - dspdata[0];
	if (i < 0) i += dspdata[4];
	setup_level (bar++, (short)i, (double)dspdata[4]); // FIFO-fill


	srdata_idle_func (xyscope);

	return TRUE;
}

int 
main (int argc, char *argv[])
{
    GnomeClient   *client;
    GtkWidget     *hbox,*meterbox,*lab;
    GtkWidget     *frame;
    gint          time_id;
    gint          i;
    gint          session_xpos = -1;
    gint          session_ypos = -1;
    gint          orient = 0;
    gint          update_ms = 50;

    struct poptOption options[] = 
    {
	{ NULL, 'x', POPT_ARG_INT, NULL, 0, 
	  N_("Specify the X position of the meter."), 
	  N_("X-Position") },
	{ NULL, 'y', POPT_ARG_INT, NULL, 0, 
	  N_("Specify the Y position of the meter."), 
	  N_("Y-Position") },
	{ NULL, 'v', POPT_ARG_NONE, NULL, 0, 
	  N_("Open a vertical version of the meter."), NULL },
	{ NULL, 'u', POPT_ARG_INT, NULL, 0, 
	  N_("Specify meter update period in ms, 50ms is default."), N_("Update period [ms]") },
	{ NULL, 'm', POPT_ARG_INT, NULL, 0, 
	  N_("Specify Max Level for audio signals (32767 is max, 32767 is default)."), N_("Max Level") },
	{ NULL, '\0', 0, NULL, 0 }
    };
    options[0].arg = &session_xpos;
    options[1].arg = &session_ypos;
    options[2].arg = &orient;
    options[3].arg = &update_ms;
    options[4].arg = &maxlevel;

//    bindtextdomain (PACKAGE, GNOMELOCALEDIR);
//    textdomain (PACKAGE);
    gnome_init_with_popt_table ("SPM DU Meter", "0.1", argc, argv, options, 
				0, NULL);
    client = gnome_master_client ();
    g_object_ref (G_OBJECT (client));
    g_object_sink (G_OBJECT (client));
    g_signal_connect (G_OBJECT (client), "save_yourself",
			G_CALLBACK (save_state), argv[0]);
    g_signal_connect (G_OBJECT (client), "die",
			G_CALLBACK (gtk_main_quit), argv[0]);

    if (gnome_client_get_flags (client) & GNOME_CLIENT_RESTORED)
    {
	gnome_config_push_prefix (gnome_client_get_config_prefix (client));

	session_xpos = gnome_config_get_int ("Geometry/x");
	session_ypos = gnome_config_get_int ("Geometry/y");

	gnome_config_pop_prefix ();
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
//    gnome_window_icon_set_from_file (GTK_WINDOW (window),
//				     GNOME_ICONDIR"/gnome-vumeter.png");
    gtk_window_set_title (GTK_WINDOW (window), N_("SPM DU Meter"));
    if (session_xpos >=0 && session_ypos >= 0)
	gtk_widget_set_uposition (window, session_xpos, session_ypos);

    g_signal_connect (G_OBJECT (window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (G_OBJECT (window), "delete_event",
			G_CALLBACK (gtk_main_quit), NULL);

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_border_width (GTK_CONTAINER (frame), 4);
    gtk_container_add (GTK_CONTAINER (window), frame);

    if ( !orient )
			hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    else
			hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_border_width (GTK_CONTAINER (hbox), 5);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    for (i = 0; i < NUM_BARS; i++){
			if ( !orient )
					meterbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			else
					meterbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

			lab = gtk_label_new (labels[i]);
			gtk_widget_set_size_request (lab, 50, -1);
			gtk_box_pack_start (GTK_BOX (meterbox), lab, FALSE, FALSE, 0);

			ledbar_pos[i] = led_bar_new (25, orient, 0);
			ledbar_neg[i] = led_bar_new (25, orient, 1);
			ledbar_dig[i] = gtk_label_new ("+000000");
			if ( !orient ){
				gtk_box_pack_start (GTK_BOX (meterbox), ledbar_neg[i], FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (meterbox), ledbar_pos[i], FALSE, FALSE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (meterbox), ledbar_pos[i], FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (meterbox), ledbar_neg[i], FALSE, FALSE, 0);
			}
			gtk_box_pack_start (GTK_BOX (meterbox), ledbar_dig[i], FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), meterbox, FALSE, FALSE, 0);
    }
    gtk_widget_show_all (window);
    open_dsp ();

    time_id = gtk_timeout_add (update_ms, (GtkFunction)update_display, NULL);

    xyscope = create_srdata ();

    gtk_main ();
    g_object_unref (G_OBJECT (client));

    gtk_timeout_remove (time_id);

    return 0;
}
