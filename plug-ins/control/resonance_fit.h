/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: inet_json_external_scandata.C
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

/*
  ## FROM PYTHON TUNE TOOL:
			def fit(function, parameters, x, data, u):
				def fitfun(params):
					for i,p in enumerate(parameters):
						p.set(params[i])
					return (data - function(x))/u

				if x is None: x = arange(data.shape[0])
				if u is None: u = ones(data.shape[0],"float")
				p = [param() for param in parameters]
				return leastsq(fitfun, p, full_output=1)

			# define function to be fitted
			def resonance(f):
				A=1000.
				return (A/A0())/sqrt(1.0+Q()**2*(f/w0()-w0()/f)**2)

			self.resmodel = "Model:  A(f) = (1000/A0) / sqrt (1 + Q^2 * (f/f0 - f0/f)^2)"

			# read data
			## freq, vr, dvr=load('lcr.dat', unpack=True)
			freq = self.Freq + self.Fstep*0.5         ## actual adjusted frequency, aligned for fit ??? not exactly sure why.
			vr   = self.ResAmp 
			dvr  = self.ResAmp/100.  ## error est. 1%

			# the fit parameters: some starting values
			A0=Parameter(iA0,"A")
			w0=Parameter(if0,"f0")
			Q=Parameter(iQ,"Q")

			p=[A0,w0,Q]

			# for theory plot we need some frequencies
			freqs=linspace(self.Fc - self.Fspan/2, self.Fc + self.Fspan/2, 200)
			initialplot=resonance(freqs)

#			self.Fit = initialplot
			
			# uncertainty calculation
#			v0=10.0
#			uvr=sqrt(dvr*dvr+vr*vr*0.0025)/v0
			v0=1.
			uvr=sqrt(dvr*dvr+vr*vr*0.0025)/v0

			# do fit using Levenberg-Marquardt
#			p2,cov,info,mesg,success=fit(resonance, p, freq, vr/v0, uvr)
			p2,cov,info,mesg,success=fit(resonance, p, freq, vr, None)

			if success==1:
				print "Converged"
			else:
				self.fitinfo[0] = "Fit not converged."
				print "Not converged"
				print mesg

			# calculate final chi square
			chisq=sum(info["fvec"]*info["fvec"])

			dof=len(freq)-len(p)
			# chisq, sqrt(chisq/dof) agrees with gnuplot
			print "Converged with chi squared ",chisq
			print "degrees of freedom, dof ", dof
			print "RMS of residuals (i.e. sqrt(chisq/dof)) ", sqrt(chisq/dof)
			print "Reduced chisq (i.e. variance of residuals) ", chisq/dof
			print

*/

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>

#define N      100    /* number of data points to fit */
#define TMAX   (40.0) /* time variable in [0,TMAX] */

struct data {
        size_t n;
        double * t;
        double * y;
};

struct mod_data {
        size_t n;
        double * t;
        double * y;
        double A,B,C;
};

/* GSL fitting:
 https://www.gnu.org/software/gsl/doc/html/nls.html#exponential-fitting-example
*/

class resonance_fit{
public:
        resonance_fit (double *frq, double *amp, double *model, size_t n) { 
                fit_data.n = n;
                fit_data.t = frq;
                fit_data.y = amp;
                result.n = n;
                result.t = frq;
                result.y = model;
                result.A  = 50.; // amplitude 1000/a
                result.B  = 30000.; // center
                result.C  = 5000.0; // Q
        };
        ~resonance_fit () { 
        };

        static double
        resonance(const double a, const double b, const double c, const double t)
        {
        // A(f) = (1000/A0) / sqrt (1 + Q^2 * (f/f0 - f0/f)^2)
        // rewite a := A0, b := f0, c := Q, t := f
        // Res(a,b,c,t)
                const double z = t/b - b/t;
                return (1000./a) / sqrt (1. + c*c * z*z);
        };
        
        static int
        func_f (const gsl_vector * x, void *params, gsl_vector * f)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double yi = d->y[i];
                                double y = resonance(a, b, c, ti);

