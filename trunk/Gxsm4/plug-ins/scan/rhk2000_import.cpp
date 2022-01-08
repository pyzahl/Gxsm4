/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rhk2000_im_export.C
 * ========================================
 * 
 * Copyright (C) 2004 The Free Software Foundation
 *
 * Authors: Peter Wahl <wahl@fkf.mpg.de>
 *
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Import Tool for RHK STM-200

% PlugInName: RHK2000-Import

% PlugInAuthor: Peter Wahl

% PlugInAuthorEmail: wahl@fkf.mpg.de

% PlugInMenuPath: File/Import/RHK STM-200 import 

% PlugInDescription

This plug-in is responsible for importing files saved by RHK STM-2000 systems.
The STM-2000 system is the STM electronics manufactured by RHK based on SGI 
workstations.

% PlugInUsage
Registers itself for loading files with the filename suffix ".Stm".

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "util.h"
#include "glbvars.h"
#include "surface.h"

#include <sstream>

using namespace std;

#define IMGMAXCOLORS 64

// Plugin Prototypes
static void rhk200_im_export_init (void);
static void rhk200_im_export_query (void);
static void rhk200_im_export_about (void);
static void rhk200_im_export_configure (void);
static void rhk200_im_export_cleanup (void);

static void rhk200_im_export_filecheck_load_callback (gpointer data );

static void rhk200_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin rhk200_im_export_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "Rhk200_Import",
  NULL,                // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Import of RHK STM-200 data format."),
  "Peter Wahl",
  "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
  N_("RHK-200,RHK-200"), // menu entry (same for both)
  NULL,
  "no more info for abc",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  rhk200_im_export_init,  
  rhk200_im_export_query,  
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  rhk200_im_export_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  NULL, //rhk200_im_export_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  rhk200_im_export_import_callback, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  rhk200_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm RHK STM-200 Data File Import Plugin\n\n"
                                   "This plugin reads in STM-200 datafiles."
	);

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  rhk200_im_export_pi.description = g_strdup_printf(N_("Gxsm rhk200_im_export plugin %s"), VERSION);
  return &rhk200_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/abc import"
