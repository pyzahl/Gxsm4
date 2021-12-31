/* COMMAND FILE TO LINK GXSM FB_SPM PROGRAM */
-l AICDriver.lib
-l rts500.lib

/* -stack=0x200 */

MEMORY
{
PAGE 0:
      RESERVED    (R   ): origin= 0x0000 length= 0x0080 /* unavailable for allocation */
      vectors     (RWX ): origin= 0x0080 length= 0x0080 /* interrupt vectors */

      /* Kernel - do not place code or data on top of the kernel */
      RESERVED_2  (R   ): origin= 0x0100 length= 0x0200

      /* On-chip Dual Access RAM minus the mail box */
      DARAM       (RWX ): origin= 0x0300 length= 0x0D00

      /* Mail box - do not place any code in it to keep the kernel happy */
      MailBox     (RW  ): origin= 0x1000 length= 0x0023

      /* On-chip Dual Access RAM (cont.) */
      DARAM1      (RWX ): origin= 0x1023 length= 0x17DD

      Expansion   (RWX ): origin= 0x4000 length= 0xC000 /* Off-chip memory */

PAGE 1 :
      DARAM2      (RWX ): origin= 0x2800 length= 0x1600
      STACK       (RWX ): origin= 0x3E00 length= 0x0200
      regs        (RW  ): origin= 0x0000 length= 0x005F /* Memory-mapped registers */
      scratch     (RW  ): origin= 0x0060 length= 0x0020 /* Scratch-pad DARAM */
      MagicData   (RW  ): origin= 0x4000 length= 0x0020 /* Communication with Gxsm magic data location */
      External_RAM(RW  ): origin= 0x4020 length= 0xFFE0 /* Off-chip memory */


PAGE 2 :
      peripheral  (RW  ): origin= 0x0000 length= 0x10000 /* not populated yet */
}


SECTIONS 
{
	.vectors        > vectors
	.pinit		> DARAM | DARAM1
	.cinit		> DARAM | DARAM1
	.text1		> DARAM  {dataprocess.obj, FB_spm_statemaschine.obj, spm_log.obj, feedback.obj, feedback_linear.obj}
	.text		> DARAM | DARAM1
	dmabuf		> DARAM | DARAM1
	.const          > DARAM2                  /* uninitialized, PAGE 1 */
	.data		> DARAM2                  /* uninitialized, PAGE 1 */
	.stack          > STACK                   /* uninitialized, PAGE 1 */
	.bss		> DARAM2                  /* uninitialized, PAGE 1 */
}