                                gsl_vector_set(f, i, yi - y);
                        }

                return GSL_SUCCESS;
        };

        // Resonance partials
        static int
        func_df (const gsl_vector * x, void *params, gsl_matrix * J)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti    = d->t[i];
                                double zi    = ti/b - b/ti;
                                double czi   = c*c*zi*zi + 1.0;
                                double czi32 = czi*sqrt(czi);
                                double dbzi  = -ti/(b*b) - 1.0/ti;
                                
                                gsl_matrix_set(J, i, 0, 1000.0/(a*a*sqrt(czi)));
                                gsl_matrix_set(J, i, 1, 1000.0*c*c*dbzi*zi/(a*czi32));
                                gsl_matrix_set(J, i, 2, 1000.0*c*zi*zi/(a*czi32));
                        }

                return GSL_SUCCESS;
        };

        // Resonance Jacobian (2nd order)
        static int
        func_fvv (const gsl_vector * x, const gsl_vector * v,
                  void *params, gsl_vector * fvv)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                double va = gsl_vector_get(v, 0);
                double vb = gsl_vector_get(v, 1);
                double vc = gsl_vector_get(v, 2);
                size_t i;

                double a2 = a*a;
                double b2 = b*b;
                double c2 = c*c;
                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double zi   = ti/b - b/ti;
                                double czi  = c2*zi*zi + 1.0;
                                double czi12 = sqrt(czi);
                                double czi32 = czi*czi12;
                                double czi52 = czi*czi*czi12;
                                double dbzi = -ti/b2 - 1.0/ti;
                                
                                double Daa = -2000.0/(a2*a*czi12);
                                double Dab = -1000.0*c2*dbzi*zi/(a2*czi32);
                                double Dac = -1000.0*c*zi*zi/(a2*czi32);
                                double Dbb = 2000.0*c2*ti*zi/(a*b2*b*czi32) + 1000.0*c2*dbzi*dbzi/(a*czi32) - 3000.0*c2*c2*dbzi*dbzi*zi*zi/(a*czi52);
                                double Dbc = 2000.0*c*dbzi*zi/(a*czi32) - 3000.0*c2*c*dbzi*zi*zi*zi/(a*czi52);
                                double Dcc = 1000.0*zi*zi*(1.0-2.0*c2*zi*zi)/(a*czi52);
                                double sum;

                                sum =
                                        va * va * Daa
                                        + 2.0 * va * vb * Dab
                                        + 2.0 * va * vc * Dac
                                        + vb * vb * Dbb
                                        + 2.0 * vb * vc * Dbc
                                        + vc * vc * Dcc;

                                gsl_vector_set(fvv, i, sum);
                        }

                return GSL_SUCCESS;
        };
        
        static void
        callback(const size_t iter, void *params,
                 const gsl_multifit_nlinear_workspace *w)
        {
                gsl_vector *f = gsl_multifit_nlinear_residual(w);
                gsl_vector *x = gsl_multifit_nlinear_position(w);
                double avratio = gsl_multifit_nlinear_avratio(w);
                double rcond;

                (void) params; /* not used */

                /* compute reciprocal condition number of J(x) */
                gsl_multifit_nlinear_rcond(&rcond, w);

                if (debug_level > 0)
                        fprintf(stderr, "iter %2zu: a = %.4f, b = %.4f, c = %.4f, |a|/|v| = %.4f cond(J) = %8.4f, |f(x)| = %.4f\n",
                                iter,
                                gsl_vector_get(x, 0),
                                gsl_vector_get(x, 1),
                                gsl_vector_get(x, 2),
                                avratio,
                                1.0 / rcond,
                                gsl_blas_dnrm2(f));
        };

        void
        solve_system(gsl_vector *x, gsl_multifit_nlinear_fdf *fdf,
                     gsl_multifit_nlinear_parameters *params)
        {
                const gsl_multifit_nlinear_type *T = gsl_multifit_nlinear_trust;
                //const size_t max_iter = 200;
                //const double xtol = 1.0e-8;
                //const double gtol = 1.0e-8;
                //const double ftol = 1.0e-8;
                const size_t max_iter = 200;
                const double xtol = 1.0e-8;
                const double gtol = 1.0e-8;
                const double ftol = 1.0e-8;
                const size_t n = fdf->n;
                const size_t p = fdf->p;
                gsl_multifit_nlinear_workspace *work = gsl_multifit_nlinear_alloc(T, params, n, p);
                gsl_vector * f = gsl_multifit_nlinear_residual(work);
                gsl_vector * y = gsl_multifit_nlinear_position(work);
                int info;
                double chisq0, chisq, rcond;

                /* initialize solver */
                gsl_multifit_nlinear_init(x, fdf, work);

                /* store initial cost */
                gsl_blas_ddot(f, f, &chisq0);

                /* iterate until convergence */
                gsl_multifit_nlinear_driver(max_iter, xtol, gtol, ftol,
                                            callback, NULL, &info, work);

                /* store final cost */
                gsl_blas_ddot(f, f, &chisq);

                /* store cond(J(x)) */
                gsl_multifit_nlinear_rcond(&rcond, work);

                gsl_vector_memcpy(x, y);

                /* print summary */

                if (debug_level > 0){
                        fprintf(stderr, "NITER         = %zu\n", gsl_multifit_nlinear_niter(work));
                        fprintf(stderr, "NFEV          = %zu\n", fdf->nevalf);
                        fprintf(stderr, "NJEV          = %zu\n", fdf->nevaldf);
                        fprintf(stderr, "NAEV          = %zu\n", fdf->nevalfvv);
                        fprintf(stderr, "initial cost  = %.12e\n", chisq0);
                        fprintf(stderr, "final cost    = %.12e\n", chisq);
                        fprintf(stderr, "final x       = (%.12e, %.12e, %12e)\n",
                                gsl_vector_get(x, 0), gsl_vector_get(x, 1), gsl_vector_get(x, 2));
                        fprintf(stderr, "final cond(J) = %.12e\n", 1.0 / rcond);
                }
                gsl_multifit_nlinear_free(work);
        };


        int execute_fit ()
        {
                const size_t n = fit_data.n;
                const size_t p = 3;    /* number of model parameters */
                const gsl_rng_type * T = gsl_rng_default;
                gsl_vector *f = gsl_vector_alloc(n);
                gsl_vector *x = gsl_vector_alloc(p);
                gsl_multifit_nlinear_fdf fdf;
                gsl_multifit_nlinear_parameters fdf_params = gsl_multifit_nlinear_default_parameters();
                gsl_rng * r;
                size_t i;

                gsl_rng_env_setup ();
                r = gsl_rng_alloc (T);

                /* define function to be minimized */
                fdf.f = func_f;
                fdf.df = func_df;
                fdf.fvv = func_fvv;
                fdf.n = n;
                fdf.p = p;
                fdf.params = &fit_data;

                /* starting point */
                gsl_vector_set(x, 0, result.A);
                gsl_vector_set(x, 1, result.B);
                gsl_vector_set(x, 2, result.C);

                fdf_params.trs = gsl_multifit_nlinear_trs_lmaccel;
                solve_system(x, &fdf, &fdf_params);

                /* store results and model */
                result.A = gsl_vector_get(x, 0);
                result.B = gsl_vector_get(x, 1);
                result.C = gsl_vector_get(x, 2);

                for (i = 0; i < result.n; ++i)
                        result.y[i] = resonance(result.A, result.B, result.C, result.t[i]);

                gsl_vector_free(f);
                gsl_vector_free(x);
                gsl_rng_free(r);

                return 0;
        };

        // set initial
        void set_Q(double q) { result.C = q; };
        void set_A(double a) { result.A = a; };
        void set_F0(double f0) { result.B = f0; };

        // get results
        double get_Q() { return result.C; };
        double get_A() { return result.A; };
        double get_F0() { return result.B; };
        
