/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Quick nine points Gxsm Plugin GUIDE by PZ:
 * ------------------------------------------------------------
 * 1.) Make a copy of this "intarea.C" to "your_plugins_name.C"!
 * 2.) Replace all "intarea" by "your_plugins_name" 
 *     --> please do a search and replace starting here NOW!! (Emacs doese preserve caps!)
 * 3.) Decide: One or Two Source Math: see line 54
 * 4.) Fill in GxsmPlugin Structure, see below
 * 5.) Replace the "about_text" below a desired
 * 6.) * Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!), 
 *       Goto Line 155 ff. please, and see comment there!!
 * 7.) Fill in math code in intarea_run(), 
 *     have a look at the Data-Access methods infos at end
 * 8.) Add intarea.C to the Makefile.am in analogy to others
 * 9.) Make a "make; make install"
 * A.) Call Gxsm->Tools->reload Plugins, be happy!
 * ... That's it!
 * 
 * Gxsm Plugin Name: baseinfo.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
 % BeginPlugInDocuSection
 % PlugInDocuCaption: Statistic base info
 % PlugInName: baseinfo
 % PlugInAuthor: Stefan Schr\"oder
 % PlugInAuthorEmail: stefan_fkp@users.sf.net
 % PlugInMenuPath: Math/Statistics/Baseinfo

 % PlugInDescription
 This plugin calculates some basic statistical information about the
 active scan or a selected rectangle within the active scan:

 \begin{description}
 \item[Abs. $Z$ min:] Minimum $Z$-value and its coordinate
 \item[Abs. $Z$ max:] Maximum $Z$-value and its coordinate
 \item[$\boldmath \sum Z$:] Sum of all $Z$-values
 \item[$\boldmath \sum Z^2$:] Sum of all squared $Z$-values
 \item[Averaged $Z$:] Average of all $Z$-values:
 \[ \overline Z = \frac{1}{A} \sum_{x,y} Z(x,y) \mbox{ mit } A:= x\cdot y \]
 \item[RMS-roughness:] root mean square roughness:
 \begin{eqnarray*}
 \sigma^2 &=& \frac{1}{A} \int_A d\vec x \, [(Z(x,y) - \overline Z)^2] \\
 &=& \frac{1}{A} [\sum_{x,y} Z(x,y)^2 
 - 2 \overline Z \sum_{x,y} Z(x,y) + \sum_{x,y} \overline Z^2]\\
 &=& \frac{1}{A} [\sum Z(x,y)^2 - 2 \overline Z \sum Z + \overline Z ^2 \sum 1]\\
 &=& \frac{1}{A} [\sum Z(x,y)^2 - 2 \overline Z \sum Z + \overline Z ^2 A]
 \end{eqnarray*}
 \end{description}

 % to translate/update:
 %  Bei der Begutachtung von $\sigma$ ist auf ein gutmütiges Verhalten des
 %  Mittelwertes zu achten.
 %  Auf der Standardausgabe werden die ermittelten Werte ausgegeben, um
 %  ein Markieren mit der Maus zu ermöglichen.
 %  Der Pixelmode ist z.Z. nicht implementiert.


 % PlugInUsage
 Choose Baseinfo from the \GxsmMenu{Math/Statistics/Baseinfo} menu.

 % OptPlugInSources
 You need one active scan.

 % OptPlugInObjects
 If a rectangle is selected the calculated information applies to the
 content of the rectangle. Otherwise, the whole scan is analyzed.

 % OptPlugInDest
 A dialog box appears, which contains the information in tabular form.
 You can press 'Dump to Stdout' to print the information on the console.
 Pressing 'Dump to comment' concatenates the calculated information to
 the comment field of this scan in the main window.

 % OptPlugInConfig
 None.

 % OptPlugInKnownBugs
 During the calculation of rms-roughness or the sum overflows can occur
 even in medium sized scans. You better choose a smaller region then.

 % OptPlugInNotes
 Calculated parameters are (with example values):
 \begin{tabbing}
 rms-roughness:  \= x \kill
 Area: \> (-35.0571 $\mu$m,-600.586 $\mu$m)-(-18.0728 $\mu$m,-656.711 $\mu$m)\\
 Area size: \>      18450 Int$^2$\\
 Minimum: \>        98 Int \\
 Min Pos: \>        (-27.942 $\mu$m,-600.815 $\mu$m)\\
 Maximum: \>        10438 Int\\
 Max Pos: \>        (-31.6143 $\mu$m,-628.763 $\mu$m)\\
 Sum:     \>        4.95591e+07 Int\\
 Sum Sq:  \>        1.86421e+09 Int\\
 Average: \>        2686.13 Int\\
 rms-roughness:  \>     nan Int
 \end{tabbing}

 % EndPlugInDocuSection
 *
 --------------------------------------------------------------------------------
