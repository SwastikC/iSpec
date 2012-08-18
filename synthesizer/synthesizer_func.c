/**
    This file is part of Spectra.
    Copyright 2011-2012 Sergi Blanco Cuaresma - http://www.marblestation.com
    
    Spectra is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spectra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with Spectra.  If not, see <http://www.gnu.org/licenses/>.
**/
/*
    This file has been adapted from spectrum.c:
     
        SPECTRUM, a Stellar Spectral Synthesis Program
        (C) Richard O. Gray 1992 - 2010 Version 2.76e
        May 3, 2010
     
    It works always on the Integrated Disk mode (normalized Intensity) 
*/
#include <stdio.h>
#include "spectrum276e/spectrum.h"
#include "synthesizer_func.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


void inmodel(atmosphere *model, char *file, int flagw);
void Density(atmosphere *model, atominfo *atom, double ah, double ahe, int flagw);
void hotDensity(atmosphere *model,atominfo *atom,double ah,double ahe,int flagw);
void veryhotDensity(atmosphere *model,atominfo *atom,double ah,double ahe,int flagw);
void tauref(atmosphere *model, double wavref);
void tauwave(atmosphere *model, double wave);
void tauflx(atmosphere *model, double wstart, double wend);
double flux(atmosphere *model, double wave);
void fluxflx(atmosphere *model, double wstart, double wend, double *flxstart, double *flxend);
double intensity(atmosphere *model, double wave, double mu);
void intenint(atmosphere *model, double wstart, double wend, double *intenstart, double *intenend, double mu);
void inlin(double wave, linedata *line, int *nline, linelist *list, int nlist, atominfo *atom, isodata *isotope);
void pop(linedata *line, int N, atmosphere *model, pfunc *V, population *POP);
void broad(atmosphere *model,linedata *line,int N,double sig,double alp,double fac);
void capnu(linedata *line, int N, atmosphere *model);
void taukap(double wave, atmosphere *model, atominfo *atom, linedata *line, int N, linedata *strgln, pfunc *V, Helium *He, population *POP);
void inatom(char atmdat[], atominfo *atom, double MH, double *ah, double *ahe);
void linelst(double wave, linelist *list, int *nlist, atominfo *atom, double teff, double logg, FILE *qf, int reset, isodata *isotope, atmosphere *model, double Flux, pfunc *V, population *POP, double dwave);
void pfinit(pfunc *V, atominfo *atom, atmosphere *model, int flagw);
void popinit(population *POP, atominfo *atom, atmosphere *model, pfunc *V, int flagw);
double depth(atmosphere *model, double wave, double Flux);
double depthflx(atmosphere *model, double wave, double wstart, double wend);
double depthmu(atmosphere *model, double wave, double Intensity, double mu);
double idepthmu(atmosphere *model, double wave, double mu, double wstart, double wend);
double opacity(atmosphere *model, double lambda, int ntau);
float **cmatrix(int nrl,int nrh,int ncl,int nch);
void nrerror(char error_text[]);
char *ggets(char *s);
void inisotope(char isofile[], isodata *isotope);
void isorelabun(isodata *isotope);
void invelgrad(char vgrad[80]);
void infix(char fixfile[], atominfo *atom, double ah);
double interval(double wave, double dwave, int flagf);
void setreset(int k);
double dmax(double x, double y),dmin(double x, double y);
int imax(int x, int y),imin(int x, int y);

// Functions at the end of this file
// - For macroturbulence
double expiu(double u);
double qsimp(double (*func)(double), double a, double b);
double Mt(double lam);
void four1(double data[],unsigned long nn,int isign);
void realft(double data[], unsigned long n, int isign);
void twofft(double data1[], double data2[], double fft1[], double fft2[],
	    unsigned long n);
void convlv(double data[], unsigned long n, double respns[], unsigned long m,
            int isign, double ans[]);
double trapzd(double (*func)(double), double a, double b, int n);
double *dvector();
void nrerror();
char *ggets(char *s);
#define SWAP(a,b) {double temp=(a);(a)=(b);(b)=temp;}
static double sqrarg;
#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)
#define EPS 1.0e-6
#define JMAX 20
#define FUNC(x) ((*func)(x))

double Zrt = 1.0;
double sqrtpi = 1.772453850905516;
double pi = 3.141593;
// Light speed in vacuum
double lightspeed = 299792.458; // km/s
double sqrt2 = 1.414214;

