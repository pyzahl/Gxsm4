#!/usr/bin/env python

import numpy as np

name = 'Fomic Acid'

molOz = 0.0  # 7.363321 -4.167425 

molxyz = [
    [ ' O',    0.0000,    0.0000,    molOz, -0.1],
    [ ' C',    1.2000,    0.0000,    molOz,  0.18],
    [ ' O',    1.7952,    1.1782,    molOz, -0.1],
    [ ' H',    1.2000,   -1.1000,    molOz,  0.01],
    [ ' H',    1.7952, 1.1782+0.97,  molOz,  0.01]
]

# C-H 1.10
# O-H 0.97

# Cu110 2.5 , 3.6 


Cu110_O_xyz = [
    [ 'Cu',     0.0000,  0.0000,  0.0000,  0.1 ],
    [ 'Cu',   2*2.5500,  0.0000,  0.0000,  0.1 ],
    [ ' O',     0.0000,  1.8000, -0.1000, -0.1 ],
    [ ' O',   2*2.5500,  1.8000, -0.1000, -0.1 ],
]

Cu110_O_ro = np.matrix ([[5.1,0, 0, 0], [0, 3.6, 0, 0]])

CuO_nr = 5
CuO_no = 5
CuOn = 4 * CuO_nr * CuO_no

# Molecule 1

i=0
v = []
for a in molxyz:
    v.append ( np.array ([ a[1], a[2], a[3], a[4] ] ) )

i=0

mn = np.size (v)/3
#natoms = 3*mn + CuOn
natoms = 1*mn + CuOn

print natoms
print name

# Molecule #1
i=0
r=np.array ([5.1/2-1.15, 0, 0, 0])

phi = np.radians(-30)
c, s = np.cos(phi), np.sin(phi)
R = np.matrix([[c, -s, 0, 0], [s, c, 0, 0], [0,0,1,0], [0,0,0,1]])

for a in v:
    a = np.dot (R,a)
    a = a + r
    print molxyz[i][0],  a[(0,0)],  a[(0,1)],  a[(0,2)], a[(0,3)]
    i=i+1        

# Molecule 2
i=0
r=np.array ([1, 3.6, 0, 0])

phi = np.radians(180)
c, s = np.cos(phi), np.sin(phi)
R = np.matrix([[c, -s, 0, 0], [s, c, 0, 0], [0,0,1,0], [0,0,0,1]])

for a in v:
    a = np.dot (R,a)
    a = a + r
#    print molxyz[i][0],  a[(0,0)],  a[(0,1)],  a[(0,2)], a[(0,3)]
    i=i+1        

# Molecule 3
i=0
r=np.array ([0, 2*3.6, 0, 0])

phi = np.radians(0)
c, s = np.cos(phi), np.sin(phi)
R = np.matrix([[c, -s, 0, 0], [s, c, 0, 0], [0,0,1,0], [0,0,0,1]])

for a in v:
    a = np.dot (R,a)
    a = a + r
#    print molxyz[i][0],  a[(0,0)],  a[(0,1)],  a[(0,2)], a[(0,3)]
    i=i+1        


## Cu(110) / O

i=0
v = []
for a in Cu110_O_xyz:
    v.append ( np.array ([ a[1], a[2], a[3], a[4] ] ) )

r0 = -round(CuO_nr/2) * Cu110_O_ro[(0)] + -round(CuO_no/2-1) * Cu110_O_ro[(1)]
# print 'Cu110/O'
for r in range (0, CuO_nr):
    for o in range (0, CuO_no):
        i=0
        for a in v:
#            print r,o
            a = a + r*Cu110_O_ro[(0)] + o*Cu110_O_ro[(1)] + r0
#            print a
            print Cu110_O_xyz[i][0],  a[(0,0)],  a[(0,1)],  a[(0,2)], a[(0,3)]
            i=i+1

    



    
