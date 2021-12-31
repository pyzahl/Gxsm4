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

#ifndef __UNIT_H
#define __UNIT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <glib/gprintf.h>

#include <gtk/gtk.h>

#include "gxsm_monitor_vmemory_and_refcounts.h"

#define UTF8_ANGSTROEM "\303\205"
#define UTF8_MU        "\302\265"
#define UTF8_DEGREE    "\302\260"


class UnitObj;

// Object UnitObj:
typedef enum { UNIT_SM_NORMAL, UNIT_SM_PS } UNIT_MODES;

class UnitObj{
public:
        UnitObj(UnitObj &usrc){
                GXSM_REF_OBJECT (GXSM_GRC_UNITOBJ);
                sym   = g_strdup(usrc.sym); 
                pssym = g_strdup(usrc.pssym); 
                prec  = g_strdup(usrc.prec);
                label = g_strdup(usrc.label);
                if (usrc.alias)
                        alias = g_strdup(usrc.alias);
                else
                        alias = NULL;
        };
        UnitObj(const gchar *s, const gchar *pss){ 
                GXSM_REF_OBJECT (GXSM_GRC_UNITOBJ);
                sym   = g_strdup(s); 
                pssym = g_strdup(pss); 
                prec  = g_strdup("g");
                label = g_strdup(" ");
                alias = NULL;
        };
        UnitObj(const gchar *s, const gchar *pss, const gchar *precc){ 
                GXSM_REF_OBJECT (GXSM_GRC_UNITOBJ);
                sym   = g_strdup(s); 
                pssym = g_strdup(pss); 
                prec  = g_strdup(precc);
                label = g_strdup(" ");
                alias = NULL;
        };
        UnitObj(const gchar *s, const gchar *pss, const gchar *precc, const gchar *lab){ 
                GXSM_REF_OBJECT (GXSM_GRC_UNITOBJ);
                sym   = g_strdup(s); 
                pssym = g_strdup(pss); 
                prec  = g_strdup(precc);
                label = g_strdup(lab);
                alias = NULL;
        };
        virtual ~UnitObj(){ 
                g_free(sym); g_free(pssym); g_free(prec); g_free(label);
                if(alias) g_free(alias);
                GXSM_UNREF_OBJECT (GXSM_GRC_UNITOBJ);
        };

        virtual UnitObj* Copy(){ return new UnitObj(*this); };

        void SetAlias(const gchar *a){ 
                if(alias == NULL)
                        alias = g_strdup(a);
        };

        void ChangeSym(const gchar *s, const gchar *pss){ 
                g_free(sym); g_free(pssym); 
                sym=g_strdup(s); pssym=g_strdup(pss); 
        };

        void ChangePrec(const gchar *precc){ 
                g_free(prec);
                prec = g_strdup(precc);
        };

        void SetLabel(const gchar *lab){ 
                g_free(label);
                label = g_strdup(lab);
        };

        /* Unit Symbol */
        /* do not g_free this return string !!! */
        gchar *Symbol(UNIT_MODES um=UNIT_SM_NORMAL){
                switch(um){
                case UNIT_SM_NORMAL: return sym; 
                case UNIT_SM_PS: return pssym; 
                }
                return sym; 
        };     
        gchar *psSymbol(){ return pssym; }; /* Symbol for Postscript use */
        gchar *Label(){ return label; }; /* Name/Label for this Unit */
        gchar *Alias(){ return alias; }; /* Units Aliasname */
        virtual gchar *MakeLongLabel( UNIT_MODES um=UNIT_SM_NORMAL ){ /* free this later !! */
                return g_strconcat(label, " in ", Symbol(um), NULL);
        };

        virtual double Usr2Base(double u){ return u; }; /* Usr -> Base */
        virtual double Usr2Base(const gchar *ustr){ return atof (ustr); }; /* Usr -> Base */
        virtual double Base2Usr(double b){ return b; }; /* Usr <- Base */
        virtual void Change(double x, int n=0){};