// - For resolution:
long intdiv();
long mmin(),mmax();

int Ntau;
float **bkap;
float **bkap2;
float **bkap3;
float **bkap4;
double inc;
//////////////////////////////////

int flagr;
int flagc;
int flagk;
int flagg;
int flagmgh;
int flagI;
int flagt;
int flagp;
int flagP;
int flagu;
int flagO;
int flagC;
int mghla;
int mghlb;
float *velgrad;
double mu;
int NI;
// variables for isotopes
double ra1H,ra2H,ra12C,ra13C,ra14N,ra15N,ra16O,ra17O,ra18O;
double ra24Mg,ra25Mg,ra26Mg,ra28Si,ra29Si,ra30Si,ra40Ca,ra42Ca;
double ra43Ca,ra44Ca,ra46Ca,ra48Ca,ra46Ti,ra47Ti,ra48Ti,ra49Ti;
double ra50Ti;
//memo reset;
FILE *opout;
//linedata *oneline;
memo reset;
FILE *opout;
linedata *oneline;


int synthesize_spectrum(char *atmosphere_model_file, char *linelist_file, char *abundances_file, double microturbulence_vel, int verbose, int num_measures, const double waveobs[], double fluxes[], progressfunc user_func, void *user_data) {
    int i;
    int nline = 0;
    int nlist = 0;
    
    
    int flagw = verbose; // Verbose mode (1: True, 0: False)
    int flaga = 0;
    int flagG = 0;
    int flagN = 0;
    int flagx = 0;
    double ah,ahe,waveref,wave,dwave=0,Flux,Depth;
    double vturb = 2.0e+5;
    atmosphere *model;
    atominfo *atom;
    linelist *list;
    linedata *line;
    linedata *strgln;
    isodata *isotope;
    pfunc *V;
    population *POP;
    Helium *He;
    char *file,name[80],lines[80],atmdat[80];
    char fixfile[80];
    char vgrad[80],isofile[80];
    FILE *qf;
    double opwave;

    setreset(0);
    ///////////////////////// Arguments
    strcpy(name, atmosphere_model_file); // stellar atmosphere data file
    file = name;
    strcpy(lines, linelist_file);
    strcpy(atmdat, abundances_file);        // atomic data file
    strcpy(isofile, "isotope.iso"); // only used if global flagI == 1 but then it produces segmentation fault (original SPECTRUM problem)
    //strcpy(oname, output_file); // output file
    //ofile = oname;
    vturb = microturbulence_vel; // km/s, if flagu == 0
    //start = wave_base; // Begin wavelength
    //end = wave_top; // End wavelength
    //dwave = wave_step; // step
    //inc = dwave;
    strcpy(vgrad,"velgrad.dat"); // velocity gradient file only used if flagg == 1
    /////////////////////////


    if(flagG == 1) flagg = 1;
    if(flagP == 1) flagp = 1;
    if(flagN == 1) flagw = 0;
    if((model = (atmosphere *) calloc(1,sizeof(atmosphere))) == NULL)
        nrerror("Allocation of memory for atmosphere failed");
    if((V = (pfunc *) calloc(1,sizeof(pfunc))) == NULL)
        nrerror("Allocation of memory for pfunc failed");
    if((atom = (atominfo *) calloc(NATOM,sizeof(atominfo))) == NULL)
        nrerror("Allocation of memory for atom failed");
    if((POP = (population *) calloc(NATOM-NMOL,sizeof(population))) == NULL)
        nrerror("Allocation of memory for population failed");
    if((line = (linedata *)
    calloc((unsigned long)NLINE,(unsigned long)sizeof(linedata))) == NULL)
        nrerror("Allocation of memory for line failed");
    if((oneline = (linedata *)
    calloc((unsigned long)1,(unsigned long)sizeof(linedata))) == NULL)
        nrerror("Allocation of memory for oneline failed");
    if((strgln = (linedata *)
        calloc(NSTRG,(unsigned long)sizeof(linedata))) == NULL)
        nrerror("Allocation of memory for caii failed");
    if((list = (linelist *) calloc(NLIST,sizeof(linelist))) == NULL)
        nrerror("Allocation of memory for list failed");
    if((He = (Helium *) calloc(NHE,sizeof(Helium))) == NULL)
        nrerror("Allocation of memory for He failed");
    if((velgrad = (float *) calloc(NTAU,sizeof(float))) == NULL)
        nrerror("Allocation of memory for velgrad failed");
    if((isotope = (isodata *) calloc(500,sizeof(isodata))) == NULL)
        nrerror("Allocation of memory for isotope failed");
    bkap = cmatrix(0,3,0,NTAU);
    bkap2 = cmatrix(0,3,0,NTAU);
    bkap3 = cmatrix(0,3,0,NTAU);
    bkap4 = cmatrix(0,3,0,NTAU);
    if(flagN != 1) {
        printf("\nSPECTRUM, a Stellar Spectral Synthesis Program"); 
        printf("\n(C) Richard O. Gray 1992 - 2010 Version 2.76e");
        printf("\nMay 3, 2010");
        printf("\n* Linked to Python by Sergi Blanco Cuaresma - February 2012\n");
        printf("\nIntegrated Disk mode (normalized Intensity)\n\n");
        if(flagc == 1) printf("Output will be continuum only (no line absorption)\n");
        if(flagw == 0) printf("Silent mode\n");
        if(flagg == 1) printf("Velocity gradient mode\n");
        if(flagI == 1) printf("Isotopes enabled\n");
        if(flagu == 1) printf("Reading microturbulent velocity from atmosphere model\n");
    }

    // stellar atmosphere data file
    inmodel(model,file,flagw);
    // Velocity gradient    
    if(flagg == 1) invelgrad(vgrad);
    if(flagg == 0) for(i=0;i<Ntau;i++) velgrad[i] = 0.0;
    
    // Enter name of line list file: (default = luke.lst)
    if((qf = fopen(lines,"r")) == NULL) {
        printf("Cannot find line data file\n");
        return(1);
    }
    if(flagI == 1) {
        // Enter name of isotope data file (default = isotope.iso)
        inisotope(isofile,isotope);
        isorelabun(isotope);
    }
    
    if(flagu == 0) {
        // microturbulence from km/s to cm/s
        vturb *= 1.0e+05;
        for(i=0;i<Ntau;i++) model->mtv[i] = vturb;
    }

    /* Normal abundances for hydrogen and helium; inatom may change these */
    ah = 0.911;
    ahe = 0.089;
    inatom(atmdat,atom,model->MH,&ah,&ahe);
    if(flaga == 1) printf("\nHydrogen abundance = %5.3f     Helium = %5.3f\n",ah,ahe);

    if(flagx == 1) infix(fixfile,atom,ah);

    if(flagw == 1) printf("\n");
    pfinit(V,atom,model,flagw);
    nline = 0;
    nlist = 0;
    if(flagw == 1) printf("Calculating Number Densities\n");
    if(model->teff >= 23500.0) veryhotDensity(model,atom,ah,ahe,flagw);
    else if(model->teff <= 8500.0) Density(model,atom,ah,ahe,flagw);
    else hotDensity(model,atom,ah,ahe,flagw);
    
    popinit(POP, atom, model, V, flagw);
    

    if(flagO == 1) {
        // Enter wavelength for opacity output
        opwave = 5000.0;
        opout = fopen("opacity.out","w");
        for(i=0;i<Ntau;i++) opacity(model,opwave,i);
        fclose(opout);
    }

    waveref = 5000.0;
    if(flagw == 1) printf("Calculating Reference Opacities\n");
    tauref(model,waveref);
    
    
    if(flagw == 1) printf("Entering Main Loop\n");

    int pos = 0;
    double dwave1 = dwave;
    double dwave2 = dwave;
    while(pos < num_measures) {
        wave = waveobs[pos];
        
        if (pos == 0) {
            dwave = fabs(waveobs[pos+1] - waveobs[pos]);
        } else if (pos == num_measures-1) {
            dwave = fabs(waveobs[pos] - waveobs[pos-1]);
        } else {
            // Choose the smallest to avoid the effects derived from
            // spectrum that is cutted in segments
            dwave1 = fabs(waveobs[pos] - waveobs[pos-1]);
            dwave2 = fabs(waveobs[pos+1] - waveobs[pos]);
            if (dwave1 < dwave2) {
                dwave = dwave1;
            } else {
                dwave = dwave2;
            }
        }
        inc = dwave;
        
        Depth = 1.0;

        tauwave(model,wave);
        
        Flux = flux(model,wave);
        
        if (pos == 0) {
            // Reset static vars such as last wave (argument 8)
            linelst(wave,list,&nlist,atom,model->teff,model->logg,qf,1,isotope,model,Flux,V,POP,dwave);
        } else {
            linelst(wave,list,&nlist,atom,model->teff,model->logg,qf,0,isotope,model,Flux,V,POP,dwave);
        }
        inlin(wave,line,&nline,list,nlist,atom,isotope);
        for(i=0;i<nline;i++) {
            if(line[i].flag == 0) {
                pop(line,i,model,V,POP);
                broad(model,line,i,line[i].sig,line[i].alp,line[i].fac);
                capnu(line,i,model);
            }
        }
        taukap(wave,model,atom,line,nline,strgln,V,He,POP);
        Depth = depth(model,wave,Flux);
        
        
        fluxes[pos] = 1.0 - Depth;
        
        if (pos % 100 == 0) {
            if(flagw == 1) printf("Wavelength %9.3f - Work completed %.2f\%\n", wave, ((1.0*pos)/num_measures)*100.0);
            user_func(((1.0*pos)/num_measures)*100.0, user_data);
        }
    
        pos++;        
    }
    fclose(qf);
    
    free(model);
    free(V);
    free(atom);
    free(POP);
    free(line);
    free(oneline);
    free(strgln);
    free(list);
    free(He);
    free(velgrad);
    free(isotope);
}

