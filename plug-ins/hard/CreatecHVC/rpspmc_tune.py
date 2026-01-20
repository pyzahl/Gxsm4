#!/usr/bin/env python3

import time
import gxsm4process as gxsm4
gxsm = gxsm4.gxsm_process()

# Query JSON parameter 'jvn': sec = jsquery_var ('SPMC_UPTIME_SECONDS')
def jsquery_var(jvn, prt=True):
	u,v,w = gxsm.rtquery('X *{}'.format(jvn))
	if prt:
            print ('{} = {}'.format(jvn, u))
	return u

def fmt_version(jvn):
    u =  int(jsquery_var(jvn, False))
    if u < 0:
        u = u + 2**32
    return '{}: {:x}'.format(jvn, u)

# Query: Send JSON data blob to adjust parameter(s): jsset_var((['SPMC_AD463_POST_SYNC_DELAY', 0 ],))
def jsset_var(jvn):
	jps=''
	for jp in jvn:
		if jps > '':
			jps = jps+','
		jps = jps+'"'+ jp[0] +'":{"value":' + '{}'.format(jp[1]) + '}'
	json = '{ "parameters":{' + jps + '}}'
	print ('JSON=', json)
	u,v,w = gxsm.rtquery('X '+json)
	return u

sec=jsquery_var ('SPMC_UPTIME_SECONDS', False)
sec = int(sec)
d, remainder = divmod(sec, 3600*24)
h, remainder = divmod(remainder, 3600)
m, s = divmod(remainder, 60)

# Use string formatting to ensure leading zeros (e.g., 05:02:00)
print (f'RPSPMC Uptime: {d:}days {h:02}:{m:02}:{s:02}')

print ()

print (fmt_version('RPSPMC_SERVER_VERSION'))
print (fmt_version('RPSPMC_SERVER_DATE'))
print (fmt_version('RPSPMC_FPGAIMPL_VERSION'))
print (fmt_version('RPSPMC_FPGAIMPL_DATE'))

print ()

print ('RPSPMC JSON Update Periods in ms:')
jsquery_var('PARAMETER_PERIOD')
jsquery_var('SIGNAL_PERIOD')

mon = gxsm.rt_query_rpspmc()

#print (mon)
print (mon['current'], 'nA')
print (mon['time'], 's')

start_time = time.perf_counter()
# nothing
end_time = time.perf_counter()
elapsed_time = 1e3*(end_time - start_time)
print ('elapsed_time perf itself: ', elapsed_time, ' ms')

start_time = time.perf_counter()
mon = gxsm.rt_query_rpspmc()
end_time = time.perf_counter()
elapsed_time = 1e3*(end_time - start_time)
print ('elapsed_time perf mon reading: ', elapsed_time, ' ms')


## WARNING ##
jsset_var((['PARAMETER_PERIOD', 200 ],))
time.sleep(1)
#jsset_var((['SIGNAL_PERIOD', 100 ],))


print ('measuring... mon updates')
mon = gxsm.rt_query_rpspmc()
tf=mon['time']
ti=tf
si=jsquery_var ('SPMC_UPTIME_SECONDS', False)
for i in range(0,30):
    start_time = time.perf_counter()
    while ti >= tf:
        mon = gxsm.rt_query_rpspmc()
        tf  = mon['time']
    end_time = time.perf_counter()
    elapsed_time = 1e3*(end_time - start_time)
    sec=jsquery_var ('SPMC_UPTIME_SECONDS', False)
    print ('elapsed_time to name parameter update: {:.1f} ms, FPGA: {:.1f} ms,  J: {} ms, I:{} nA'.format(elapsed_time, 1e3*(tf-ti), 1e3*(sec-si), mon['current']))
    ti=tf
    si=sec

