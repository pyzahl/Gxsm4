#!./PVInflux/bin/python3

## #!/usr/bin/python3

### sudo ip route add 192.168.40.0/24 dev enx74fecea8c2c3

# #!/usr/bin/env python3
## Createc CryoBox to influxdb
import sys
import threading
#import serial
import io, time
import math
import socket, os, json

import urllib.request, urllib.parse

from pathlib import Path

import pgi
pgi.require_version('Gtk', '3.0')
from pgi.repository import Gtk
#import gi
#gi.require_version('Gtk', '4.0')
#from gi.repository import Gtk, Gdk, GLib

import numpy
from datetime import datetime
from influxdb_client import Point, InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

import urllib3
urllib3.disable_warnings()


token = "8AsR73p1SDY5Io2QWHcX527i3Usn-3DmW9nd901VCNax0oKm5PpL5JTCcIK2eC519jHggi80kHcs8KdHHAlX4Q=="
#WIN#token = "vWAYEnQDC3-qc5L0wQN8X4ZC4dFK7f1Tdc2IJ38Uw2Xj2EdPzqnIiKAQrQVZhmRwAfM8Plx7TuRxdKMD9pT8Nw=="
org = "CryoControl"
url = "http://localhost:8086"
bucket = "CryoControl"

token_monitoring_cryobox_only="s-Uehs6hsiBzfPpJ3z2-WlHylHu6qyDeJmU3j_YANnhujE0zSmqhCZ_TzUz3RNJTHqcGO6e39PGRjJDQP_Wwvg=="


class CryoControlApp(Gtk.Application):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, application_id="com.ctreatec.cryocontrol",
                         **kwargs)
        self.window = None

    def on_destroy(self, window):
        self.cbox.quit ()
        sys.exit()
        
    def do_activate(self):
        if not self.window:
            self.window = Gtk.ApplicationWindow(application=self, title="Cryo Control Box Influxdb-Log")
        self.connect('destroy', self.on_destroy)
        self.window.present()

        hb = Gtk.HeaderBar() 
        #hb.set_show_close_button(True) 
        #hb.props.title = "xx"
        self.window.set_titlebar(hb) 
        #hb.set_subtitle("Createc Piezo Drive") 
        
        self.grid = Gtk.Grid()
        self.window.set_child (grid)

        print ('Starting Logging Thread, then GTK Main LOOP')
        self.cbox = CryoBox()
        self.cbbox.start_thread(10)
        
        self.indicator = Gtk.Label(label="Status");
        self.grid.attach(self.indicator, 1,0,1,1)

        self.stm = Gtk.Label(label="STM");
        self.grid.attach(self.stm, 1,1,1,1)
        self.cryo = Gtk.Label(label="Cryo");
        self.grid.attach(self.cryo, 2,1,1,1)
        self.lhe = Gtk.Label(label="LHe");
        self.grid.attach(self.lhe, 1,2,1,1)

        self.button = Gtk.Button(label="Start Fill")
        self.button.connect("clicked", self.on_fill_clicked)
        self.grid.attach(self.button, 1,10,1,1)

        self.button = Gtk.Button(label="Stop Fill")
        self.button.connect("clicked", self.on_stop_clicked)
        self.grid.attach(self.button, 1,11,1,1)

        self.button = Gtk.Button(label="Manual Mode")
        self.button.connect("clicked", self.on_manual_clicked)
        self.grid.attach(self.button, 1,12,1,1)

        self.button = Gtk.Button(label="Auto Mode")
        self.button.connect("clicked", self.on_auto_clicked)
        self.grid.attach(self.button, 1,13,1,1)

        self.button = Gtk.Button(label="Read")
        self.button.connect("clicked", self.on_read_clicked)
        self.grid.attach(self.button, 1,14,1,1)

        self.button = Gtk.CheckButton(label="Fast Reading")
        self.button.connect("clicked", self.on_fast_clicked)
        self.grid.attach(self.button, 1,15,1,1)

        self.button = Gtk.Button(label="Exit")
        self.button.connect("clicked", self.on_exit_clicked)
        self.grid.attach(self.button, 1,20,1,1)


        print ('GUI done')
        #self.source_id = GLib.timeout_add(10000, self.GUIupdate, self)

    def on_fast_clicked(self, widget):
        
        self.GUIupdate ()
        

    def on_fill_clicked(self, widget):
        self.GUIupdate ()
        
    def on_stop_clicked(self, widget):
        self.GUIupdate ()

    def on_manual_clicked(self, widget):
        self.GUIupdate ()

    def on_auto_clicked(self, widget):
        self.GUIupdate ()

    def on_read_clicked(self, widget):
        self.GUIupdate ()

    def on_exit_clicked(self, widget):
        self.cbox.quit();
        sys.exit()

        
    def GUIupdate(self):
        print ('udpate GUI')
        if False:
            self.cbox.read_cryobox()
            v,u = self.cbox.query('4Wire Sensor 1 Temperature')
            if u != 'X':
                self.stm.set_markup('<b></span>STM: {:.3} {} </span></b>'.format (float(v), u))
                v,u = self.cbox.query('4Wire Sensor 2 Temperature')
                self.cryo.set_markup('<b></span>Cryo: {:.3} {} </span></b>'.format (float(v), u))
                v,u = self.cbox.query('LHe Fill Height')
                self.lhe.set_markup('<b></span>LHe: {:.3} {} </span></b>'.format (float(v), u))
        return True

    

        