// Expects waveobs to be homogeneusly spaced and at least of length 2
int macroturbulence_spectrum(const double waveobs[], double fluxes[], int num_measures, double macroturbulence, int verbose, progressfunc user_func, void *user_data) {
    int i;
    // Macroturbulence
    ////////////////////
    //double dlam = 0.01; // Spacing in armstrongs
    double dlam = waveobs[1] - waveobs[0];
    unsigned long k,n,m,N,M;
    double lam,lam0;
    double intspec1 = 0.0;
    double intspec2 = 0.0;
    double *data,*ans,*respns,*wav;
    lam0 = (waveobs[num_measures-1]+waveobs[0])/2.0;

    Zrt = lam0*(macroturbulence/lightspeed);

    n = num_measures;
    m = 10.0*Zrt/dlam;
    if(m%2 == 0) m++;
    M = num_measures + m;
    N = 2;
    while(N < M) N *= 2;

    // WARNING: dvector returns a pointer shifted by 1, so wav[0] is wrong and wav[N] correct
    wav = dvector(1,N);
    data = dvector(1,N);
    ans = dvector(1,2*N);
    respns = dvector(1,N);
    printf("%i %i\n", num_measures, N);
    intspec1 = 0.5*(fluxes[0] + fluxes[num_measures-1]);
    for(i=2;i<num_measures;i++) intspec1 += fluxes[i];
    
    // First part equal to the synthetic spectra
    for(i=0;i<num_measures;i++) {
        wav[i+1] = waveobs[i];
        data[i+1] = fluxes[i];
    }
    // Second part to zero
    for(i=num_measures;i<N;i++) wav[i+1] = data[i+1] = 0.0;
    for(i=0;i<N;i++) respns[i+1] = 0.0;

    lam = 0.0;
    for(i=1;i<=m/2+1;i++) {
        respns[i] = Mt(lam);
        lam += dlam;
    } 
    lam = dlam;
    for(i=m;i>m/2+1;i--) {
        respns[i] = Mt(lam);
        lam += dlam;
    }

    convlv(data,N,respns,m,1,ans);

    intspec2 = 0.5*(ans[1] + ans[num_measures]);
    for(i=2;i<num_measures;i++) intspec2 += ans[i];

    for(i=0;i<num_measures;i++) {
        fluxes[i] = (intspec1/intspec2)*ans[i+1];
    }
    //printf("%9.3f %e\n", wav[i+1],(intspec1/intspec2)*ans[i+1]);
    
    return(0);
}


