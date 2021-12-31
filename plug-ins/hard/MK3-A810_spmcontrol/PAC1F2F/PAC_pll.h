// Maximum size for the acquisition buffer

#define MaxSizeSigBuf 1000000

// Functions declarations

extern int StartPLL(int ioIN, int ioOUT);	// Must be called by the DPS only  (Return 1 if PLL option activated Return 0 if not)
extern void DataprocessPLL();				// Must be called by the DPS only
extern void ChangeFreqHL();					// Must be called at high level by the PC
extern void OutputSignalSetUp_HL();			// Must be called at high level by the PC
extern void	TestPhasePIResp_HL();			// Must be called at high level by the PC
extern void TestAmpPIResp_HL();				// Must be called at high level by the PC

// Variables ******************************

// PLL Buffers for the acquition 

extern	int Signal1[MaxSizeSigBuf];
extern	int Signal2[MaxSizeSigBuf];
	
// Pointers for the acsiquition

extern	int		*pSignal1; 
extern	int		*pSignal2; 
	
// Block length

extern	int 	blcklen; // Max MaxSizeSigBuf-1 
	
// Sine variables

extern	int deltaReT; 
extern	int deltaImT; 
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
extern	int setpoint_Amp;
extern	int pcoef_Amp,icoef_Amp; 
extern	int errorPI_Amp; 
extern	long long memI_Amp;
	
// LP filter selections
	
extern	int LPSelect_Phase;
extern	int LPSelect_Amp; 

// Other variables

extern int outmix_display[4]; // Must be set at 7 to bypass the PLL output generation
extern int InputFiltered;
extern int SineOut0; 
extern int phase;
extern int amp_estimation;
extern int Filter64Out[4];

// Mode 2F variable

extern short Mode2F; // 0: mode normal (F on analog out/F on the Phase detector) 1: mode 2F (F on analog out/2F on the Phase detector)

// Output of the sine generator (F and 2F)

extern	int cr1; // Sine F (RE)
extern	int ci1; // Sine F (IM)
extern	int cr2; // Sine 2F (RE)
extern	int ci2; // Sine 2F (IM)
	
	
	
