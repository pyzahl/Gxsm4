# Load scan support object from library
# GXSM_USE_LIBRARY <gxsm4-lib-scan>

## GXSM PyRemote socket server script

import selectors
import socket
import sys
import os
import fcntl
import json

## CONNECTION CONFIG

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

##

sys.stdout.write ("************************************************************\n")
sys.stdout.write ("* GXSM Py Socket Server is listening on "+HOST+":"+str(PORT)+"\n")
sc=gxsm.get ("script-control")
sys.stdout.write ('* Script Control is set to {} currently.\n'.format(int(sc)))
sys.stdout.write ('* Set Script Control >  0 to keep server alife! 0 will exit.\n')
sys.stdout.write ('* Set Script Control == 1 for idle markings...\n')
sys.stdout.write ('* Set Script Control == 2 for silence.\n')
sys.stdout.write ('* Set Script Control >  2 minial sleep, WARNIGN: GUI may be sluggish.\n')
sys.stdout.write ("************************************************************\n\n")

## message processing

def process_message(jmsg):
    #print ('processing JSON:\n', jmsg)
    for cmd in jmsg.keys():
        #print('Request = {}'.format(cmd))
        if cmd == 'command':
            for k in jmsg['command'][0].keys():
                if k == 'set':
                    ## {'command': [{'set': id, 'value': value}]}

                    #print('gxsm.set ({}, {})'.format(jmsg['command'][0]['set'], jmsg['command'][0]['value']))
                    gxsm.set(jmsg['command'][0]['set'], jmsg['command'][0]['value'])
                    return {'result': [{'set':jmsg['command'][0]}]}

                elif k == 'gets':
                    ## {'command': [{'get': id}]}

                    #print('gxsm.gets ({})'.format(jmsg['command'][0]['gets']))
                    value=gxsm.gets(jmsg['command'][0]['gets'])
                    #print(value)
                    return {'result': [{'gets':jmsg['command'][0]['gets'], 'value':value}]}


                elif k == 'get':
                    ## {'command': [{'get': id}]}

                    #print('gxsm.get ({})'.format(jmsg['command'][0]['get']))
                    value=gxsm.get(jmsg['command'][0]['get'])
                    #print(value)
                    return {'result': [{'get':jmsg['command'][0]['get'], 'value':value}]}

                elif k == 'query':
                    ## {'command': [{'query': x, 'args': [ch,...]}]}
                    
                    if (jmsg['command'][0]['query'] == 'chfname'):
                        value=gxsm.chfname(jmsg['command'][0]['args'][0])
                        return {'result': [{'query':jmsg['command'][0]['query'], 'value':value}]}

                    elif (jmsg['command'][0]['query'] == 'y_current'):
                        value=gxsm.y_current()
                        return {'result': [{'query':jmsg['command'][0]['query'], 'value':value}]}

                    else:
                        return {'result': 'invalid query command'}
                else:
                    return {'result': 'invalid command'}

        elif cmd == 'action':
            if (jmsg['action'][0] == 'start-scan'):
                gxsm.startscan ()
                return {'result': 'ok'}
            elif (jmsg['action'][0] == 'stop-scan'):
                gxsm.stopscan ()
                return {'result': 'ok'}
            elif (jmsg['action'][0] == 'autosave'):
                value = gxsm.autosave ()
                return {'result': 'ok', 'ret':value}
            elif (jmsg['action'][0] == 'autoupdate'):
                value = gxsm.autoupdate ()
                return {'result': 'ok', 'ret':value}
            else:
                return {'result': 'invalid action'}
        elif cmd == 'echo':
            return {'result': [{'echo': jmsg['echo'][0]['message']}]}
        else: return {'result': 'invalid request'}
            
## socket server 
    
# set sys.stdin non-blocking
def set_input_nonblocking():
    orig_fl = fcntl.fcntl (sys.stdin, fcntl.F_GETFL)
    fcntl.fcntl (sys.stdin, fcntl.F_SETFL, orig_fl | os.O_NONBLOCK)

def create_socket (server_addr, max_conn):
    sys.stdout.write ("*** GXSM Py Socket Server is now listening on "+HOST+":"+str(PORT)+"\n")
    server = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.setblocking (False)
    server.bind (server_addr)
    server.listen (max_conn)
    return server

def try_connect (server, server_addr):
    try:
        server.connect (server_addr)
        return True
    except:
        pass
    return False

