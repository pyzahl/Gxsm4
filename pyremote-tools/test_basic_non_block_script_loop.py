gxsm.set('script-control','1')
i=0
while int(gxsm.get('script-control')) > 0:
	print (i)
	i=i+1
	gxsm.sleep(10)