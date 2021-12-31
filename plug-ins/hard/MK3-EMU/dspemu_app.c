/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "string.h"
#include <gtk/gtk.h>

#include "dspemu_app.h"
#include "dspemu_app_window.h"
#include "dspemu_app_prefs.h"

#include "FB_spm_statemaschine.h"
#include "FB_spm_dataexchange.h"
#include "dataprocess.h"

// POINTER TO MMAPED AND SHARED DSP EMU MEMORY FOR DATA
DSP_EMU_INTERNAL_MEMORY *dspmem = NULL;
        
struct _DSPEmuApp
{
        GtkApplication parent;
};

typedef struct DSPEmuAppPrivate DSPEmuAppPrivate;

struct DSPEmuAppPrivate
{
        GThread* dspemu_thread;
        gboolean dspemu_pflg;

        GSettings *settings;
};

G_DEFINE_TYPE_WITH_PRIVATE(DSPEmuApp, dspemu_app, GTK_TYPE_APPLICATION);



#define handle_error(msg)                                       \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

int
map_dsp_memory (const gchar *sr_emu_dev_file)
{
        void *addr;
        int ret;
        static int shm_fd = 0;
        struct stat sb;

        size_t length = sizeof (DSP_EMU_INTERNAL_MEMORY);
        off_t  offset = 0;
        off_t  pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

        if (sr_emu_dev_file) {

                /* create the shared memory segment */
                g_message ("DSPEMU: creating %s", sr_emu_dev_file);
                shm_fd = shm_open (sr_emu_dev_file, O_CREAT | O_TRUNC | O_RDWR, 0666);
                if (shm_fd == -1){
                        handle_error("open");
                        return -1;
                }
                
                /* configure the size of the shared memory segment */
                ret = ftruncate(shm_fd, length);
                if (ret == -1){
                        handle_error("truncate and set size");
                        return -1;
                }

                if (fstat(shm_fd, &sb) == -1){           /* To obtain file size */
                        handle_error("fstat");
                        return -1;
                }
                g_message ("shm mmap file size: %d", sb.st_size);
                if (offset >= sb.st_size) {
                        fprintf(stderr, "offset is past end of file\n");
                        exit(EXIT_FAILURE);
                }


                /* map the shared memory segment in the address space of the process */
                g_message ("DSPEMU: mapping dspmem size=%lx", length);
                addr = mmap (NULL,
                             length + offset - pa_offset,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, shm_fd, pa_offset);
                if (addr == MAP_FAILED){
                        handle_error("mmap");
                        return -1;
                }
                
                dspmem = (DSP_EMU_INTERNAL_MEMORY*) addr;

                return 0;
        } else {
                if (shm_fd){
                        g_message ("DSPEMU: unmapping dspmem and closing");
                        addr = (void*) dspmem;
                        dspmem = NULL;
                        munmap (addr, length + offset - pa_offset);
                        close (shm_fd);
                        return 0;
                }
                return -1;
        }
}

static void
dspemu_app_init (DSPEmuApp *app)
{
        DSPEmuAppPrivate *priv;

        priv = dspemu_app_get_instance_private (app);

        priv->dspemu_pflg = 0;
}

gpointer dspemu_dataprocess_thread (void *env){
        DSPEmuAppPrivate *emu = (DSPEmuAppPrivate*)env;;
        emu->dspemu_pflg = 1;
        do {
                dsp_idle_loop ();
                dataprocess ();
                //g_message ("%d", dspmem->magic.life);
        } while (emu->dspemu_pflg);
        return NULL;
}


static void
start_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       app)
{
        DSPEmuAppPrivate *emu;
        emu = dspemu_app_get_instance_private (app);

        if (!emu->dspemu_pflg){
                emu->dspemu_thread = g_thread_new ("dspemu_dataprocess_thread", dspemu_dataprocess_thread, emu);
        }
}

static void
stop_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
        DSPEmuAppPrivate *emu;
        emu = dspemu_app_get_instance_private (app);

        if (emu->dspemu_pflg){
                emu->dspemu_pflg = 0;
                g_thread_join (emu->dspemu_thread);
        }
}