class CryoBox():
    def __init__(self):
        self.write_client = InfluxDBClient(url = url, token = token, org = org, ssl=True, verify_ssl=False)
        self.write_api = self.write_client.write_api(write_options=SYNCHRONOUS)

        # This stores all values into InfluxDB

        # Microcontroller is clocked at 600 MHz
        self.microcontroller_clock = 600E+6

        self.ip_address = ("192.168.40.30", 1234)
        if len(sys.argv) == 2: # If an address is given, override default address
                self.ip_address = sys.argv[1]

        self.error_codes = ["Overvoltage", "Overcurrent", "Overpower", "Energy exceeded", "Timeout", "Resistance exceeded", "Current mismatch", "Voltage ratio exceeded"]

        print ('Enabling LHe reading, normal mode.')
        self.enable_LHe()
        time.sleep(3)
        
        print ('Entering Forever Read Loop. CTRL-C to quit')
        # Last values of the received data
        self.last_data_values = {}
        self.update_last=time.monotonic_ns()*1e-9-10

        self.CryoData = {'CryoBoxPy': 'by Py V0.01'}
        
    def start_logging_forever(self, t=60):
        self.sleepy_time=t
        while True:
            self.add_point ()
            #self.update ()
            print ('Zzzzzzzzzzz ', t, 's')
            time.sleep(self.sleepy_time)


    def start_thread(self, t=60):
        self.sleepy_time=t
        self.run_flag=True
        self.thread = threading.Thread(target = self.data_update)
        self.thread.start()
        print ('Data Read Thread is running')

    def query(self, key):
        try:
            print (key, self.CryoData[key])
            print (self.CryoData[key]['Value'], self.CryoData[key]['Unit'])
            return self.CryoData[key]['Value'], self.CryoData[key]['Unit']
        except:
            return '0','X'
        
    def quit(self):
        self.run_flag=False
        self.thread.join()

        
    def data_update(self):
        print ('Data Read Thread')
        while self.run_flag:
                #self.update ()
                self.add_point ()
                print ('Zzzzzzzzzzz 60s')
                if self.sleepy_time > 2:
                    i=int(self.sleepy_time)
                    while i and self.run_flag:
                        time.sleep(1)
                        i=i-1
                else:
                    time.sleep(self.sleepy_time)

    def read_cryobox(self):
        json_data = {}
        now = time.monotonic_ns()*1e-9
        print (now - self.update_last)
        if (now - self.update_last) < 0.5:
            return

        try:
                # Set up a TCP socket but don't connect it yet
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

                # 2 second timeout
                s.settimeout(2.0)

                # Connect to TCP port 1234 on the device
                s.connect(self.ip_address)

                print(f"InfluxDB at {url} with org {org} and bucket {bucket}, exit with Ctrl + C")

                # Do this on repeat
                if True:
                    # Initialize an empty byte string
                    data = b""

                    # Receive data until the null terminator is found
                    while len(data) == 0 or data[-1] != b"\0"[0]:
                        try:
                            data += s.recv(1024)

                        # For when the next data point takes longer than the configured timeout
                        except TimeoutError:
                            # Do nothing
                            pass

                    # Convert data from byte string to UTF-8 string, strip null terminator and newlines
                    decoded_data = data.decode("utf-8").rstrip("\x00\n")

                    try:
                        # Split the received line into data name and the JSON data
                        name, json_raw_data = decoded_data.strip().split(": ", maxsplit = 1) # Remove excessive newlines and split into name and data

                        if name == "Data Update":
                            # Decode JSON string to dictionary
                            json_data = json.loads(json_raw_data)
                            # print (json_data)
                            # Iterate over the keys of the dictionary
                            for key in json_data:
                                # The value corresponding to the key in the dictionary
                                value = json_data[key]

                        elif name == "Command Result":
                            json_data = json.loads(json_raw_data)
                            if json_data["return"]:
                                point = point.field("Commands", json_data["command"])
                                self.write_api.write(bucket = bucket, org = org, record = point)
                                print ('Command Result: ', json_data["command"])

                        else:
                            print(decoded_data)

                    except json.decoder.JSONDecodeError as error:
                        print(f"JSON decode error: {error}\nat data: {json_raw_data} {ord(json_raw_data[-1])}")

                        
            # For example when the device is power cycled, LAN cable plugged out or so
        except (ConnectionResetError, ConnectionAbortedError, TimeoutError, OSError): # TODO: OSError? "[WinError 10065] Der Host war bei einem Socketvorgang nicht erreichbar"
                print("Read Data: Connection reset, retrying in 10 seconds")

        self.update_last = now

        s.close()
        return json_data

    ## add dataset points to influxdb
    def add_point(self):
        json_data = self.read_cryobox()

        for key in json_data:
            # The value corresponding to the key in the dictionary
            point = Point("CryoControl").tag("Address", self.ip_address[0])
            value = json_data[key]
            if type(value) is dict and "Unit" in value: # To exclude IP address etc
                if "Unit" in value:
                    point = point.tag("Unit", value["Unit"])

                if value["Type"] == "int":
                    point = point.field(key, int(value["Value"]))

                else:
                    if value["Value"].startswith("ERR:"):
                        parts = value["Value"].split("Err: ")
                        if len(parts) == 2:
                            point = point.field(key + " Error", error_codes[int(parts[1])]) # Convert to error code

                        else:
                            with open("faultydata.txt", "a") as efile:
                                efile.write(decoded_data + "\n")

                    else:
                        point = point.field(key, float(value["Value"]))

                self.write_api.write(bucket = bucket, org = org, record = point)
                self.CryoData.update ({key: value})
                print("Logged", key, value)

            elif key == "Time" or key == "CPU_Time":
                point = point.field(key, value)
                self.write_api.write(bucket = bucket, org = org, record = point)
                print("Logged", key, value)
                self.CryoData.update ({key: value})

    # get a reading to dictionary only
    def get_reading(self):
        json_data = self.read_cryobox()

        for key in json_data:
            # The value corresponding to the key in the dictionary
            value = json_data[key]
            if type(value) is dict and "Unit" in value: # To exclude IP address etc
                print("Reading", key, value)
                self.CryoData.update ({key: value})

            elif key == "Time" or key == "CPU_Time":
                print("Reading", key, value)
                self.CryoData.update ({key: value})
            


    def send_command(self, commands):
        try:
            #s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #s.connect(self.ip_address)
            #s.settimeout(2.0)

            for command in commands:
                print ('Sending: >{}<'.format(command))
                #s.send(command.encode() + b"\0")
                #time.sleep(1)
                urllib.request.urlopen("http://"+self.ip_address[0]+"/PostData?Command=" + urllib.parse.quote(command))
            #s.close()
        except (ConnectionResetError, ConnectionAbortedError, TimeoutError, OSError): # TODO: OSError? "[WinError 10065] Der Host war bei einem Socketvorgang nicht erreichbar"
            #s.close()
            print("Send Command: Connection reset, retrying in 10 seconds")
            return True


    def enable_LHe(self, enable='True', fast_mode='False'):
        self.send_command(('LHe:Error 0', 'LHe:Enabled {}'.format(enable), 'LHe:Fast_Mode {}'.format(fast_mode)))