private:
        struct data fit_data;
        struct mod_data result;
};


// Gaussian Model  a * exp( -1/2 * [ (t - b) / c ]^2 )

class gaussian_fit{
public:
        gaussian_fit (double *frq, double *amp, double *model, size_t n) { 
                fit_data.n = n;
                fit_data.t = frq;
                fit_data.y = amp;
                result.n = n;
                result.t = frq;
                result.y = model;
                result.A  = 50.;
                result.B  = 30000.;
                result.C  = 5000.0;
        };
        ~gaussian_fit () { 
        };


        /* model function: a * exp( -1/2 * [ (t - b) / c ]^2 ) */
        static double
        gaussian(const double a, const double b, const double c, const double t)
        {
                const double z = (t - b) / c;
                return (a * exp(-0.5 * z * z));
        };

        static int
        func_f (const gsl_vector * x, void *params, gsl_vector * f)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double yi = d->y[i];
                                double y = gaussian(a, b, c, ti);

                                gsl_vector_set(f, i, yi - y);
                        }

                return GSL_SUCCESS;
        };

// Gaussian partials (1st order)
        static int
        func_df (const gsl_vector * x, void *params, gsl_matrix * J)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double zi = (ti - b) / c;
                                double ei = exp(-0.5 * zi * zi);

                                gsl_matrix_set(J, i, 0, -ei);
                                gsl_matrix_set(J, i, 1, -(a / c) * ei * zi);
                                gsl_matrix_set(J, i, 2, -(a / c) * ei * zi * zi);
                        }

                return GSL_SUCCESS;
        };