// Expects waveobs to be homogeneusly spaced and at least of length 2
// vsini is rotation in km/s
int rotation_spectrum(const double waveobs[], double fluxes[], int num_measures, double vsini, double limb_darkening_coeff, int verbose, progressfunc user_func, void *user_data) {
    int i;
    // ROTATION
    double modified_fluxes[num_measures];
    convolv(waveobs, fluxes, modified_fluxes, num_measures, vsini, limb_darkening_coeff);

    for(i=0;i<num_measures;i++){
        fluxes[i] = modified_fluxes[i];
    }

    return(0);
}


// Convolve by a given resolution
int resolution_spectrum(const double waveobs[], double fluxes[], int num_measures, int R, int verbose, progressfunc user_func, void *user_data) {
    int i;
    double wave;
    double dwave = 0;
    double dwave1 = dwave;
    double dwave2 = dwave;
        
    // Copy the synthetic fluxes with infinite resolution
    double original_fluxes[num_measures];
    for(i=0;i<num_measures;i++){
        original_fluxes[i] = fluxes[i];
    }
    long k;
    //double wave = waveobs[0]; // First wavelength
    wave = waveobs[0]; // First wavelength
    double res;
    long n, low, high;
    double n1, n2, a, sum1, sum2, z, Ds;

    i = 0;
    while(i < num_measures) {
        res = waveobs[i]/R;
        res /= 2.0;

        // Spacing
        if (i == 0) {
            dwave = fabs(waveobs[i+1] - waveobs[i]);
        } else if (i == num_measures-1) {
            dwave = fabs(waveobs[i] - waveobs[i-1]);
        } else {
            // Choose the smallest to avoid the effects derived from
            // spectrum that is cutted in segments
            dwave1 = fabs(waveobs[i] - waveobs[i-1]);
            dwave2 = fabs(waveobs[i+1] - waveobs[i]);
            if (dwave1 < dwave2) {
                dwave = dwave1;
            } else {
                dwave = dwave2;
            }
        }

        n = intdiv(res,dwave);
        a = -log10(0.50)/(n*n);
        low = (long) mmax(0, i-3*n);
        high = (long) mmin(num_measures-1, i+3*n);
        sum1 = sum2 = 0.0;
        for(k=low;k<=high;k++) {
            z = pow(10.0,-a*(k-i)*(k-i));
            sum1 += original_fluxes[k]*z;
            sum2 += z;
        }
        Ds = sum1/sum2;
        //printf("%8.3f %g  %g  %i\n",wave, original_fluxes[i], Ds, i);
        fluxes[i] = Ds;
        i += 1;
    }

    return(0);
}