gchar *watch_magic (){
        static int count = 0;
        return g_strdup_printf ("magic ** DSPEMU SIZEOF POINTER DSP_INT32 = %d bits\n"
                                ".magic:     %04x\n"
                                ".version:   %04x\n"
                                ".year,mmdd: %04x %04x\n"
                                ".softid:    %04x\n"
                                ".statemachine:   %08x\n"
                                ".AIC_in:         %08x\n"
                                ".AIC_out:        %08x\n"
                                ".analog:         %08x\n"
                                ".signal_monitor: %08x\n"
                                ".feedback_mixer: %08x\n"
                                ".z_servo:        %08x\n"
                                ".m_servo:        %08x\n"
                                ".scan:           %08x\n"
                                ".move:           %08x\n"
                                ".probe:          %08x\n"
                                ".signal_lookup:  %08x\n"
                                " S00  %08x %08x %08x %08x\n"
                                " S04  %08x %08x %08x %08x\n"
                                " S08  %08x %08x %08x %08x\n"
                                " S12  %08x %08x %08x %08x\n"
                                " -----\n"
                                ".life: %d\n"
                                " *** %d *** ",
                                (int)(sizeof (DSP_INT32_P) * 8),
                                dspmem->magic.magic,
                                dspmem->magic.version,
                                dspmem->magic.year, dspmem->magic.mmdd,
                                dspmem->magic.dsp_soft_id,
                                dspmem->magic.statemachine,
                                dspmem->magic.AIC_in,
                                dspmem->magic.AIC_out,
                                dspmem->magic.analog,
                                dspmem->magic.signal_monitor,
                                dspmem->magic.feedback_mixer,
                                dspmem->magic.z_servo,
                                dspmem->magic.m_servo,
                                dspmem->magic.scan,
                                dspmem->magic.move,
                                dspmem->magic.probe,
                                dspmem->magic.signal_lookup,
                                dspmem->dsp_signal_lookup[0],dspmem->dsp_signal_lookup[1],
                                dspmem->dsp_signal_lookup[2],dspmem->dsp_signal_lookup[3],
                                dspmem->dsp_signal_lookup[4],dspmem->dsp_signal_lookup[5],
                                dspmem->dsp_signal_lookup[6],dspmem->dsp_signal_lookup[7],
                                dspmem->dsp_signal_lookup[8],dspmem->dsp_signal_lookup[9],
                                dspmem->dsp_signal_lookup[10],dspmem->dsp_signal_lookup[11],
                                dspmem->dsp_signal_lookup[12],dspmem->dsp_signal_lookup[13],
                                dspmem->dsp_signal_lookup[14],dspmem->dsp_signal_lookup[15],
                                dspmem->magic.life,
                                count++
                                );
}

gchar *watch_state (){
        static int count = 0;
        return g_strdup_printf ("state\n"
                                ".mode: 0x%4x\n"
                                ".DSP_time: %d\n"
                                "sig_mon:\n"
                                ".mindex:    %d\n"
                                ".signal_id: %d\n"
                                ".act_address_input_set: 0x%12x\n"
                                ".act_address_signal:    0x%12x\n"
                                " *** %d *** ",
                                dspmem->state.mode,
                                dspmem->state.DSP_time,
                                dspmem->sig_mon.mindex,
                                dspmem->sig_mon.signal_id,
                                dspmem->sig_mon.act_address_input_set,
                                dspmem->sig_mon.act_address_signal,
                                count++
                                );
}

gchar *watch_scan (){
        static int count = 0;
        return g_strdup_printf ("move\n"
                                ".xy:    %8d  %8d         p:%d\n"
                                "scan\n"
                                ".xyz:   %8d  %8d  %8d    p:%d\n"
                                ".xy_r:  %8d  %8d\n"
                                " *** %d *** ",
                                dspmem->move.xyz_vec[0], dspmem->move.xyz_vec[1], dspmem->move.pflg,
                                dspmem->scan.xyz_vec[0], dspmem->scan.xyz_vec[1], dspmem->scan.xyz_vec[2], dspmem->scan.pflg,
                                dspmem->scan.xy_r_vec[0], dspmem->scan.xy_r_vec[1],
                                count++
                                );
}

gchar *watch_analog (){
        static int count = 0;
        return g_strdup_printf ("analog\n"
                                ".bias:  %8d\n"
                                ".motor: %8d\n"
                                ".out:   %6d %6d %6d %6d  %6d %6d %6d %6d  %6d %6d\n"
                                ".in:    %6d %6d %6d %6d  %6d %6d %6d %6d\n"
                                ".counter: %10d %10d\n"
                                " *** %d *** ",
                                dspmem->analog.bias,
                                dspmem->analog.motor,
                                dspmem->analog.out[0].s,dspmem->analog.out[1].s,dspmem->analog.out[2].s,dspmem->analog.out[3].s,
                                dspmem->analog.out[4].s,dspmem->analog.out[5].s,dspmem->analog.out[6].s,dspmem->analog.out[7].s,
                                dspmem->analog.out[8].s,dspmem->analog.out[9].s,
                                dspmem->analog.in[0],dspmem->analog.in[1],dspmem->analog.in[2],dspmem->analog.in[3],
                                dspmem->analog.in[4],dspmem->analog.in[5],dspmem->analog.in[6],dspmem->analog.in[7],
                                dspmem->analog.counter[0],dspmem->analog.counter[1],
                                count++
                                );
}

