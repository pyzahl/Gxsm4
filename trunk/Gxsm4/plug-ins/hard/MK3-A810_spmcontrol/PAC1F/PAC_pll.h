// Maximum size for the acquisition buffer

#define MaxSizeSigBuf 1000000

// Functions declarations

extern int StartPLL(int ioIN, int ioOUT);	// Must be called by the DPS only  (Return 1 if PLL option activated Return 0 if not)
extern void DataprocessPLL();				// Must be called by the DPS only
extern void ChangeFreqHL();					// Must be called at high level by the PC
extern void OutputSignalSetUp_HL();			// Must be called at high level by the PC
extern void	TestPhasePIResp_HL();			// Must be called at high level by the PC
extern void TestAmpPIResp_HL();				// Must be called at high level by the PC

extern void SineSDB(int *cr, int *ci, int deltasRe, int deltasIm); // SineSDB 64bit precision generator utility

// Variables ******************************

// PLL Buffers for the acquition 

extern	int Signal1[MaxSizeSigBuf];
extern	int Signal2[MaxSizeSigBuf];
	
// Pointers for the acsiquition

extern	int		*pSignal1; 
extern	int		*pSignal2; 
	
// Block length

extern	volatile int 	blcklen; // Max MaxSizeSigBuf-1 
	
// Sine variables

extern	volatile int deltaReT; 
extern	volatile int deltaImT; 
extern	int volumeSine; 
 
// Phase/Amp detector time constant 

extern	int Tau_PAC;

// Phase corrector variables
	
extern	int PI_Phase_Out; 
extern	int pcoef_Phase,icoef_Phase; 
extern	int errorPI_Phase; 
extern	long long memI_Phase; 
extern	int setpoint_Phase; 
extern	int ctrlmode_Phase; 
extern	long long corrmin_Phase; 
extern	long long corrmax_Phase; 
	
// Min/Max for the output signals
	
extern	int ClipMin[4]; 
extern	int ClipMax[4]; 

// Amp corrector variables
	
extern	int ctrlmode_Amp;
extern	long long corrmin_Amp; 
extern	long long corrmax_Amp; 
extern	volatile int setpoint_Amp;
extern	int pcoef_Amp,icoef_Amp; 
extern	int errorPI_Amp; 
extern	long long memI_Amp;
	
// LP filter selections
	
extern	volatile int LPSelect_Phase;
extern	volatile int LPSelect_Amp; 
	
// ---- PZ -----
// added missing variables for monitoring
extern  int InputFiltered;
extern  int SineOut0;
extern  int phase;
extern  int amp_estimation;
extern  int Filter64Out[];