/////////////////////////////////////////////////////////////////////////////////
// AUXILIARY FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////

/// FUNCTIONS for MACROTURBULENCE
double Mt(double lam)
{
  double integral;
  double smalllam = 1.0e-16;

  if(lam == 0.0) 
    integral = (2.0*smalllam/(sqrtpi*Zrt*Zrt))*qsimp(expiu,0,Zrt/smalllam);
  else integral = (2.0*lam/(sqrtpi*Zrt*Zrt))*qsimp(expiu,0,Zrt/lam);
  return(integral);
}


double expiu(double u)
{
  if(u == 0.0) return(0.0);

  return(exp(-1.0/(u*u)));
}
	 

void four1(double data[],unsigned long nn,int isign)
{
  unsigned long  n,mmax,m,j,istep,i;
  double wtemp,wr,wpr,wpi,wi,theta;
  double tempr,tempi;

  n = nn << 1;
  j = 1;
  for(i=1;i<n;i+=2) {
    if(j>i) {
      SWAP(data[j],data[i]);
      SWAP(data[j+1],data[i+1]);
    }
    m = nn;
    while(m >= 2 && j > m) {
      j -= m;
      m >>= 1;
    }
    j += m;
  }
  mmax = 2;
  while(n > mmax) {
    istep = mmax << 1;
    theta = isign*(6.28318530717959/mmax);
    wtemp = sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi = sin(theta);
    wr = 1.0;
    wi = 0.0;
    for(m=1;m<mmax;m+=2) {
      for(i=m;i<=n;i+=istep) {
	j = i+mmax;
	tempr = wr*data[j]-wi*data[j+1];
	tempi = wr*data[j+1]+wi*data[j];
	data[j] = data[i]-tempr;
	data[j+1]=data[i+1]-tempi;
	data[i] += tempr;
	data[i+1] += tempi;
      }
      wr = (wtemp=wr)*wpr - wi*wpi+wr;
      wi = wi*wpr + wtemp*wpi + wi;
    }
    mmax = istep;
  }
}