*/

#include <sstream>

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"


using namespace std;


// Plugin Prototypes
static void baseinfo_init( void );
static void baseinfo_about( void );
static void baseinfo_configure( void );
static void baseinfo_cleanup( void );

static gboolean baseinfo_run( Scan *Src );

// Fill in the GxsmPlugin Description here
GxsmPlugin baseinfo_pi = {
        NULL,                   // filled in and used by Gxsm, don't touch !
        NULL,                   // filled in and used by Gxsm, don't touch !
        0,                      // filled in and used by Gxsm, don't touch !
        NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
        // filled in here by Gxsm on Plugin load, 
        // just after init() is called !!!
        // ----------------------------------------------------------------------
        // Plugins Name, CodeStly is like: Name-M1S[ND]|M2S-BG|F1D|F2D|ST|TR|Misc
        g_strdup ("Baseinfo-M1SND-ST"),
        NULL,
        // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
        g_strdup("Basic statistic information"),
        // Author(s)
        "Stefan Schroeder",
        // Menupath to position where it is appendet to
        "math-statistics-section",
        // Menuentry
        N_("Baseinfo"),
        // help text shown on menu
        N_("Basic statistic info."),
        // more info...
        "no more info",
        NULL,          // error msg, plugin may put error status msg here later
        NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
        // init-function pointer, can be "NULL", 
        // called if present at plugin load
        baseinfo_init,  
        // query-function pointer, can be "NULL", 
        // called if present after plugin init to let plugin manage it install itself
        NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
        // about-function, can be "NULL"
        // can be called by "Plugin Details"
        baseinfo_about,
        // configure-function, can be "NULL"
        // can be called by "Plugin Details"
        baseinfo_configure,
        // run-function, can be "NULL", if non-Zero and no query defined, 
        // it is called on menupath->"plugin"
        NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
        // cleanup-function, can be "NULL"
        // called if present at plugin removal
        NULL, // direct menu entry callback1 or NULL
        NULL, // direct menu entry callback2 or NULL

        baseinfo_cleanup
};

// special math Plugin-Strucure, use
GxsmMathOneSrcNoDestPlugin baseinfo_m1s_pi = {
        baseinfo_run
};

// Text used in Aboutbox, please update!!a
static const char *about_text = N_("Gxsm Plugin\n\n"
                                   "Basic statistic information.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        baseinfo_pi.description = g_strdup_printf(N_("Gxsm MathOneArg baseinfo plugin %s"), VERSION);
        return &baseinfo_pi; 
}

// Symbol "get_gxsm_math_one[_no_dest]|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!

GxsmMathOneSrcNoDestPlugin *get_gxsm_math_one_src_no_dest_plugin_info( void ) {
        return &baseinfo_m1s_pi; 
}

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//

/* Holds the String for dumping on STDOUT */
ostringstream info;


/* callback fuer delete_event*/
gint delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
        return(MATH_OK); 
}


// init-Function
static void baseinfo_init(void)
{
        PI_DEBUG (DBG_L2, "baseinfo Plugin Init");
}

