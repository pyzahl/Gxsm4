import time
gxsm.set('script-control','1')
print("Starting loop")
keep_going = True
i=0
while keep_going and int(gxsm.get('script-control')) > 0:
    try:
        #gxsm.sleep(10)
        time.sleep(0.1)
        print(i)
        i=i+1
    except KeyboardInterrupt:
        print("KBInterrupt received, ending")
        keep_going = False

print("End of loop")
