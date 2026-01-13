import sys
sys.modules[__name__].__dict__.clear()

import logging
from string import *
import os
import time


full_path_name = gxsm.chfname(0).split()[0]
ncfname = os.path.basename(full_path_name)
folder = os.path.dirname(full_path_name)
name, ext = os.path.splitext(ncfname)
logfile_name = folder+'/'+name+'-XXX-initial.log'

print(logfile_name)

logging.basicConfig(filename=logfile_name, encoding='utf-8', level=logging.DEBUG, format='%(asctime)s %(message)s')
#logging.basicConfig(filename=logfile_name, encoding='utf-8', level=logging.DEBUG)
#logging.basicConfig(format='%(asctime)s %(message)s')

logging.info('Test  logfile start. [{}]'.format(logfile_name))
logging.debug('This message should go to the log file')
logging.info('So should this')
logging.warning('And this, too')
logging.error('And non-ASCII stuff, too, like Øresund and Malmö')

logging.shutdown()

