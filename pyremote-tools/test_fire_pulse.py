gxsm.action("EXECUTE_FirePulse")
gxsm.sleep(10)
gxsm.action("EXECUTE_ScopeSaveData")

print(xsm.action("GET_RPDATA_VECTOR"))