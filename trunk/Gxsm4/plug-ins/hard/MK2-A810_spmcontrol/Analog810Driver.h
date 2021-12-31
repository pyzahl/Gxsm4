/*___________________________________________________________________________

	Structure and function defintion of the SR2_Analog810 I/O board
	
___________________________________________________________________________*/     

#define HPIC            0xA018
#define Acknowledge     0x0107
#define MaxCh           16
#define SATVAL          31000   // Absolute value of saturation detection

extern void	start_Analog810();
extern void	stop_Analog810();

struct iobuf_rec {	int min[8];
			int mout[8];
};
						
					
									
