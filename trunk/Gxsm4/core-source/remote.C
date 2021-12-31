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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

// #include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>


#include "glbvars.h"
#include "remote.h"


// remote:
// basics of remote control
// - open / close fifo
// - reading and splitting comands
// - error check
remote::remote(){
  waitflg=FALSE;
  remotefifo=0;
  statusfifo=0;
}

remote::~remote(){ 
  rcclose(); 
}

void remote::rcopen(gchar *crtlfifo, gchar *statfifo){ 
  if(!remotefifo){ // only once !!
    remotefifo = open(crtlfifo, O_RDONLY);
  }
  if(!statusfifo && statfifo){ // only once !!
    statusfifo = open(statfifo, O_WRONLY);
  }
}

void remote::rcclose(){ 
  if(remotefifo){
    close(remotefifo);
    remotefifo=0;
  }
  if(statusfifo){
    close(statusfifo);
    statusfifo=0;
  }
}

int remote::writestatus(gchar *mld){
  if(statusfifo){
    return write(statusfifo, mld, strlen(mld));
  }
  return 0;
}

int remote::readfifo(GIOChannel *src){
	gchar *cmdstring = g_new (gchar, 1024);
//	gchar *cmdstring = NULL;
	gchar **cmdlist, **cmd;
	gsize size;
	memset (cmdstring, 0, 1024);
//	GError *error = NULL; // needs glibc2.x ...
	
	XSM_DEBUG(DBG_L2, "**** remote::readfifo read()" );

//	g_io_channel_read_to_end (src, &cmdstring, &size, &error);
	size=read(remotefifo, cmdstring, 1024);

	if(size){
		XSM_DEBUG(DBG_L2, "**** remote::readfifo DATA:@"<<cmdstring<<"@" );
		cmdlist = g_strsplit(cmdstring, "\n", 100);
		cmd = cmdlist;
		
		while(*cmd)
			eval(*cmd++);
		
		g_strfreev(cmdlist);
		XSM_DEBUG(DBG_L2, "**** remote::readfifo eval done " );
	}
	g_free (cmdstring);
	XSM_DEBUG(DBG_L2, "**** remote::readfifo return " );
	return TRUE;
}

int remote::eval(char *cmdstr){
  XSM_DEBUG(DBG_L2, "Remote: got cmd =>" << cmdstr << "<=" );
  return 0;
}

// Task is to check remote input and pe-parse it
int remote_crtl::eval(gchar *cmdstr){
  int n;
  gchar **cmdargv, **p, *r, *rr;

  // strip cmdstring
  // convert Spaces to Tabs (\t), Quotes "\ " are remaining 
  for(rr=cmdstr, r=cmdstr; *r; ++r){
    if(rr==cmdstr && (*r == ' ' || *r == '\t')) continue;
    if(*r == '\\' && *(r+1)){ *rr++ = *++r; continue; }
    if(*r == ' ' || *r == '\t'){ 
      *rr++ = '\t'; 
      while(*(r+1) == ' ') ++r; 
      continue; 
    }
    *rr++ = *r;
  }
  while(*rr) *rr++ = ' ';
  g_strstrip(cmdstr);

  cmdargv = g_strsplit(cmdstr, "\t", 10);

  for(n=0, p=cmdargv; *p; ++p, ++n);

  if(n>1){
    addcmd(cmdargv);
    sendcmd(cmdargv[0], cmdargv[1], cmdargv);
  }else{
    senderror("Sorry, please be a bit more verbose !");
  }

  g_strfreev(cmdargv);
  return 0;
}

remote_crtl::remote_crtl(remote_verb *rvlist) : remote(){
  rlist = rvlist;
  rfkt = NULL;
}

remote_crtl::~remote_crtl(){
}

int remote_crtl::ExecAction(gchar **arglist){ 
  if(rfkt){ 
	  XSM_DEBUG(DBG_L2, "EA: excuting..." );
    if(rfkt->CmdFkt)
      (rfkt->CmdFkt)(arglist, rfkt->data);
    else
      XSM_DEBUG(DBG_L2, "CmdFkt is Zero !" );
    XSM_DEBUG(DBG_L2, "EA: done" );
    rfkt=NULL; 
  } 
  return 0;
}

gboolean remote_crtl::FindFktExec(char *verb, char *cmd, gchar **arglist){
  remote_verb *rt=rlist;
  remote_cmd  *cl=NULL;
  while(rt->verb && rt->typ != REMO_ENDLIST){
    if(strncmp(rt->verb, verb, MAX(strlen(verb) , strlen(rt->verb))))
      rt++;
    else{
      cl = rt->cmd_list;
      switch(rt->typ){
      case REMO_ACTION:
	while(cl->cmd){
	  if(strncmp(cl->cmd, cmd, MAX(strlen(cmd) , strlen(cl->cmd))))
	    cl++;
	  else{
	    rfkt = cl;                // spool Action
	    ExecAction(arglist);
	    XSM_DEBUG (DBG_L3, "Exec OK.");
	    return TRUE;
	  }
	}
	break;

      case REMO_MENUACTION:
	rfkt = cl;                // spool Action
	ExecAction(arglist);
	return TRUE;

      case REMO_SETENTRY:
	checkentry(arglist);
 	XSM_DEBUG (DBG_L3, "Set Entry OK.");
	return TRUE;

      case REMO_CBACTION:
	checkactions(arglist);
 	XSM_DEBUG (DBG_L3, "Action done.");
	return TRUE;

      default: break;
      }
    }
  }
  return FALSE;
}

void remote_crtl::senderror(char *errstr){
  if(!FindFktExec("ERROR", errstr, NULL))
    XSM_DEBUG_ERROR (DBG_L1, "No Errorfkt: " << errstr );
}

void remote_crtl::sendcmd(char *cmd, char *arg, gchar **arglist){
  if(!FindFktExec(cmd, arg, arglist))
    XSM_DEBUG_ERROR (DBG_L1, "No Cmdfkt: " << cmd << " " << arg );
}

void remote_crtl::quit(){
  if(!FindFktExec("quit", NULL, NULL))
    XSM_DEBUG_ERROR (DBG_L1, "No Quitfkt: -- " );
}





