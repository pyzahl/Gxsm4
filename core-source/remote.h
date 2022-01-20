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

#ifndef REMOTE__H
#define REMOTE__H


#define REMO_PIPEBROKEN          0
#define REMO_CMD                 1
#define REMO_INVALID_CMD         2
#define REMO_QUIT                3

#define CMDFKTARGS gchar **arglist, gpointer data

#include "remoteargs.h"

enum REMOTE_TYP { REMO_ACTION, REMO_MENUACTION, REMO_SETENTRY, REMO_CBACTION, REMO_ENDLIST };

typedef struct {
        gchar  *cmd;
        void (*RemoteCb)(GtkWidget *widget , gpointer data);
        GtkWidget *widget;
        gpointer data;
} remote_action_cb;

typedef struct {
        char     *cmd;
        int      (*CmdFkt)(CMDFKTARGS);
        gpointer data;
} remote_cmd;

typedef struct {
        REMOTE_TYP typ;
        char       *verb;
        remote_cmd *cmd_list;
} remote_verb;

class remote{
public:
        remote();
        virtual ~remote();

        void rcopen(gchar *crtlfifo, gchar *statfifo=NULL);
        void rcclose();

        int writestatus(gchar *mld);
        gboolean readfifo(GIOChannel *src);
        virtual int eval(gchar *cmdstr);

        void wait(gint w){ waitflg=w; };
        gint waiting(){ return waitflg; };
        virtual void waitfkt(){};
protected:
        gint remotefifo;
        gint statusfifo;
private:
        gint waitflg;
};

class remote_crtl : public remote{
public:
        remote_crtl(remote_verb *rvlist);
        virtual ~remote_crtl();

        virtual int eval(gchar *cmdstr);
        virtual int addcmd(gchar **cmdargvlist)=0;
        virtual int checkentry(gchar **cmdargvlist)=0;
        virtual int checkactions(gchar **cmdargvlist)=0;
        virtual void waitfkt(){};

        void senderror(const char *errstr);
        void sendcmd(const char *cmd, char *arg, gchar **arglist);
        void quit();

        int ExecAction(gchar **arglist);

protected:
private:
        int FindFktExec(const gchar *cmd, const gchar *arg, gchar **arglist);

        remote_verb *rlist;
        remote_cmd *rfkt;
        char *rarg;
};

#endif