        /* do g_free this returned string !!! */
        virtual gchar *UsrString(double b, UNIT_MODES um=UNIT_SM_NORMAL, const gchar *prec_override=NULL){
                gchar *fmt = g_strconcat("%",prec_override?prec_override:prec,um==UNIT_SM_NORMAL?" ":"", Symbol(um),NULL);
                gchar *txt;
                txt = g_strdup_printf(fmt, Base2Usr(b));
                g_free(fmt);
                return txt;
        };
        virtual gchar *UsrStringSqr(double b, UNIT_MODES um=UNIT_SM_NORMAL){
                //    gchar *fmt = g_strconcat("%",prec," ",Symbol(um),"^2",NULL);
                gchar *fmt = g_strconcat("%",prec," ", Symbol(um),"\302\262",NULL);
                gchar *txt;
                txt = g_strdup_printf(fmt, Base2Usr(Base2Usr(b)));
                g_free(fmt);
                return txt;
        };
        virtual void setval(gchar *name, double x){};
protected:
        gchar *sym, *pssym, *prec;
        gchar *label;
        gchar *alias;
private:
};

class UnitAutoMag;
class UnitAutoMag : public UnitObj{
public:
        UnitAutoMag(UnitAutoMag &usrc)
                :UnitObj(usrc){ fac=usrc.fac; mi=usrc.mi; };
        UnitAutoMag(const gchar *s, const gchar *pss)
                :UnitObj(s, pss){ fac=1.; mi=6; };
        UnitAutoMag(const gchar *s, const gchar *pss, const gchar *lab)
                :UnitObj(s, pss, "g", lab){ fac=1.; mi=6; };

        virtual UnitObj* Copy(){ return new UnitAutoMag(*this); };

        virtual gchar *MakeLongLabel( UNIT_MODES um=UNIT_SM_NORMAL ){ /* free this later !! */
                const gchar  *prefix[]    = { "a",  "f",   "p",   "n", UTF8_MU, "m", " ", "k", "M", "G", "T"  };
                return g_strconcat(label, " in ",  prefix[mi], Symbol(um), NULL);
        };

        double set_mag_get_base (double v, double bf=0.) {
                if (bf > 0.){
                        fac = bf;
                }
                //                            a:0    f:1    p:2    n:3    mu:4    m:5   1:6 k:7  M:8  G:9  T:10
                const double magnitude[]  = { 1e-18, 1e-15, 1e-12, 1e-9,  1e-6,   1e-3, 1., 1e3, 1e6, 1e9, 1e12 };
                double x = fac*v;
                if (fabs (x) < 1e-22){
                        mi=6;
                } else {
                        double m = fabs (x*1e-3);
                        for (mi=0; mi<=10; ++mi)
                                if (m < magnitude[mi])
                                        break;
                        if (mi>10)
                                mi=10;
                }
                g_print ("set_mag_get_base:: %g [x=%g]:[mi=%d]{%g}\n", v, x, mi, magnitude[mi]);
                return x/magnitude[mi];
        };

        virtual gchar *UsrString(double b, UNIT_MODES um=UNIT_SM_NORMAL){
                const gchar  *prefix[]    = { "a",  "f",   "p",   "n", UTF8_MU, "m", " ", "k", "M", "G", "T"  };
                double x = set_mag_get_base (b);
                gchar *fmt = g_strconcat("%",prec," ", prefix[mi], Symbol(um), NULL);
                gchar *txt;
                txt = g_strdup_printf(fmt, x);
                g_free(fmt);
                return txt;
        };
        virtual double Usr2Base(const gchar *ustr){
                double u=atof (ustr);
                const gchar  *prefix[]    = { "a",  "f",   "p",   "n", UTF8_MU, "m", " ", "k", "M", "G", "T", "u",   };
                const double magnitude[]  = {  1e-18, 1e-15, 1e-12, 1e-9,  1e-6,  1e-3, 1., 1e3, 1e6, 1e9, 1e12, 1e-6 };
                int i;
                for (i=0; i<=11; ++i){
                        gchar *mgu = g_strconcat(prefix[i], Symbol(), NULL);
                        g_print ("Usr2Base:: testing '%s' for '%s'\n", ustr, mgu);
                        if (strstr (ustr, mgu)){
                                g_free(mgu);
                                break;
                        }
                        g_free(mgu);
                }
                if (i<11) mi=i;
                else if (i==11) mi=4; // u -> mu

                g_print ("Usr2Base:: '%s' [u=%g]):[mi=%d]{%g} fac=%g\n", ustr, u, mi, magnitude[mi], fac);

                
                // else assume same as before in case user deleted unit or messed with it...
                return u*magnitude[mi]/fac;
        }; /* Usr -> Base */
        /* should not be used for in this context -- fall back only */
        virtual double Usr2Base (double u){
                return u;
        }; /* Usr -> Base */
        virtual double Base2Usr (double b){
                return set_mag_get_base (b);
        }; /* Usr <- Base */

private:
        int mi;
        double fac;
};