gchar *watch_other (){
        static int count = 0;
        return g_strdup_printf ("Z-servo\n"
                                ".cp:          %f\n"
                                ".ci:          %f\n"
                                ".setpoint:    %8d\n"
                                ".input:       %8d\n"
                                ".delta:       %8d\n"
                                ".i_sum:       %8d\n"
                                ".control:     %8d\n"
                                ".neg_control: %8d\n"
                                ".watch:       %d\n"
                                " *** %d *** ",
                                dspmem->z_servo.cp/32767.,
                                dspmem->z_servo.ci/32767.,
                                dspmem->z_servo.setpoint,
                                *dspmem->z_servo.input,
                                dspmem->z_servo.delta,
                                dspmem->z_servo.i_sum,
                                dspmem->z_servo.control,
                                dspmem->z_servo.neg_control,
                                dspmem->z_servo.watch,
                                count++
                                );
}

static void
watch_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       app)
{
        DSPEmuAppWindow *win = NULL;
        GList *windows = gtk_application_get_windows (GTK_APPLICATION (app));
        if (windows)
                win = DSPEMU_APP_WINDOW (windows->data);
        else
                win = dspemu_app_window_new (DSPEMU_APP (app));

        //GVariant *old_state = g_action_get_state (G_ACTION (action));
        GVariant *new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        if (!strcmp (g_variant_get_string (new_state, NULL), "magic")){
                dspemu_app_window_insert (win,
                                          "Magic",
                                          watch_magic);
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "state")){
                dspemu_app_window_insert (win,
                                          "State",
                                          watch_state);
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "scan")){
                dspemu_app_window_insert (win,
                                          "Scan",
                                          watch_scan);
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "analog")){
                dspemu_app_window_insert (win,
                                          "Analog",
                                          watch_analog);
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "other")){
                dspemu_app_window_insert (win,
                                          "Other",
                                          watch_other);
        }

        gtk_window_present (GTK_WINDOW (win));
}

static void
preferences_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
  DSPEmuAppPrefs *prefs;
  GtkWindow *win;

  win = gtk_application_get_active_window (GTK_APPLICATION (app));
  prefs = dspemu_app_prefs_new (DSPEMU_APP_WINDOW (win));
  gtk_window_present (GTK_WINDOW (prefs));
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
        g_application_quit (G_APPLICATION (app));
        map_dsp_memory (NULL);
}

static GActionEntry app_entries[] =
        {
                { "start", start_activated, NULL, NULL, NULL },
                { "stop", stop_activated, NULL, NULL, NULL },
                { "watch", watch_activated,  "s", "'none'", NULL },
                { "preferences", preferences_activated, NULL, NULL, NULL },
                { "quit", quit_activated, NULL, NULL, NULL }
        };

static void
dspemu_app_startup (GApplication *app)
{
        GtkBuilder *builder;
        GMenuModel *app_menu;
        const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };

        G_APPLICATION_CLASS (dspemu_app_parent_class)->startup (app);

        g_action_map_add_action_entries (G_ACTION_MAP (app),
                                         app_entries, G_N_ELEMENTS (app_entries),
                                         app);
        gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                               "app.quit",
                                               quit_accels);

        builder = gtk_builder_new_from_resource ("/org/gtk/mk3dspemu/app-menu.ui");
        app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu"));
        gtk_application_set_app_menu (GTK_APPLICATION (app), app_menu);
        g_object_unref (builder);
}

static void
dspemu_app_activate (GApplication *app)
{
        DSPEmuAppWindow *win;

        win = dspemu_app_window_new (DSPEMU_APP (app));
        gtk_window_present (GTK_WINDOW (win));
}

static void
dspemu_app_open (GApplication  *app,
                 GFile        **files,
                 gint           n_files,
                 const gchar   *hint)
{
        GList *windows;
        DSPEmuAppWindow *win;
        int i;

        windows = gtk_application_get_windows (GTK_APPLICATION (app));
        if (windows)
                win = DSPEMU_APP_WINDOW (windows->data);
        else
                win = dspemu_app_window_new (DSPEMU_APP (app));

        for (i = 0; i < n_files; i++)
                dspemu_app_window_open (win, files[i]);

        gtk_window_present (GTK_WINDOW (win));
}

static void
dspemu_app_class_init (DSPEmuAppClass *class)
{
        G_APPLICATION_CLASS (class)->startup = dspemu_app_startup;
        G_APPLICATION_CLASS (class)->activate = dspemu_app_activate;
        G_APPLICATION_CLASS (class)->open = dspemu_app_open;
}

DSPEmuApp *
dspemu_app_new (void)
{
        return g_object_new (DSPEMU_APP_TYPE,
                             "application-id", "org.gtk.dspemuapp",
                             "flags", G_APPLICATION_HANDLES_OPEN,
                             NULL);
}