// ----------------------------------------------------------------------
// !!!! make sure the "rhk200_im_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void rhk200_im_export_query(void)
{
  if(rhk200_im_export_pi.status) g_free(rhk200_im_export_pi.status); 
  rhk200_im_export_pi.status = g_strconcat (
	  N_("Plugin query has attached "),
	  rhk200_im_export_pi.name, 
	  N_(": File Input Filter is ready to use"),
	  NULL);

  
// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open, 
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	rhk200_im_export_pi.app->ConnectPluginToLoadFileEvent (rhk200_im_export_filecheck_load_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void rhk200_im_export_init(void)
{
  PI_DEBUG (DBG_L2, "rhk200_import Plugin Init" );
}

// about-Function
static void rhk200_im_export_about(void)
{
  const gchar *authors[] = { rhk200_im_export_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  rhk200_im_export_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void rhk200_im_export_configure(void)
{
  if(rhk200_im_export_pi.app)
    rhk200_im_export_pi.app->message("rhk200_im_export Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void rhk200_im_export_cleanup(void)
{
  PI_DEBUG (DBG_L2, "rhk200_im_export Plugin Cleanup" );
#if 0
  gnome_app_remove_menus (GNOME_APP (rhk200_im_export_pi.app->getApp()),
			  N_("File/Import/RHK STM-200 Import"), 1);
#endif
}

// make a new derivate of the base class "Dataio"
class rhk200_ImExportFile : public Dataio{
 public:
  rhk200_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
  virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
  virtual FIO_STATUS Write();
 private:
  bool topo;
  double gain;
  int pix,lin;
  int bit,img,type,page,adt,dat;
  double adf,daf,max_ad,min_ad,max_da,min_da;
  double a_v[4];
  string a_name[4],a_unit[4];
  double d_v[2];
  string d_name[2],d_unit[2];
  string user,date,dx[2];
  double scan_range[3],offset[3],scan_sensitivity[3][3],compensation[3];
  int sch,sgn;
  double sweep_time,angle,temp;
  int feedback_channel,image_channel,points[2];
  int nad,nda,sweeps,rcl,das,ssm,scg;
  double bias_range[2];
  double dwell,hold,sadf,sdaf,ex[2];
  double c_v[4];
  string microscope_name;
  double offset_sensitivity[3][3];
  string software;
  double version;
  int jbl;
  char sum;
  string fbs;
  double signal_gain[4],ro[4],junction_bias,loop_setpoint,los,scv,osv;
  double time_constant,int_loop_gain,prop_gain,diff_gain,as[2],ppg;
  int gettag(istream &is,string tag,string &val);
  int gettag(istream &is,string tag,int &val);
  int gethextag(istream &is,string tag,int &val);
  int gettag(istream &is,string tag,char &val);
  int gettag(istream &is,string tag,double &val);
  int gettag(istream &is,string tag);
  void checktopo();
  int readhead(istream &is);
  void readtopo(istream &is);
  void readspec(istream &is);
  FIO_STATUS rhkRead(const char *fname);
};

int rhk200_ImExportFile::gettag(istream &is,string tag)
{
  string buf;
  is>>buf; 
  if(buf==tag)
    return 0;
  else {
    cerr<<buf<<" "<<tag<<endl;
    PI_DEBUG (DBG_L2,"Unexpected tag.");
    return -1;
  }
}

int rhk200_ImExportFile::gettag(istream &is,string tag,int &val)
{
  string buf;
  is>>buf;
  if(buf==tag) {
    is>>val;
    return 0;
  } else {
    cerr<<buf<<" "<<tag<<endl;
    PI_DEBUG (DBG_L2,"Unexpected tag.");
    return -1;
  }
}

int rhk200_ImExportFile::gethextag(istream &is,string tag,int &val)
{
  string buf;
  is>>buf;
  if(buf==tag) {
    is>>hex>>val>>dec;
    return 0;
  } else {
    cerr<<buf<<" "<<tag<<endl;
    PI_DEBUG (DBG_L2,"Unexpected tag.");
    return -1;
  }
}

int rhk200_ImExportFile::gettag(istream &is,string tag,char &val)
{
  string buf;
  is>>buf;
  if(buf==tag) {
    is>>val;
    return 0;
  } else {
    cerr<<buf<<" "<<tag<<endl;
    PI_DEBUG (DBG_L2,"Unexpected tag.");
    return -1;
  }
}

int rhk200_ImExportFile::gettag(istream &is,string tag,double &val)
{
  string buf;
  is>>buf;
  if(buf==tag) {
    is>>val;
    return 0;
  } else {
    cerr<<buf<<" "<<tag<<endl;
    PI_DEBUG (DBG_L2,"Unexpected tag.");
    return -1;
  }
}

int rhk200_ImExportFile::gettag(istream &is,string tag,string &val)
{
  string buf;
  is>>buf;
  if(buf==tag) {
    char line[101];
    is.ignore(1);
    is.getline(line,100);
    val=string(line);
    return 0;
  } else {
    cerr<<buf<<" "<<tag<<endl;
    PI_DEBUG (DBG_L2,"Unexpected tag.");
    return -1;
  }
}

// d2d import :=)
FIO_STATUS rhk200_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
 
	// check for file exists and is OK !
	// else open File Dlg
	ifstream f;
	f.open(fname, ios::in);
	if(!f.good()){
		PI_DEBUG (DBG_L2, "File Fehler" );
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	
	// abc ".abc" type?
	if ((ret=rhkRead (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

int rhk200_ImExportFile::readhead(istream &is)
{
        int error;
	string dumstr;
        error+=gettag(is,"SwV",dumstr);
	error+=gettag(is,"Pix",pix); error+=gettag(is,"Lin",lin); error+=gettag(is,"Bit",bit); 
	if(bit!=16) return -1;
	error+=gettag(is,"Img",img); error+=gettag(is,"PTy",type); error+=gettag(is,"Pag",page);
	error+=gettag(is,"ADT",adt); error+=gettag(is,"DAT",dat); error+=gettag(is,"ADF",adf); 
	error+=gettag(is,"DAF",daf); error+=gettag(is,"MAD",max_ad); error+=gettag(is,"mAD",min_ad);
	error+=gettag(is,"MDA",max_da); error+=gettag(is,"mDA",min_da);
	for(int i=0;i<4;i++) {
	  ostringstream tagname;
	  tagname<<"A"<<i<<"V";
	  error+=gettag(is,tagname.str(),a_v[i]);
	  tagname.str("");
	  tagname<<"A"<<i<<"N";
	  error+=gettag(is,tagname.str(),a_name[i]);
	  tagname.str("");
	  tagname<<"A"<<i<<"U";
	  error+=gettag(is,tagname.str(),a_unit[i]);
	}
	for(int i=0;i<2;i++) {
	  ostringstream tagname;
	  tagname<<"D"<<i<<"V";
	  error+=gettag(is,tagname.str(),d_v[i]);
	  tagname.str("");
	  tagname<<"D"<<i<<"N";
	  error+=gettag(is,tagname.str(),d_name[i]);
	  tagname.str("");
	  tagname<<"D"<<i<<"U";
	  error+=gettag(is,tagname.str(),d_unit[i]);
	}
	error+=gettag(is,"Usr",user); error+=gettag(is,"STm",date);
	error+=gettag(is,"Dx0",dx[0]); error+=gettag(is,"Dx1",dx[1]); 
	for(int i=0;i<3;i++) {
	  ostringstream tagname;
	  tagname<<"SR"<<i;
	  error+=gettag(is,tagname.str(),scan_range[i]);
	}
	for(int i=0;i<3;i++) {
	  ostringstream tagname;
	  tagname<<"OS"<<i;
	  error+=gettag(is,tagname.str(),offset[i]);
	}
	for(int i=0;i<3;i++) {
	  ostringstream tagname;
	  tagname<<"CM"<<i;
	  error+=gettag(is,tagname.str(),compensation[i]);
	}
	error+=gettag(is,"SCh",sch); error+=gethextag(is,"SGn",sgn);
	error+=gettag(is,"LTm",sweep_time); error+=gettag(is,"SAn",angle);
	error+=gettag(is,"Tmp",temp); error+=gettag(is,"FbC",feedback_channel);
	error+=gettag(is,"ImC",image_channel); 
	error+=gettag(is,"XPt",points[0]); error+=gettag(is,"YPt",points[1]);
	error+=gettag(is,"NAD",nad); error+=gettag(is,"NDA",nda);
	error+=gettag(is,"NSw",sweeps); error+=gettag(is,"Rcl",rcl); 
	error+=gettag(is,"DAs",das); error+=gettag(is,"SSm",ssm);
	error+=gettag(is,"ScG",scg); 
	error+=gettag(is,"LoB",bias_range[0]); error+=gettag(is,"HiB",bias_range[1]);
	error+=gettag(is,"Dwl",dwell); error+=gettag(is,"Hld",hold);
	error+=gettag(is,"ADf",sadf); error+=gettag(is,"DAf",sdaf);
	error+=gettag(is,"ExH",ex[0]); error+=gettag(is,"ExL",ex[1]);
	for(int i=0;i<4;i++) {
	  ostringstream tagname;
	  tagname<<"C"<<i<<"V";
	  error+=gettag(is,tagname.str(),c_v[i]);
	}
	error+=gettag(is,"MNm",microscope_name);
	for(int i=0;i<3;i++)
	  for(int j=0;j<3;j++) {
	    ostringstream tagname;
	    tagname<<"S"<<i<<j;
	    error+=gettag(is,tagname.str(),scan_sensitivity[i][j]);
	  }
	for(int i=0;i<3;i++)
	  for(int j=0;j<3;j++) {
	    ostringstream tagname;
	    tagname<<"O"<<i<<j;
	    error+=gettag(is,tagname.str(),offset_sensitivity[i][j]);
	  }
	error+=gettag(is,"ENm",software); error+=gettag(is,"EVr",version);
	error+=gettag(is,"JBL",jbl); error+=gettag(is,"Sum",sum);
	error+=gettag(is,"FBS",fbs);
	for(int i=0;i<4;i++) {
	  ostringstream tagname;
	  tagname<<"SG"<<i;
	  error+=gettag(is,tagname.str(),signal_gain[i]);
	}
	for(int i=0;i<4;i++) {
	  ostringstream tagname;
	  tagname<<"RO"<<i;
	  error+=gettag(is,tagname.str(),ro[i]);
	}
	error+=gettag(is,"JBs",junction_bias); error+=gettag(is,"SPt",loop_setpoint);
	error+=gettag(is,"LOs",los); error+=gettag(is,"SCV",scv);
	error+=gettag(is,"OSV",osv); error+=gettag(is,"TCn",time_constant);
	error+=gettag(is,"LpG",int_loop_gain); error+=gettag(is,"PrG",prop_gain);
	error+=gettag(is,"DrG",diff_gain); 
	error+=gettag(is,"AS0",as[0]); error+=gettag(is,"AS1",as[1]);
	error+=gettag(is,"PPG",ppg);
	if(!error) return error;
	gain=1.0/(signal_gain[image_channel]*a_v[image_channel]*(1<<((sgn>>(4*image_channel))&0x000f)));
	if(a_name[image_channel]=="Z Signal") gain*=scan_sensitivity[2][0]*scv/max_ad;
	loop_setpoint=loop_setpoint/a_v[feedback_channel]*ppg;
	//from here on, the RHK header contains 3D-related stuff which we are not interested in
	checktopo();
	return 0;
}

void rhk200_ImExportFile::checktopo()
{
  if((points[0]==0)&&(points[1]==0)) topo=true;
  else {
    if(a_name[image_channel]=="Lockin") topo=false;
    else {
      topo=true;
      cerr<<"Warning: Detected topography in file with spectroscopy grid."<<endl;
    }
  }
}

void rhk200_ImExportFile::readtopo(istream &is)
{
  int nx,ny;
  short dumint;
  char *dumptr=(char *)&dumint;
  nx=pix; ny=lin; 
  scan->data.s.ntimes  = 1;
  scan->data.s.nvalues = 1; 
  scan->mem2d->Resize (scan->data.s.nx = nx,scan->data.s.ny = ny,ZD_SHORT);
  
  is.seekg(2048);
  for(int j=0; j<scan->data.s.ny; j++)
    for(int i=0; i<scan->data.s.nx; i++) {
      is.read((char *)&dumint,sizeof(short));
      swap(dumptr[0],dumptr[1]);
      scan->mem2d->PutDataPkt (dumint,i ,j );
    }
}

void rhk200_ImExportFile::readspec(istream &is)
{
  cerr<<"To be implemented !"<<endl;
  int nx=points[0],ny=points[1],speclinesize=sweeps*lin;
  short dumint;
  float *buf=new float(scan->data.s.nvalues = pix);
  char *dumptr=(char *)&dumint;
  scan->data.s.ntimes  = 1;
  scan->mem2d->Resize (scan->data.s.nx = nx, scan->data.s.ny = ny, scan->data.s.nvalues, ZD_FLOAT);
  is.seekg(2048);
  for(int j=0; j<scan->data.s.ny; j++)
    for(int i=0; i<scan->data.s.nx; i++) {
      for(int n=0; n<scan->data.s.nvalues; n++) buf[n]=0.0;
      for(int m=0; m<speclinesize; m++ )
	if(!(m%2)) for(int n=0; n<scan->data.s.nvalues; n++) {
	  is.read((char *)&dumint,sizeof(short));
	  swap(dumptr[0],dumptr[1]); 
	  buf[n]+=(float)dumint;
	} 
	else for(int n=scan->data.s.nvalues; n>0; n--) {
	  is.read((char *)&dumint,sizeof(short));
	  swap(dumptr[0],dumptr[1]); 
	  buf[n-1]+=(float)dumint;
	}
      for(int n=0; n<scan->data.s.nvalues; n++)
	scan->mem2d->PutDataPkt (buf[n]/speclinesize,i ,j, n );
    }
  delete buf;
}

//to be done:
// * support for spectroscopy, dI/dV-maps
FIO_STATUS rhk200_ImExportFile::rhkRead(const char *fname){
	int error=0;

	// Am I resposible for that file, is it in RHK STM2000 format ?
	if (strncmp(fname+strlen(fname)-4,".Stm",4)&&strncmp(fname+strlen(fname)-4,".sts",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	
	// feel free to do a more sophisticated file type check...
	ifstream is(fname,ios::in);
	if(gettag(is,"Ss*")) {
	  PI_DEBUG (DBG_L2, "Magic file number not detected - probably not an STM2000 file.");
	  is.close();
	  return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	} else PI_DEBUG (DBG_L2, "It's an RHK File!" );
	if(readhead(is)) {
	  PI_DEBUG (DBG_L2, "Error - returning !");
	  is.close();
	  return FIO_INVALID_FILE;
	}
	
	cerr<<"Read head ... "<<endl;
	if(topo) readtopo(is);
	else readspec(is);
	
  	cerr<<"Read topo ... "<<endl;
	
	// put some usefull values in the ui structure
	scan->data.ui.SetUser (user.c_str());
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("RHK STM-2000"); 
	scan->data.ui.SetComment((dx[0]+"\n"+dx[1]).c_str());

	// initialize scan structure -- this is a minimum example
	scan->data.s.rx = scan_sensitivity[0][0]*scan_range[0];
	scan->data.s.ry = scan_sensitivity[1][0]*scan_range[1];
	scan->data.s.dx = scan->data.s.rx/scan->data.s.nx;
	scan->data.s.dy = scan->data.s.ry/scan->data.s.ny;
	scan->data.s.x0 = offset_sensitivity[0][0]*offset[0];
	scan->data.s.y0 = offset_sensitivity[1][0]*offset[1];
	scan->data.s.alpha = angle;
	double dz=gain/32768.0;
	UnitObj *zu = main_get_gapp()->xsm->MakeUnit ("nm","nm");
        scan->data.s.dz = zu->Usr2Base(dz);
        scan->data.SetZUnit(zu);
        delete zu;
	scan->data.s.Bias=junction_bias;
	scan->data.s.Current = loop_setpoint;
	scan->data.ui.SetDateOfScan(date.c_str());
	scan->mem2d->data->MkYLookup(scan->data.s.y0, scan->data.s.y0+scan->data.s.ry);
	scan->mem2d->data->MkXLookup(scan->data.s.x0+scan->data.s.rx/2., scan->data.s.x0-scan->data.s.rx/2.);

	// be nice and reset this to some defined state
	scan->data.display.z_high       = 1e3;
	scan->data.display.z_low        = 1.;

	// set the default view parameters
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 1.0;

	// FYI: (PZ)
	//  scan->data.display.vrange_z  = ; // View Range Z in base ZUnits
	//  scan->data.display.voffset_z = 0; // View Offset Z in base ZUnits
	//  scan->AutoDisplay([...]); // may be used too...
  

	// abc read done.
	is.close();
	cerr<<"Done initialization"<<endl;
	return FIO_OK; 
}


FIO_STATUS rhk200_ImExportFile::Write(){
  return FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...

static void rhk200_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "checking for rhk2000 file type>" << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){ 
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}

		rhk200_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
//			main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
			main_get_gapp()->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "Skipping" << *fn << "<" );
	}
}


// Menu Call Back Fkte

static void rhk200_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
//				  "known extensions: pgm h16 nsc d2d dat sht byt flt dbl",
	gchar *fn = main_get_gapp()->file_dialog("RHK STM-200 Import", NULL,
				  "*.Stm", 
				  NULL, "RHK STM-200 Import");

    PI_DEBUG (DBG_L2, "FLDLG-IM::" << fn );
    rhk200_im_export_filecheck_load_callback (&fn );
}