void realft(double data[], unsigned long n, int isign)
{
   void four1(double data[], unsigned long nn, int isign);
   unsigned long i,i1,i2,i3,i4,np3;
   double c1=0.5,c2,h1r,h1i,h2r,h2i;
   double wr,wi,wpr,wpi,wtemp,theta;

   theta=M_PI/(double) (n>>1);
   if (isign == 1) {
      c2 = -0.5;
      four1(data,n>>1,1);
   } else {
      c2=0.5;
      theta = -theta;
   }
   wtemp=sin(0.5*theta);
   wpr = -2.0*wtemp*wtemp;
   wpi=sin(theta);
   wr=1.0+wpr;
   wi=wpi;
   np3=n+3;
   for (i=2;i<=(n>>2);i++) {
      i4=1+(i3=np3-(i2=1+(i1=i+i-1)));
      h1r=c1*(data[i1]+data[i3]);
      h1i=c1*(data[i2]-data[i4]);
      h2r = -c2*(data[i2]+data[i4]);
      h2i=c2*(data[i1]-data[i3]);
      data[i1] = h1r+wr*h2r-wi*h2i;
      data[i2] = h1i+wr*h2i+wi*h2r;
      data[i3] = h1r-wr*h2r+wi*h2i;
      data[i4] = -h1i+wr*h2i+wi*h2r;
      wr=(wtemp=wr)*wpr-wi*wpi+wr;
      wi=wi*wpr+wtemp*wpi+wi;
   }
   if (isign == 1) {
      data[1] = (h1r=data[1])+data[2];
      data[2] = h1r-data[2];
   } else {
      data[1]=c1*((h1r=data[1])+data[2]);
      data[2]=c1*(h1r-data[2]);
      four1(data,n>>1,-1);
   }
}


void twofft(double data1[], double data2[], double fft1[], double fft2[],
      unsigned long n)
/* Given two real input arrays data1[1..n] and data2[1..n], this routine
calls four1 and returns two complex output arrays, fft1[1..2n] and
fft2[1..2n], each of complex length n (i.e., real length 2*n), which
contain the discrete Fourier transforms of the respective data arrays. n
MUST be an integer power of 2. */
{
   void four1(double data[], unsigned long nn, int isign);
   unsigned long nn3,nn2,jj,j;
   double rep,rem,aip,aim;
   nn3=1+(nn2=2+n+n);
   for (j=1,jj=2;j<=n;j++,jj+=2) { 
   /* Pack the two real arrays into one complex array. */
      fft1[jj-1]=data1[j];
      fft1[jj]=data2[j];
   }
   four1(fft1,n,1); /* Transform the complex array. */
   fft2[1]=fft1[2];
   fft1[2]=fft2[2]=0.0;
   for (j=3;j<=n+1;j+=2) {
      /* Use symmetries to separate the two transforms. */
      rep=0.5*(fft1[j]+fft1[nn2-j]); 
      rem=0.5*(fft1[j]-fft1[nn2-j]);
      aip=0.5*(fft1[j+1]+fft1[nn3-j]);
      aim=0.5*(fft1[j+1]-fft1[nn3-j]);
      fft1[j]=rep; /* Ship them out in two complex arrays. */
      fft1[j+1]=aim;
      fft1[nn2-j]=rep;
      fft1[nn3-j] = -aim;
      fft2[j]=aip;
      fft2[j+1] = -rem;
      fft2[nn2-j]=aip;
      fft2[nn3-j]=rem;
   }
}

void convlv(double data[], unsigned long n, double respns[], unsigned long m,
            int isign, double ans[])
/* Convolves or deconvolves a real data set data[1..n] (including any
user-supplied zero padding) with a response function respns[1..n]. The
response function must be stored in wrap-around order in the first m
elements of respns, where m is an odd integer.  Wrap-around order
means that the first half of the array respns contains the impulse
response function at positive times, while the second half of the array
contains the impulse response function at negative times, counting down
from the highest element respns[m]. On input isign is +1 for
convolution, n. The answer is returned in the first n components of ans.
However, ans must be supplied in the calling program with dimensions
[1..2*n], for consistency with twofft. n MUST be an integer power of
two. */
{
   void realft(double data[], unsigned long n, int isign);
   void twofft(double data1[], double data2[], double fft1[], double fft2[],
        unsigned long n);
   unsigned long i,no2;
   double dum,mag2,*fft;

   fft=dvector(1,n<<1);
   for (i=1;i<=(m-1)/2;i++) /* Put respns in array of length n. */
      respns[n+1-i]=respns[m+1-i];
   for (i=(m+3)/2;i<=n-(m-1)/2;i++) /* Pad with zeros. */
      respns[i]=0.0;
   twofft(data,respns,fft,ans,n); /* FFT both at once. */
   no2=n>>1;
   for (i=2;i<=n+2;i+=2) {
      if (isign == 1) {
	 /* Multiply FFTs to convolve. */
         ans[i-1]=(fft[i-1]*(dum=ans[i-1])-fft[i]*ans[i])/no2; 
         ans[i]=(fft[i]*dum+fft[i-1]*ans[i])/no2;
      } else if (isign == -1) {
         if ((mag2=SQR(ans[i-1])+SQR(ans[i])) == 0.0)
            nrerror("Deconvolving at response zero in convlv");
	 /* Divide FFTs to deconvolve. */ 
         ans[i-1]=(fft[i-1]*(dum=ans[i-1])+fft[i]*ans[i])/mag2/no2; 
	 ans[i]=(fft[i]*dum-fft[i-1]*ans[i])/mag2/no2;
      } else nrerror("No meaning for isign in convlv");
   }
   ans[2]=ans[n+1]; /* Pack last element with first for realft. */
   realft(ans,n,-1); /* Inverse transform back to time domain. */
   free_dvector(fft,1,n<<1);
}


