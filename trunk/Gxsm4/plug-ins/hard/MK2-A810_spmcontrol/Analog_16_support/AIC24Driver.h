/*___________________________________________________________________________

Structure and function defintion of the AIC24Driver
	
___________________________________________________________________________*/     

#define	HPIC		0xA018
#define Acknowledge	0x0107
#define MaxCh 		16
#define SATVAL		31000	// Absolute value of saturation detection

extern int      NbrChByPort;
extern void	start_aic24();
extern void	stop_aic24();

struct aicreg_rec 	{	
	int pReg4_m[16];
	int pReg4_n[16];
	int Reg3A [16];
	int Reg3B [16];
	int Reg3C [16];
	int Reg4_m [16];
	int Reg4_n [16];
	int Reg5A [16];
	int Reg5B [16];
	int Reg5C [16];
	int Reg6A [16];
	int Reg6B [16];
	int Reg1 [16];
	int Reg2 [16];
};

struct iobuf_rec 	{	
	int min[32];
	int mout[32];
};
						
					
									