// about-Function
static void baseinfo_about(void)
{
        const gchar *authors[] = { baseinfo_pi.authors, NULL};
        gtk_show_about_dialog (NULL,
                               "program-name",  baseinfo_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void baseinfo_configure(void)
{
        if(baseinfo_pi.app)
                baseinfo_pi.app->message("Baseinfo Plugin Configuration");
}

// cleanup-Function
static void baseinfo_cleanup(void)
{
        PI_DEBUG (DBG_L2, "Baseinfo Plugin Cleanup");
}

int ok_button_callback( GtkWidget *widget, gpointer data)
{
        printf("OK pressed. \n");
        return 0;
}

void dump_button_callback( GtkWidget *widget, gpointer data)
{
        //        cout << info << endl;
        cout << (const gchar*)info.str().c_str() << endl;
}

void comment_button_callback( GtkWidget *widget, gpointer data)
{
        // Add info to comment-field.
        main_get_gapp()->xsm->data.ui.comment = g_strconcat( main_get_gapp()->xsm->data.ui.comment, (const gchar*)info.str().c_str(), NULL);
        // Update textfield on screen.
        main_get_gapp()->spm_update_all();
}


// run-Function
static gboolean baseinfo_run(Scan *Src)
{
        int line, col, minline, mincol, maxline, maxcol;
        double akt, minimum, maximum, summe;
        double sumsquare;
        int left, right, top, bottom;
        double rms;
        double unitleft, unitright, unittop, unitbottom;
        double unitmincol, unitminline, unitmaxcol, unitmaxline; 
        double area, average, unitarea;
        MOUSERECT msr;
        MkMausSelect (Src, &msr, Src->mem2d->GetNx(), Src->mem2d->GetNy());

        if( msr.xSize  < 1 || msr.ySize < 1){
                left   = 0; 
                right  = Src->mem2d->GetNx();
                top    = 0;
                bottom = Src->mem2d->GetNy();
                area   = right * bottom;
        }
        else{
                left   = msr.xLeft;
                right  = msr.xRight;
                top    = msr.yTop;
                bottom = msr.yBottom;
                area   = msr.Area;
        }

        summe = 0.;
        sumsquare = 0.;
        minimum = maximum = Src->mem2d->GetDataPkt (0, 0);
 
        minline = maxline = top;
        mincol = maxcol = left;

        for(col = left; col < right; col++){
                for(line = top; line < bottom; line++){
                        akt = Src->mem2d->GetDataPkt(col, line);
                        summe += akt;
                        sumsquare += akt*akt;

                        if (akt > maximum) {
                                maximum = akt;
                                maxline = line; maxcol = col;
                        }
                        if (akt < minimum){
                                minimum = akt;
                                minline = line; mincol = col;
                        }
                }
        }

        average = summe / area;
        rms = ( sumsquare - 2*average*summe + average*average*area );
        rms = rms/area;
        rms = sqrt(rms);

        // convert pixelvalues to Unitvalues (but without Unitlabel)
        unitleft = Src->mem2d->data->GetXLookup(left);
        unitright= Src->mem2d->data->GetXLookup(right-1);
        unittop  = Src->mem2d->data->GetYLookup(top);
        unitbottom = Src->mem2d->data->GetYLookup(bottom-1);

        unitmincol =Src->mem2d->data->GetXLookup(mincol);
        unitminline=Src->mem2d->data->GetYLookup(minline);
        unitmaxcol =Src->mem2d->data->GetXLookup(maxcol);
        unitmaxline=Src->mem2d->data->GetYLookup(maxline);

        unitarea = (unitright-unitleft) * (unitbottom-unittop);

        GtkWidget *button1, *button2, *button3;
        GtkWidget *dialog = gtk_dialog_new();

        GtkWidget *grid = gtk_grid_new();

        gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                            grid, TRUE, TRUE, 10);

        const int n_col = 3;

        GtkTreeIter iter;
        const gchar *colum_hdr[3] = {  "Information","DA/DSP value", "value" };
        gchar* data[10][3];


        GtkTreeViewColumn *column[3];
        GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);


        int j=0;

        //Area was
        data [j][0] = g_strdup_printf (N_("Area was: "));
        data [j][1] = g_strdup_printf ("(%i,%i)-(%i,%i)",left,top,right-1,bottom-1);
        data [j][2] = g_strdup_printf ("(%s,%s)-(%s,%s)", 	Src->data.Xunit->UsrString(unitleft),
                                       Src->data.Yunit->UsrString(unittop),
                                       Src->data.Xunit->UsrString(unitright),
                                       Src->data.Yunit->UsrString(unitbottom));
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //Area
        data [j][0] = g_strdup_printf (N_("Area: "));
        data [j][1] = g_strdup_printf ("%f", area);
        data [j][2] = g_strdup_printf ("%s", Src->data.Zunit->UsrStringSqr(unitarea));
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //Minimum
        data [j][0] = g_strdup_printf (N_("Minimum: "));
        data [j][1] = g_strdup_printf ("%g", minimum);
        data [j][2] = g_strdup_printf ("%s", Src->data.Zunit->UsrString(Src->mem2d->GetDataPkt(mincol,minline)*(Src->data.s.dz)));
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;


        // Minimum-pos
        data [j][0] = g_strdup_printf (N_("Minimum-Pos: "));
        data [j][1] = g_strdup_printf ("(%i,%i)", mincol, minline);
        data [j][2] = g_strdup_printf ("(%s,%s)", Src->data.Xunit->UsrString(unitmincol), Src->data.Yunit->UsrString(unitminline));
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //Maximum
        data [j][0] = g_strdup_printf (N_("Maximum: "));
        data [j][1] = g_strdup_printf ("%g", maximum);
        data [j][2] = g_strdup_printf ("%s", Src->data.Zunit->UsrString(Src->mem2d->GetDataPkt(maxcol,maxline)*(Src->data.s.dz)));
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //Maximum-pos
        data [j][0] = g_strdup_printf (N_("Maximum-Pos: "));
        data [j][1] = g_strdup_printf ("(%i,%i)", maxcol, maxline);
        data [j][2] = g_strdup_printf ("(%s,%s)", Src->data.Xunit->UsrString(unitmaxcol), Src->data.Yunit->UsrString(unitmaxline));
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //sum
        data [j][0] = g_strdup_printf (N_("Sum: "));
        data [j][1] = g_strdup_printf ("%g", summe);
        data [j][2] = g_strdup_printf ("%s", Src->data.Zunit->UsrString(Src->data.s.dz*summe));
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //sumsq
        data [j][0] = g_strdup_printf (N_("Sum Squares: "));
        data [j][1] = g_strdup_printf ("%g", sumsquare);
        data [j][2] = g_strdup_printf ("%s^2", Src->data.Zunit->UsrString(Src->data.s.dz*Src->data.s.dz*sumsquare));
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //ave
        data [j][0] = g_strdup_printf (N_("Average: "));
        data [j][1] = g_strdup_printf ("%f", average);
        data [j][2] = g_strdup_printf ("%s", Src->data.Zunit->UsrString(Src->data.s.dz*average));
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        //rms
        data [j][0] = g_strdup_printf (N_("rms-roughness: "));
        data [j][1] = g_strdup_printf ("%f", rms);
        data [j][2] = g_strdup_printf ("%s", Src->data.Zunit->UsrString(Src->data.s.dz*rms));
        gtk_list_store_set (store, &iter,
                            G_TYPE_STRING, data [j][0],
                            G_TYPE_STRING, data [j][1],
                            G_TYPE_STRING, data [j][2],
                            -1);
        ++j;

        GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
        g_object_unref (G_OBJECT (store));

        gtk_grid_attach (GTK_GRID (grid), view, 0, 0, 3, 1);

        for (int i=0; i<n_col; ++i){
                GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
                column[i] = gtk_tree_view_column_new_with_attributes (colum_hdr[i], renderer,
                                                                      "text", i,
                                                                      NULL);
                /* Add the column to the view. */
                gtk_tree_view_append_column (GTK_TREE_VIEW (view),  column[i]);
        }

        // OK Button
        button1 = gtk_button_new_with_label ("OK");
        gtk_grid_attach (GTK_GRID (grid), button1, 0, 1, 1, 1);
        g_signal_connect (G_OBJECT (button1), "clicked",
                          G_CALLBACK (ok_button_callback), NULL);

        // Dump to stdout Button
        button2 = gtk_button_new_with_label ("Dump to Stdout");
        gtk_grid_attach (GTK_GRID (grid), button2, 1, 1, 1, 1);
        g_signal_connect (G_OBJECT (button2), "clicked",
                          G_CALLBACK (dump_button_callback), NULL);

        // Dump to comment Button
        button3 = gtk_button_new_with_label ("Dump to Comment");
        gtk_grid_attach (GTK_GRID (grid), button3, 2, 1, 1, 1);
        g_signal_connect (G_OBJECT (button3), "clicked",
                          G_CALLBACK (comment_button_callback), NULL);

        gtk_widget_show_all (dialog);
        gtk_dialog_run (GTK_DIALOG (dialog));

        info << N_("Area was: \t") 
             << "\t(" << left << "," << top << ")-(" << right-1 << "," << bottom-1 << ")"   
             << "\t(" << Src->data.Xunit->UsrString(unitleft) << "," << Src->data.Yunit->UsrString(unittop)<<")-("
             <<  Src->data.Xunit->UsrString(unitright) << "," << Src->data.Yunit->UsrString(unitbottom) << ")" << endl
             << N_("Area size: \t") << area << "px^2,  \t" << Src->data.Zunit->UsrStringSqr(unitarea)<< endl
             << N_("Minimum: \t") << minimum << "px  \t" <<Src->data.Zunit->UsrString(minimum) <<endl
             << N_("Min Pos: \t") << "(" << mincol << "," << minline << ")" << "\t(" 
             << Src->data.Xunit->UsrString(unitmincol) << "," << Src->data.Yunit->UsrString(unitminline)<<")" << endl
             << N_("Maximum: \t") << maximum << "px  \t" << Src->data.Zunit->UsrString(maximum) <<endl
             << N_("Max Pos: \t") << "(" << maxcol    << "," << maxline << ")"  << "\t(" 
             << Src->data.Xunit->UsrString(unitmaxcol) << "," << Src->data.Yunit->UsrString(unitmaxline)<<")" << endl
             << N_("Sum: \t\t")   << summe            << "px  \t"    << Src->data.Zunit->UsrString(Src->data.s.dz*summe) <<endl
             << N_("Sum Sq: \t")  << sumsquare        << "px^2 \t" << Src->data.Zunit->UsrString(Src->data.s.dz*Src->data.s.dz*sumsquare) << "^2" <<endl
             << N_("Average: \t") << average          << "px \t"   << Src->data.Zunit->UsrString(Src->data.s.dz*average) <<endl 
             << N_("rms-roughness: \t") << rms        << "\t"       << Src->data.Zunit->UsrString(Src->data.s.dz*rms)  <<endl
             << ends;


        for (int n=0; n<j; ++n)
                for (int k=0; k<3; ++k)
                        g_free (data[n][k]);

        return MATH_OK;
}
