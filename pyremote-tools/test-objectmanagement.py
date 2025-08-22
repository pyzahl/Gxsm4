import time

if 0:
	gxsm.set('RangeX', '400')
	gxsm.set('RangeY', '400')
	gxsm.set('PointsX', '400')
	gxsm.set('PointsY', '400')
	gxsm.startscan()
	print('scanning for 5 seconds...')
	time.sleep(1)
	gxsm.stopscan()

print('List Objects -- if any:')
o1=gxsm.get_object (0, 0) # Ch0, Obj#0
print(o1)
o2=gxsm.get_object (0, 1) # Ch0, Obj#1
print(o2)
o2=gxsm.get_object (0, 2) # Ch0, Obj#2
print(o2)

print('Removing all Rectangles!')
r=gxsm.marker_getobject_action(0, 'Rectangle','REMOVE-ALL')
print(r)

print('Create Rectangles')
r=gxsm.add_marker_object(0, 'Rectangle[01]', 0xff00fff0, 300,200,100)  # id=Rectangle[01], color: 0xff00fff0 = #RRGGBBAA in hex, rectangle centered at XY[px[=300,200, width=height=100
# the id part '[01]' can be any string, [01], -01, _01, abc, a01, mol01, ....
print(r)
r=gxsm.add_marker_object(0, 'Rectangle[02]', 0xff00fff0, 230,140,70)
print(r)

print('List Objects again:')
o1=gxsm.get_object (0, 0) # Ch0, Obj#0
print(o1)
o2=gxsm.get_object (0, 1) # Ch0, Obj#1
print(o2)
o2=gxsm.get_object (0, 2) # Ch0, Obj#2
print(o2)

print('Get Coordinates, and auto setup geometry for scan accordingly!')
time.sleep(1)
r=gxsm.marker_getobject_action(0, 'Rectangle[01]','GET-COORDS')
print(r)
time.sleep(1)
r=gxsm.marker_getobject_action(0, 'Rectangle[02]','GET-COORDS')
print(r)

time.sleep(1)

print('Renaming')
i=1
label='Test Label'
print(gxsm.marker_getobject_action(0, 'Rectangle[{:02d}]'.format(i),'SET-LABEL-TO:'+label))
gxsm.autodisplay()

time.sleep(1)
print('Cleanup')
r=gxsm.marker_getobject_action(0, 'Rectangle[02]','REMOVE')
print(r)
time.sleep(1)
r=gxsm.marker_getobject_action(0, 'Rectangle[01]','REMOVE')
print(r)

print('restore active scan coords/geometry for scan')
gxsm.chmodea(0)