double trapzd(double (*func)(double), double a, double b, int n)
{
  double x,tnm,sum,del;
  static double s;
  int it,j;

  if(n == 1) {
    return(s=0.5*(b-a)*(FUNC(a)+FUNC(b)));
  } else {
    for(it=1,j=1;j<n-1;j++) it <<= 1;
    tnm = it;
    del=(b-a)/tnm;
    x = a+0.5*del;
    for(sum=0.0,j=1;j<=it;j++,x+=del) sum += FUNC(x);
    s = 0.5*(s+(b-a)*sum/tnm);
    return s;
  }
}

double qsimp(double (*func)(double), double a, double b)
{
  double trapzd(double (*func)(double), double a, double b, int n);
  void nrerror(char error_text[]);
  int j;
  double s,st,ost,os;

  ost = os = -1.0e30;

  for(j=1;j<=JMAX;j++) {
    st = trapzd(func,a,b,j);
    s = (4.0*st-ost)/3.0;
    if(fabs(s-os) < EPS*fabs(os)) return s;
    if(s == 0.0 && os == 0.0 && j > 6) return s;
    os = s;
    ost = st;
  }
  nrerror("Too many steps in routine qsimp");
  return 0.0;
}

/// FUNCTIONS for V SIN(i) - Rotation
void convolv(waveobs,y,ys,num,vsini,u)
double *waveobs;
double *y,*ys;
double vsini,u;
long num;
{
  double beta,gam,w,s,t,dlc,c1,c2,dv,r1,r2,f,v;
  long i,n1,n2,n;
  char tmp[10];
  double st = waveobs[0];
  double dw = waveobs[1] - waveobs[0];
  long nd = waveobs[num-1] * vsini/(dw*lightspeed) + 5.5;

  beta = (1.0-u)/(1.0 - 0.333333*u);
  gam = u/(1.0 - 0.333333*u);
  
  /* End Effect */

  n1 = nd + 1;
  for(i=1;i<=nd;i++) ys[i] = y[i];
  n2 = num - nd -1;
  for(i=n2;i<=num;i++) ys[i] = y[i];
  if(vsini < 0.5) {
    for(i=1;i<=num;i++) ys[i] = y[i];
    return;
  }

  /* convolve with rotation profile */

   w = st + (n1-1)*dw;
   for(n=n1;n<=n2;n++) {
     w = w+dw;
     s = 0.0;
     t = 0.0;
     dlc = w*vsini/lightspeed;
     c1 = 0.63661977*beta/dlc;
     c2 = 0.5*gam/dlc;
     dv = dw/dlc;

     for(i=-nd;i<=nd;i++) {
       v = i*dv;
       r2 = 1.0 - v*v;
       if(r2 > 0.0) {
         f = c1*sqrt(r2) + c2*r2;
         t = t+f;
         s = s + f*y[n+i];
       }
     }
     ys[n] = s/t;
   }
   return;
}



/// FUNCTIONS for RESOLUTION
long intdiv(a,b)
double a,b;
{
  double n1,n2,ab;
  long n;

  ab = a/b;
  n1 = floor(ab);
  n2 = ceil(ab);
  if(fabs(ab-n1) < fabs(ab-n2)) n = n1;
  else n = n2;

  return(n);
}

long mmin(m,n)
long m,n;
{
  if(m <= n) return(m);
  else return(n);
}

long mmax(m,n)
long m,n;
{
  if(m >= n) return(m);
  else return(n);
}

