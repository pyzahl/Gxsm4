
#ifndef __SCAN_TYPES_H
#define __SCAN_TYPES_H


// this is the type of the current active object...
// we need to get rid of this...
typedef enum { 
        MNONE,
        MPOINT,
        MLINE,
        MRECTANGLE,
        MPOLYLINE,
        MPARABEL,
        MCIRCLE,
        MTRACE,
        MEVENT,
        MKSYS
} OBJECT_TYPE;

// Scan Coordinate System Modes
typedef enum {
	SCAN_COORD_ABSOLUTE, // absolute world coordinates, including offset and rotation
	SCAN_COORD_RELATIVE  // relative world coordinates, no offset, no rotation (in local scan coords)
} SCAN_COORD_MODE;
 
typedef struct {
	int       index;
	double    time;
	int       refcount;
	Mem2d     *mem2d;
	SCAN_DATA *sdata;
} TimeElementOfScan;

#endif