def read_direct(conn, mask):
    global keep_alive
    try:
        client_address = conn.getpeername ()
        data = conn.recv (1024)
        #print ('Got {} from {}'.format(data, client_address))
        try:
            serialized = json.dumps(process_message (data))
        except (TypeError, ValueError):
            serialized = json.dumps({'JSON-Error'})
            raise Exception('You can only send JSON-serializable data')
        ret = '{}\n{}'.format(len(serialized), serialized)
        sys.stdout.write ('Return JSON for {} => {}\n'.format(data,ret))
        conn.sendall (ret.encode('utf-8'))
        if not data:
            keep_alive = True
    except:
        pass
      
def read(conn, mask):
    global keep_alive
    try:
        client_address = conn.getpeername ()
        jdata = receive_json(conn)
        if jdata:
            #print ('Got {} from {}'.format(jdata, client_address))
            ret=process_message (jdata)
            sys.stdout.write ('Return JSON for {} => {}\n'.format(jdata,ret))
            send_as_json(conn, ret)
        if not jdata:
            keep_alive = True
    except:
        pass
            
def send_as_json(conn, data):
    try:
        serialized = json.dumps(data)
    except (TypeError, ValueError):
        print ('JSON-SEND-Error')
        serialized = json.dumps({'JSON-SEND-Error'})
        raise Exception('You can only send JSON-serializable data')
    # send the length of the serialized data first
    sd = '{}\n{}'.format(len(serialized), serialized)
    sys.stdout.write ('Sending JSON: {}\n'.format(sd))
    conn.sendall (sd.encode('utf-8'))

def receive_json(conn):
    try:
        # try simple assume one message
        data = conn.recv (1024)
        if data:
            count, jsdata = data.split(b'\n')
            #print ('Got Data N={} D={}'.format(count,jsdata))
            try:
                deserialized = json.loads(jsdata)
            except (TypeError, ValueError):
                print('EE JSON-Deserialize-Error\n')
                deserialized = json.loads({'JSON-Deserialize-Error'})
                raise Exception('Data received was not in JSON format')
            return deserialized
    except:
        pass

def receive_json_long(conn):
    # read the length of the data, letter by letter until we reach EOL
    length_str = b''
    char = conn.recv(1)
    while char != '\n':
        length_str += char
        char = conn.recv(1)
    total = int(length_str)
    # use a memoryview to receive the data chunk by chunk efficiently
    view = memoryview(bytearray(total))
    next_offset = 0
    while total - next_offset > 0:
        recv_size = conn.recv_into(view[next_offset:], total - next_offset)
        next_offset += recv_size
        try:
            deserialized = json.loads(view.tobytes())
        except (TypeError, ValueError):
            raise Exception('Data received was not in JSON format')
    return deserialized
    
def accept(sock, mask):
    new_conn, addr = sock.accept ()
    new_conn.setblocking (False)
    print ('Accepting connection from {}'.format(addr))
    m_selector.register(new_conn, selectors.EVENT_READ, read)

def quit ():
    global keep_alive
    print ('Exiting...')
    keep_alive = False

def from_keyboard(arg1, arg2):
    line = arg1.read()
    if line == 'quit\n':
        quit()
    else:
        print('User input: {}'.format(line))

## MAIN ##

keep_alive = True

m_selector = selectors.DefaultSelector ()

set_input_nonblocking()
m_selector.register (sys.stdin, selectors.EVENT_READ, from_keyboard)

sys.stdout.write ("*** GXSM Py Socket Server is (re)starting...\n")
# listen to port 10000, at most 10 connections

server_addr = (HOST, PORT)
server = create_socket (server_addr, 10)
m_selector.register (server, selectors.EVENT_READ, accept)

while keep_alive:
    sc=gxsm.get ("script-control")
    if sc == 1:
        sys.stdout.write ('.')
        # sys.stdout.flush ()

    for key, mask in m_selector.select(0):
        callback = key.data
        callback (key.fileobj, mask)

    if sc > 3: # sc=4
        gxsm.sleep (0.01) # 1ms
    elif sc > 2: # sc=3
        gxsm.sleep (0.1) # 10ms
    else: # sc=2
        gxsm.sleep (1) # 100ms

    if sc == 0:
        sys.stdout.write ('\nScript Control is 0:  Closing server down now.\n')
        quit ()

sys.stdout.write ("*** GXSM Py Socket Server connection shutting down...\n")
m_selector.unregister (server)

# close connection
server.shutdown (socket.SHUT_RDWR)
server.close ()

# unregister events
m_selector.unregister (sys.stdin)

#  close select
m_selector.close ()

sys.stdout.write ("*** GXSM Py Socket Server Finished. ***\n")