class LinUnit;

class LinUnit : public UnitObj{
public:
        LinUnit(LinUnit &usrc)
                :UnitObj(usrc){ fac=usrc.fac; off=usrc.off; };
        LinUnit(const gchar *s, const gchar *pss, double f=1., double o=0.)
                :UnitObj(s, pss){ fac=f; off=o; };
        LinUnit(const gchar *s, const gchar *pss, const gchar *lab, double f=1., double o=0.)
                :UnitObj(s, pss, "g", lab){ fac=f; off=o; };

        virtual UnitObj* Copy(){ return new LinUnit(*this); };
        virtual void setval(const gchar *name, double x){
                switch(*name){
                case 'f': fac=x; break;
                case 'o': off=x; break;
                }
        };

        virtual double Usr2Base(const gchar *ustr){ double u=atof (ustr); return (u*fac+off); }; /* Usr -> Base */
        virtual double Usr2Base(double u){ return (u*fac+off); };   /* Usr -> Base */
        virtual double Base2Usr(double b){ return ((b-off)/fac); }; /* Usr <- Base */

private:
        double fac, off;
};

// % BZ (Brilluin Zone) auf der Basis von Volt (Oktopol Ablenspannung)
class BZUnit;

class BZUnit : public UnitObj{
public:
        BZUnit(BZUnit &usrc):UnitObj(usrc){ En=usrc.En; Sens=usrc.Sens; };
        BZUnit(const gchar *s, const gchar *pss, double sensi=50., double energy=100.)
                :UnitObj(s, pss, "g", "k"){ Sens=sensi; En=energy; };
        BZUnit(const gchar *s, const gchar *pss, const gchar *lab, double sensi=50., double energy=100.)
                :UnitObj(s, pss, "g", lab){ Sens=sensi; En=energy; };

        virtual UnitObj* Copy(){ return new BZUnit(*this); };

        virtual double Usr2Base(const gchar *ustr){ double bz=atof (ustr);  return (bz/Sens*sqrt(En)); }; /* Usr -> Base */
        virtual double Usr2Base(double bz){ return (bz/Sens*sqrt(En)); }; /* Usr (BZ) -> Base (V) */
        virtual double Base2Usr(double  u){ return ( u*Sens/sqrt(En)); }; /* Usr (BZ) <- Base (V) */

        double GetE() { return En; };
        void SetE(double E) { En=E; };
        double GetSens() { return Sens; };
        void CalcSens(double volt, double bz){
                Sens = bz/volt*sqrt(En);
        };
private:
        double En, Sens;
};


// Phase S auf der Basis von Energy [eV], d0 Lagenabstand
class SUnit;

class SUnit : public UnitObj{
public:
        SUnit(SUnit &usrc):UnitObj(usrc){ d0=usrc.d0; cth=usrc.cth; };
        SUnit(const gchar *s, const gchar *pss, double dd0=3.141, double theta=5.)
                :UnitObj(s, pss, "g", "Phase"){ 
                d0=dd0; cth=cos(theta*M_PI/180.);
        };
        SUnit(const gchar *s, const gchar *pss, const gchar *lab, double dd0=3.141, double theta=5.)
                :UnitObj(s, pss, "g", lab){ 
                d0=dd0; cth=cos(theta*M_PI/180.);
        };

        virtual UnitObj* Copy(){ return new SUnit(*this); };