// Gaussian Jacobian (2nd order)
        static int
        func_fvv (const gsl_vector * x, const gsl_vector * v,
                  void *params, gsl_vector * fvv)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                double va = gsl_vector_get(v, 0);
                double vb = gsl_vector_get(v, 1);
                double vc = gsl_vector_get(v, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double zi = (ti - b) / c;
                                double ei = exp(-0.5 * zi * zi);
                                double Dab = -zi * ei / c;
                                double Dac = -zi * zi * ei / c;
                                double Dbb = a * ei / (c * c) * (1.0 - zi*zi);
                                double Dbc = a * zi * ei / (c * c) * (2.0 - zi*zi);
                                double Dcc = a * zi * zi * ei / (c * c) * (3.0 - zi*zi);
                                double sum;

                                sum = 2.0 * va * vb * Dab +
                                        2.0 * va * vc * Dac +
                                        vb * vb * Dbb +
                                        2.0 * vb * vc * Dbc +
                                        vc * vc * Dcc;

                                gsl_vector_set(fvv, i, sum);
                        }

                return GSL_SUCCESS;
        };
        
        static void
        callback(const size_t iter, void *params,
                 const gsl_multifit_nlinear_workspace *w)
        {
                gsl_vector *f = gsl_multifit_nlinear_residual(w);
                gsl_vector *x = gsl_multifit_nlinear_position(w);
                double avratio = gsl_multifit_nlinear_avratio(w);
                double rcond;

                (void) params; /* not used */

                /* compute reciprocal condition number of J(x) */
                gsl_multifit_nlinear_rcond(&rcond, w);

                if (debug_level > 0)
                        fprintf(stderr, "iter %2zu: a = %.4f, b = %.4f, c = %.4f, |a|/|v| = %.4f cond(J) = %8.4f, |f(x)| = %.4f\n",
                                iter,
                                gsl_vector_get(x, 0),
                                gsl_vector_get(x, 1),
                                gsl_vector_get(x, 2),
                                avratio,
                                1.0 / rcond,
                                gsl_blas_dnrm2(f));
        };

        void
        solve_system(gsl_vector *x, gsl_multifit_nlinear_fdf *fdf,
                     gsl_multifit_nlinear_parameters *params)
        {
                const gsl_multifit_nlinear_type *T = gsl_multifit_nlinear_trust;
                const size_t max_iter = 200;
                const double xtol = 1.0e-8;
                const double gtol = 1.0e-8;
                const double ftol = 1.0e-8;
                const size_t n = fdf->n;
                const size_t p = fdf->p;
                gsl_multifit_nlinear_workspace *work = gsl_multifit_nlinear_alloc(T, params, n, p);
                gsl_vector * f = gsl_multifit_nlinear_residual(work);
                gsl_vector * y = gsl_multifit_nlinear_position(work);
                int info;
                double chisq0, chisq, rcond;

                /* initialize solver */
                gsl_multifit_nlinear_init(x, fdf, work);

                /* store initial cost */
                gsl_blas_ddot(f, f, &chisq0);

                /* iterate until convergence */
                gsl_multifit_nlinear_driver(max_iter, xtol, gtol, ftol,
                                            callback, NULL, &info, work);

                /* store final cost */
                gsl_blas_ddot(f, f, &chisq);

                /* store cond(J(x)) */
                gsl_multifit_nlinear_rcond(&rcond, work);

                gsl_vector_memcpy(x, y);

                /* print summary */

                if (debug_level > 0){
                        fprintf(stderr, "NITER         = %zu\n", gsl_multifit_nlinear_niter(work));
                        fprintf(stderr, "NFEV          = %zu\n", fdf->nevalf);
                        fprintf(stderr, "NJEV          = %zu\n", fdf->nevaldf);
                        fprintf(stderr, "NAEV          = %zu\n", fdf->nevalfvv);
                        fprintf(stderr, "initial cost  = %.12e\n", chisq0);
                        fprintf(stderr, "final cost    = %.12e\n", chisq);
                        fprintf(stderr, "final x       = (%.12e, %.12e, %12e)\n",
                                gsl_vector_get(x, 0), gsl_vector_get(x, 1), gsl_vector_get(x, 2));
                        fprintf(stderr, "final cond(J) = %.12e\n", 1.0 / rcond);
                }
                gsl_multifit_nlinear_free(work);
        };

        int execute_fit ()
        {
                const size_t n = fit_data.n;
                const size_t p = 3;    /* number of model parameters */
                const gsl_rng_type * T = gsl_rng_default;
                gsl_vector *f = gsl_vector_alloc(n);
                gsl_vector *x = gsl_vector_alloc(p);
                gsl_multifit_nlinear_fdf fdf;
                gsl_multifit_nlinear_parameters fdf_params = gsl_multifit_nlinear_default_parameters();
                gsl_rng * r;
                size_t i;

                gsl_rng_env_setup ();
                r = gsl_rng_alloc (T);

                /* define function to be minimized */
                fdf.f = func_f;
                fdf.df = func_df;
                fdf.fvv = func_fvv;
                fdf.n = n;
                fdf.p = p;
                fdf.params = &fit_data;

                /* starting point */
                gsl_vector_set(x, 0, result.A);
                gsl_vector_set(x, 1, result.B);
                gsl_vector_set(x, 2, result.C);

                fdf_params.trs = gsl_multifit_nlinear_trs_lmaccel;
                solve_system(x, &fdf, &fdf_params);

                /* store results and model */
                result.A = gsl_vector_get(x, 0);
                result.B = gsl_vector_get(x, 1);
                result.C = gsl_vector_get(x, 2);

                for (i = 0; i < result.n; ++i)
                        result.y[i] = gaussian(result.A, result.B, result.C, result.t[i]);

                gsl_vector_free(f);
                gsl_vector_free(x);
                gsl_rng_free(r);

                return 0;
        };

        // set initial
        void set_A(double a) { result.A = a; };
        void set_B(double b) { result.B = b; };
        void set_C(double c) { result.C = c; };

        // get results
        double get_A() { return result.A; };
        double get_B() { return result.B; };
        double get_C() { return result.C; };
        
