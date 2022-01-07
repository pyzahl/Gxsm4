/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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


#include <iostream>
#include <iomanip>
#include <time.h>
#include "monitor.h"
#include "meldungen.h"
#include "unit.h"
#include "xsmdebug.h"
#include "gxsm_monitor_vmemory_and_refcounts.h"

extern "C++" {

#ifdef GXSM_MONITOR_VMEMORY_USAGE
extern gint global_ref_counter[];

const gchar *grc_name[] = {
        "UnitObj",
        "ScanDataObj",
        "ScanObj",
        "ZData",
        "LayerInfo",
        "Mem2d",
        "Mem2dSco",
        "VObj",
        "CairoItem",
        "ProfileObj",
        "ProfileCtrl",
        "PrbHdrArr",
        "PrbVecArr",
        NULL
};

gchar* Monitor::get_vmem_and_refcounts_info (){
        gchar *tmp;
        gint64 t=g_get_real_time ();
        gchar *info = g_strdup_printf ("RealTime: %21ld us %10.3f s VmSize: %14d kB RefCounts: ",
                                       t, (double)(t-t0)*1e-6,
                                       getValue ("VmSize:"));
        for (int i=0; i<GXSM_GRC_LAST && grc_name[i]; ++i){
                tmp=g_strdup_printf (" %s: %6d", grc_name[i], global_ref_counter[i]);
                info = g_strconcat (info, tmp, NULL);
                g_free (tmp);
        }
        return info;
}

gint Monitor::auto_log_timeout_func (gpointer data){
        Monitor* m = (Monitor*)data;
        if (m){
                gchar *info = m->get_vmem_and_refcounts_info ();
                m->LogEvent ("GXSM-Vm-Status", info, 3);
                // only this is visible in monitor buffer
                g_free (info);
        }
        return true;
}

gint Monitor::parseLine (char* line){
        // This assumes that a digit will be found and the line ends in " Kb".
        gint i = strlen(line);
        const char* p = line;
        while (*p <'0' || *p > '9') p++;
        line[i-3] = '\0';
        i = atoi(p);
        return i;
}

// use: getValue ("VmSize:");
gint Monitor::getValue (const gchar *what){ //Note: this value is in KB!
        FILE* file = fopen("/proc/self/status", "r");
        gint result = -1;
        gchar line[128];
        
        while (fgets(line, 128, file) != NULL){
                if (strncmp(line, what, strlen(what)) == 0){
                        result = parseLine(line);
                        break;
                }
        }
        fclose(file);
        return result;
}

#endif

Monitor::Monitor(gint loglevel){
        dt=10.0;
        t0=g_get_real_time ();        

#ifdef GXSM_MONITOR_VMEMORY_USAGE
        vmem_auto_log_interval_seconds = 10;
#else
        vmem_auto_log_interval_seconds = 0;
#endif
        
        set_logging_level (loglevel);
        
        char buf[64];
        time_t t;
        time(&t);               // 012345678901234567890123456789
        strcpy(buf, ctime(&t)); // Thu Jun 25 12:44:55 1998
        buf[7]=0; buf[24]=0;
        logname = g_strdup_printf("Ev_%3s%4s.log", &buf[4], &buf[20]);
        XSM_DEBUG (DBG_L2, "LogName:>" << logname << "<" );

        for(int i=0; i < MAXMONITORFIELDS; ++i)
                Fields[i] = NULL;

        LogEvent("Monitor", "startup");

#ifdef GXSM_MONITOR_VMEMORY_USAGE
        g_timeout_add_seconds (vmem_auto_log_interval_seconds, (GSourceFunc) auto_log_timeout_func, this);
#endif
}

Monitor::~Monitor(){
        if(logname)
                g_free(logname);
        for(gchar **field = Fields; *field; ++field)
                g_free(*field);
}

void Monitor::SetLogName(char *name){
        if(logname)
                g_free(logname);
        logname = g_strdup(name);
}

void Monitor::Messung(double val, gchar *txt){
        gchar *result = g_strdup_printf ("%8.4f", val);
        LogEvent (txt, result);
        g_free (result);
}

void Monitor::PutLogEvent(const gchar *Action, const gchar *Entry, gint r){

        for(gchar **field = Fields; *field; ++field){
                g_free(*field);
                *field=NULL;
        }
        
        time_t t;
        time(&t);
        Fields[0] = g_strdup(ctime(&t)); Fields[0][24]=' ';
        Fields[1] = g_strdup_printf("%12s : ",Action);
        Fields[2] = g_strdup_printf("%s : ",Entry);
        Fields[3] = NULL; //g_strdup_printf("%10.6f ",0.0);
        Fields[4] = NULL; //g_strdup_printf("%10.6f ",0.0);
        Fields[5] = NULL; //g_strdup_printf("%10.6f ",0.0);
        Fields[6] = NULL;

        AppLine();

#ifdef GXSM_MONITOR_VMEMORY_USAGE
        if (r==0){ // only if no recursion
                // add VmInfo line, this only goes into the log file
                gchar *info = get_vmem_and_refcounts_info ();
                PutLogEvent ("GXSM-Vm-Status", info, 1); // recursion control=1
                g_free (info);
        }
#endif
}

gint Monitor::AppLine(){
        // Autologging
        if(logname){
                std::ofstream f;
                f.open(logname, std::ios::out | std::ios::app);
                if(!f.good()){
                        std::cerr << ERR_SORRY << "\n" << ERR_FILEWRITE << ": " << logname << std::endl;
                        return 1;
                }
    
                for(gchar **field = Fields; *field; ++field)
                        f << *field;

                f << "\n";
                f.close();
        }
        return 0;
}
        
} // extern C++

// END