## START ##

print ("Starting CryoBox Control/Influxdb Logger")


print (sys.argv)

if True:  ## PLAIN MODE WITHOUT GUI
    cb  = CryoBox()
    print ('Starting simple -- quit with CTRL-C')

    fast_mode=0
    if fast_mode:
        print ('Fast Mode')
        cb.enable_LHe(enable='On', fast_mode='On')
        cb.start_logging_forever(1)
    else:
        print ('Normal Mode')
        cb.start_logging_forever()
    
else:
    if __name__ == "__main__":
        app = CryoControlApp()
        app.run(sys.argv)


    










######################################### obsoleted




# All in one
def XXXupdate(self):
        now = time.monotonic_ns()*1e-9
        print (now - self.update_last)
        if (now - self.update_last) < 0.5:
            return

        try:
                # Set up a TCP socket but don't connect it yet
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

                # 2 second timeout
                s.settimeout(2.0)

                # Connect to TCP port 1234 on the device
                s.connect(self.ip_address)

                print(f"InfluxDB at {url} with org {org} and bucket {bucket}, exit with Ctrl + C")

                # Do this on repeat
                if True:
                    # Initialize an empty byte string
                    data = b""

                    # Receive data until the null terminator is found
                    while len(data) == 0 or data[-1] != b"\0"[0]:
                        try:
                            data += s.recv(1024)

                        # For when the next data point takes longer than the configured timeout
                        except TimeoutError:
                            # Do nothing
                            pass

                    # Convert data from byte string to UTF-8 string, strip null terminator and newlines
                    decoded_data = data.decode("utf-8").rstrip("\x00\n")

                    try:
                        # Split the received line into data name and the JSON data
                        name, json_raw_data = decoded_data.strip().split(": ", maxsplit = 1) # Remove excessive newlines and split into name and data

                        if name == "Data Update":
                            # Decode JSON string to dictionary
                            json_data = json.loads(json_raw_data)
                            # print (json_data)
                            # Iterate over the keys of the dictionary
                            for key in json_data:
                                # The value corresponding to the key in the dictionary
                                value = json_data[key]
                                point = Point("CryoControl").tag("Address", self.ip_address[0])

                                if type(value) is dict and "Unit" in value: # To exclude IP address etc
                                    if "Unit" in value:
                                        point = point.tag("Unit", value["Unit"])

                                    if value["Type"] == "int":
                                        point = point.field(key, int(value["Value"]))

                                    else:
                                        if value["Value"].startswith("ERR:"):
                                            parts = value["Value"].split("Err: ")
                                            if len(parts) == 2:
                                                point = point.field(key + " Error", error_codes[int(parts[1])]) # Convert to error code

                                            else:
                                                with open("faultydata.txt", "a") as efile:
                                                    efile.write(decoded_data + "\n")

                                        else:
                                            point = point.field(key, float(value["Value"]))

                                    self.write_api.write(bucket = bucket, org = org, record = point)
                                    print("Logged", key, value)
                                    self.CryoData.update ({key: value})

                                elif key == "Time" or key == "CPU_Time":
                                    point = point.field(key, value)
                                    self.write_api.write(bucket = bucket, org = org, record = point)
                                    print("Logged", key, value)
                                    self.CryoData.update ({key: value})

                        elif name == "Command Result":
                            json_data = json.loads(json_raw_data)
                            if json_data["return"]:
                                point = point.field("Commands", json_data["command"])
                                self.write_api.write(bucket = bucket, org = org, record = point)

                        else:
                            print(decoded_data)

                        self.update_last = now

                    except json.decoder.JSONDecodeError as error:
                        print(f"JSON decode error: {error}\nat data: {json_raw_data} {ord(json_raw_data[-1])}")

            # For example when the device is power cycled, LAN cable plugged out or so
        except (ConnectionResetError, ConnectionAbortedError, TimeoutError, OSError): # TODO: OSError? "[WinError 10065] Der Host war bei einem Socketvorgang nicht erreichbar"
                s.close()
                print("Read Data: Connection reset, retrying in 10 seconds")
                return True

        s.close()

        return True

        