private:
        struct data fit_data;
        struct mod_data result;
};





class phase_fit{
public:
        phase_fit (double *frq, double *ph, double *model, size_t n) { 
                fit_data.n = n;
                fit_data.t = frq;
                fit_data.y = ph;
                result.n = n;
                result.t = frq;
                result.y = model;
                result.A  = 1.;
                result.B  = 30000.;
                result.C  = 0.;
        };
        ~phase_fit () { 
        };


        /* model function: atan (a*(t-b)) + c */
        static double
        phase(const double a, const double b, const double c, const double t)
        {
                const double rad = 180.0/M_PI;
                const double z = t - b;
                return rad*atan (a * z) + c;
        };

        static int
        func_f (const gsl_vector * x, void *params, gsl_vector * f)
        {
                struct data *d = (struct data *) params;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double yi = d->y[i];
                                double y = phase(a, b, c, ti);

                                gsl_vector_set(f, i, yi - y);
                        }

                return GSL_SUCCESS;
        };

// Phase partials (1st order)
        static int
        func_df (const gsl_vector * x, void *params, gsl_matrix * J)
        {
                struct data *d = (struct data *) params;
                const double rad = 180.0/M_PI;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                double a2 = a*a;
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double zi = ti - b;
                                double zi2 = zi*zi;
                                
                                gsl_matrix_set(J, i, 0, -rad*zi/(a2*zi2+1.0));
                                gsl_matrix_set(J, i, 1, rad*a/(a2*zi2+1.0));
                                gsl_matrix_set(J, i, 2, -1.0);
                        }

                return GSL_SUCCESS;
        };