        virtual double Usr2Base(const gchar *ustr){
                double S=atof (ustr);
                double sdc=S/d0/cth;
                return (37.6*sdc*sdc);
        }; /* Usr -> Base */
        virtual double Usr2Base(double  S){  /* Usr (S) -> Base (eV) */
                double sdc=S/d0/cth;
                return (37.6*sdc*sdc);
        };
        virtual double Base2Usr(double En){  /* Usr (S) <- Base (eV) */
                return (d0*cth*sqrt(En/37.6));
        };

private:
        double d0, cth;
};

// % CPSCNTUnit (Counts Per Second) needs Gatetime in ms
class CPSCNTUnit;

class CPSCNTUnit : public UnitObj{
public:
        CPSCNTUnit(CPSCNTUnit &usrc):UnitObj(usrc){ gt=usrc.gt; };
        CPSCNTUnit(const gchar *s, const gchar *pss, double Gt=1.)
                :UnitObj(s, pss, "g", "CPS"){ gt=Gt; };
        CPSCNTUnit(const gchar *s, const gchar *pss, const gchar *lab, double Gt=1.)
                :UnitObj(s, pss, "g", lab){ gt=Gt; };

        virtual UnitObj* Copy(){ return new CPSCNTUnit(*this); };

        virtual double Usr2Base(const gchar *ustr){
                double cps=atof (ustr);
                return (cps*gt);
        };
        virtual double Usr2Base(double cps){ return (cps*gt) ; }; /* Usr (CPS) -> Base (CNT) */
        virtual double Base2Usr(double cnt){ return (cnt/gt) ; }; /* Usr (CPS) <- Base (CNT) */
        virtual void Change(double x, int n=0){ SetGatetime(x); };

        double GetGatetime() { return gt; };
        void SetGatetime(double Gt) { gt=Gt; };
private:
        double gt;
};

class LogUnit : public UnitObj{
public:
        LogUnit(LogUnit &usrc)
                :UnitObj(usrc){ fac=usrc.fac; off=usrc.off; };
        LogUnit(const gchar *s, const gchar *pss, double f=1., double o=1.)
                :UnitObj(s, pss){ fac=f; off=o; };
        LogUnit(const gchar *s, const gchar *pss, const gchar *lab, double f=1., double o=1.)
                :UnitObj(s, pss, "g", lab){ fac=f; off=o; };

        virtual UnitObj* Copy(){ return new LogUnit(*this); };
        virtual void setval(const gchar *name, double x){
                switch(*name){
                case 'f': fac=x; break;
                case 'o': off=x; break;
                }
        };
        virtual double Usr2Base(const gchar *ustr){
                double u=atof (ustr);
                return (off * log10 (u/fac) );
        };   /* Usr -> Base */
        virtual double Usr2Base(double u){ return (off * log10 (u/fac) ); };   /* Usr -> Base */
        virtual double Base2Usr(double b){ return (fac * pow (10., b/off)); }; /* Usr <- Base */

private:
        double fac, off;
};

class InvUnit : public UnitObj{
public:
        InvUnit(InvUnit &usrc)
                :UnitObj(usrc){ fac=usrc.fac; off=usrc.off; };
        InvUnit(const gchar *s, const gchar *pss, double f=1., double o=0.)
                :UnitObj(s, pss){ fac=f; off=o; };
        InvUnit(const gchar *s, const gchar *pss, const gchar *lab, double f=1., double o=0.)
                :UnitObj(s, pss, "g", lab){ fac=f; off=o; };

        virtual UnitObj* Copy(){ return new InvUnit(*this); };
        virtual void setval(const gchar *name, double x){
                switch(*name){
                case 'f': fac=x; break;
                case 'o': off=x; break;
                }
        };
        virtual double Usr2Base(const gchar *ustr){
                double u=atof (ustr);
                return (fac/u+off); 
        };
        virtual double Usr2Base(double u){ return (fac/u+off); };   /* Usr -> Base */
        virtual double Base2Usr(double b){ return (fac/(b-off)); }; /* Usr <- Base */

private:
        double fac, off;
};



#endif