// Phase Jacobian (2nd order)
        static int
        func_fvv (const gsl_vector * x, const gsl_vector * v,
                  void *params, gsl_vector * fvv)
        {
                struct data *d = (struct data *) params;
                const double rad = 180.0/M_PI;
                double a = gsl_vector_get(x, 0);
                double b = gsl_vector_get(x, 1);
                double c = gsl_vector_get(x, 2);
                double a2 = a*a;
                double va = gsl_vector_get(v, 0);
                double vb = gsl_vector_get(v, 1);
                double vc = gsl_vector_get(v, 2);
                size_t i;

                for (i = 0; i < d->n; ++i)
                        {
                                double ti = d->t[i];
                                double zi = ti - b;
                                double zi2 = zi*zi;
                                double zi3 = zi2*zi;
                                double a2zi2  = a2*zi2+1.0;
                                double a2zi22 = a2zi2*a2zi2;
                                double Daa = 2.0*a*zi3/a2zi22;
                                double Dab = -(a2*zi2-1.0)/a2zi22;
                                double Dbb = 2.0*a2*a*zi/a2zi22;
                                // D*c = 0;
                                double sum;

                                sum =
                                        va * va * Daa
                                        + 2.0 * va * vb * Dab
                                        + vb * vb * Dbb;

                                gsl_vector_set(fvv, i, rad*sum);
                        }

                return GSL_SUCCESS;
        };
        
        static void
        callback(const size_t iter, void *params,
                 const gsl_multifit_nlinear_workspace *w)
        {
                gsl_vector *f = gsl_multifit_nlinear_residual(w);
                gsl_vector *x = gsl_multifit_nlinear_position(w);
                double avratio = gsl_multifit_nlinear_avratio(w);
                double rcond;

                (void) params; /* not used */

                /* compute reciprocal condition number of J(x) */
                gsl_multifit_nlinear_rcond(&rcond, w);

                if (debug_level > 0)
                        fprintf(stderr, "iter %2zu: a = %.4f, b = %.4f, c = %.4f, |a|/|v| = %.4f cond(J) = %8.4f, |f(x)| = %.4f\n",
                                iter,
                                gsl_vector_get(x, 0),
                                gsl_vector_get(x, 1),
                                gsl_vector_get(x, 2),
                                avratio,
                                1.0 / rcond,
                                gsl_blas_dnrm2(f));
        };

        void
        solve_system(gsl_vector *x, gsl_multifit_nlinear_fdf *fdf,
                     gsl_multifit_nlinear_parameters *params)
        {
                const gsl_multifit_nlinear_type *T = gsl_multifit_nlinear_trust;
                const size_t max_iter = 200;
                const double xtol = 1.0e-8;
                const double gtol = 1.0e-8;
                const double ftol = 1.0e-8;
                const size_t n = fdf->n;
                const size_t p = fdf->p;
                gsl_multifit_nlinear_workspace *work = gsl_multifit_nlinear_alloc(T, params, n, p);
                gsl_vector * f = gsl_multifit_nlinear_residual(work);
                gsl_vector * y = gsl_multifit_nlinear_position(work);
                int info;
                double chisq0, chisq, rcond;

                /* initialize solver */
                gsl_multifit_nlinear_init(x, fdf, work);

                /* store initial cost */
                gsl_blas_ddot(f, f, &chisq0);

                /* iterate until convergence */
                gsl_multifit_nlinear_driver(max_iter, xtol, gtol, ftol,
                                            callback, NULL, &info, work);

                /* store final cost */
                gsl_blas_ddot(f, f, &chisq);

                /* store cond(J(x)) */
                gsl_multifit_nlinear_rcond(&rcond, work);

                gsl_vector_memcpy(x, y);

                /* print summary */

                if (debug_level > 0){
                        fprintf(stderr, "NITER         = %zu\n", gsl_multifit_nlinear_niter(work));
                        fprintf(stderr, "NFEV          = %zu\n", fdf->nevalf);
                        fprintf(stderr, "NJEV          = %zu\n", fdf->nevaldf);
                        fprintf(stderr, "NAEV          = %zu\n", fdf->nevalfvv);
                        fprintf(stderr, "initial cost  = %.12e\n", chisq0);
                        fprintf(stderr, "final cost    = %.12e\n", chisq);
                        fprintf(stderr, "final x       = (%.12e, %.12e, %12e)\n",
                                gsl_vector_get(x, 0), gsl_vector_get(x, 1), gsl_vector_get(x, 2));
                        fprintf(stderr, "final cond(J) = %.12e\n", 1.0 / rcond);
                }
                gsl_multifit_nlinear_free(work);
        };

        int execute_fit ()
        {
                const size_t n = fit_data.n;
                const size_t p = 3;    /* number of model parameters */
                const gsl_rng_type * T = gsl_rng_default;
                gsl_vector *f = gsl_vector_alloc(n);
                gsl_vector *x = gsl_vector_alloc(p);
                gsl_multifit_nlinear_fdf fdf;
                gsl_multifit_nlinear_parameters fdf_params = gsl_multifit_nlinear_default_parameters();
                gsl_rng * r;
                size_t i;

                gsl_rng_env_setup ();
                r = gsl_rng_alloc (T);

                /* define function to be minimized */
                fdf.f = func_f;
                fdf.df = func_df;
                fdf.fvv = func_fvv;
                fdf.n = n;
                fdf.p = p;
                fdf.params = &fit_data;

                /* starting point */
                gsl_vector_set(x, 0, result.A);
                gsl_vector_set(x, 1, result.B);
                gsl_vector_set(x, 2, result.C);

                fdf_params.trs = gsl_multifit_nlinear_trs_lmaccel;
                solve_system(x, &fdf, &fdf_params);

                /* store results and model */
                result.A = gsl_vector_get(x, 0);
                result.B = gsl_vector_get(x, 1);
                result.C = gsl_vector_get(x, 2);

                for (i = 0; i < result.n; ++i)
                        result.y[i] = phase(result.A, result.B, result.C, result.t[i]);

                gsl_vector_free(f);
                gsl_vector_free(x);
                gsl_rng_free(r);

                return 0;
        };

        // set initial
        void set_A(double a) { result.A = a; };
        void set_B(double b) { result.B = b; };
        void set_C(double c) { result.C = c; };

        // get results
        double get_A() { return result.A; };
        double get_B() { return result.B; };
        double get_C() { return result.C; };
        
private:
        struct data fit_data;
        struct mod_data result;
};

