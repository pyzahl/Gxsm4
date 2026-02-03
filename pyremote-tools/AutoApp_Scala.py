#!/usr/bin/env python3

import tkinter as tk
import serial
import serial.tools.list_ports
import threading
import time
import queue

# use SHM bridge to Gxsm4
import gxsm4process as gxsm4
gxsm = gxsm4.gxsm_process()


def find_teensy():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        print (p)
        return p.device ### BLIND TESTING
        if "Teensy" in p.description or "16C0" in p.hwid or "0483" in p.hwid:
            print(f"Found Teensy on {p.device}")
            return p.device
    raise IOError("No Teensy device found!")

SERIAL_PORT = find_teensy()
BAUD_RATE = 9600

class SignalController:
    def __init__(self, master):
        self.master = master
        self.master.title("Teensy Signalsteuerung")
        self._serial_lock = threading.Lock()
        
        self.serial = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.5)
        self.serial.dtr = True
        self.serial.rts = False
        time.sleep(0.5)
        self.serial.reset_input_buffer()
        self.serial.reset_output_buffer()
        self.serial.write(b"WAKEUP\n")

        self.signals = {
            "ZMAX": 0,
            "IZERO_inv": False,
            "U_RANGE": False,
            "I_RANGE": False
        }
        
        self.zmax = 0
        self.old_zmax = 0
        
        self.indicators = {}
        row = 0
        for signal in ["ZMAX", "IZERO_inv"]:
            tk.Label(master, text=signal).grid(row=row, column=0, padx=10, pady=5)
            canvas = tk.Canvas(master, width=20, height=20)
            canvas.grid(row=row, column=1)
            self.indicators[signal] = canvas
            row += 1

        tk.Label(master, text="I:").grid(row=row, column=0, sticky="e", padx=10)
        self.label_I = tk.Label(master, text="---")
        self.label_I.grid(row=row, column=1, sticky="w")
        row += 1

        tk.Label(master, text="I_Setpoint:").grid(row=row, column=0, sticky="e", padx=10)
        self.label_I_setpoint = tk.Label(master, text="---")
        self.label_I_setpoint.grid(row=row, column=1, sticky="w")
        row += 1

        tk.Label(master, text="VZ:").grid(row=row, column=0, sticky="e", padx=10)
        self.label_VZ = tk.Label(master, text="---")
        self.label_VZ.grid(row=row, column=1, sticky="w")
        row += 1
        
        self.U_RANGE_button = tk.Button(master, text="U_RANGE:LOW", width=12, command=self.toggle_U_RANGE)
        self.U_RANGE_button.grid(row=row, column=0, columnspan=2, pady=5)
        row += 1
        
        self.I_RANGE_button = tk.Button(master, text="I_RANGE:LOW", width=12, command=self.toggle_I_RANGE)
        self.I_RANGE_button.grid(row=row, column=0, columnspan=2, pady=5)
        row += 1


        self.I_setpoint = gxsm.get("dsp-fbs-mx0-current-set")

        #### DO THIS BEFORE STARTING THREADs!! Moved HERE!
        gxsm.set('dsp-adv-dsp-zpos-ref',6750) ## gxsm4process also takes a value now!
        gxsm.set('dsp-fbs-mx0-current-level',self.I_setpoint)


        #self._I = abs(gxsm.rtquery("f")[1])   ## fix to new scheme
        gxsm.rt_query_rpspmc() ## update RT readings buffer "gxsm.rpspmc" from SHM:

#                # RPSPMC XYZ Monitors via GXSM4 
#                self.XYZ_monitor = {
#                        'monitor': [0.0,0.0,0.0], # Volts
#                        'monitor_min': [0.0,0.0,0.0],
#                        'monitor_max': [0.0,0.0,0.0],
#                }
#                self.rpspmc = {
#                        'bias':    0.0,    # Volts
#                        'current': 0.0, # nA
#                        'gvp' :    { 'x':0.0, 'y':0.0, 'z': 0.0, 'u': 0.0, 'a': 0.0, 'b': 0.0, 'am':0.0, 'fm':0.0 },
#                        'pac' :    { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 },
#                        'zservo':  { 'mode': 0.0, 'setpoint': 0.0, 'cp': 0.0, 'ci': 0.0, 'cp_db': 0.0, 'ci_db': 0.0, 'upper': 0.0, 'lower': 0.0, 'setpoint_cz': 0.0, 'level': 0.0, 'in_offcomp': 0.0, 'src_mux': 0.0 }, # RPSPMC Units (i.e. Volt)
#                        'scan':    { 'alpha': 0.0, 'slope' : { 'dzx': 0.0, 'dzy': 0.0, 'slew': 0.0, }, 'x': 0.0, 'y': 0.0, 'slew': 0.0, 'opts': 0.0, 'offset' : { 'x': 0.0, 'y': 0.0, 'z': 0.0 }}, # RPSPMC Units (i.e. Volt)
#                        'time' :   0.0  # exact FPGA uptime. This is the FPGA time of last reading in seconds, 8ns resolution
#                }
        
        print ('Setpoint: ', gxsm.rpspmc['zservo']['setpoint']) ## in I Setpoint in Volt
        print ('Setpoint: ', gxsm.rpspmc['current']) ## in nA
        self._I = abs(gxsm.rpspmc['current'])
        
        ### May sleep a moment (20ms)
        time.sleep(0.1) ## to make sure it's done
        
        self.running = True
        self.receiver_thread = threading.Thread(target=self.read_serial_loop, daemon=True)  ### thread uses gxsm.get
        self.receiver_thread.start()
        self.updater_thread = threading.Thread(target=self.gxsm_updater, daemon=True)   ### thread uses gxsm.get/set
        self.updater_thread.start()
        self.write_queue = queue.Queue()
        self.writer_thread = threading.Thread(target=self.serial_writer_loop, daemon=True)
        self.writer_thread.start()
        
        self.write_queue.put("REQUEST:U_RANGE\n")
        self.write_queue.put("REQUEST:I_RANGE\n")
        self.write_queue.put("REQUEST:STATUS\n")
        


        #### DO THIS BEFORE STARTING THREADs!! Easy!
        #gxsm.set('dsp-adv-dsp-zpos-ref','6750')
        #gxsm.set('dsp-fbs-mx0-current-level','{}'.format(self.I_setpoint))
        #self._I = abs(gxsm.rtquery("f")[1])
        
        if self._I >= 0.66*self.I_setpoint:
            self.signals["IZERO_inv"] = True
            self.write_queue.put("IZERO_inv:ON\n")
        elif self._I < 0.15*self.I_setpoint:
            self.signals["IZERO_inv"] = False
            self.write_queue.put("IZERO_inv:OFF\n")

        
        self.master.after(10, self.periodic_update)


    def serial_writer_loop(self):
        while self.running:
            try:
                msg = self.write_queue.get(timeout=0.001)
            except queue.Empty:
                continue
            with self._serial_lock:
                try:
                    if "\n" in msg:
                        self.serial.write(msg.encode())
                        #print("OUT:", msg)
                except Exception as e:
                    print("Serial write error:", e)

    def read_serial_loop(self):
        buf = b""
        while self.running:
            try:
                data = self.serial.read(64)
                if not data:
                    continue
                buf += data
                while b'\n' in buf:
                    line, buf = buf.split(b'\n', 1)
                    line = line.decode(errors="ignore").strip()
                    if line:
                        self.process_serial_line(line)
            except Exception as e:
                print("Serial read error:", e)
                time.sleep(0.05)
                    
    def process_serial_line(self, line: str):
        #print("IN:", repr(line))
        if ":" in line:
            key, value = line.split(":", 1)
            key = key.strip().upper()
            value = value.strip().upper()
            if key in self.signals:
                self.signals[key] = (value == "ON")
                
        elif line.startswith("EXTEND"):
            if self.running:
                if 0:
                    if gxsm.get('dsp-fbs-ci') > (-83):
                        print("Warning: CI value is greater than -83 dB. Auto Approach might be dangerously fast.")
                    if gxsm.get('dsp-fbs-ci') < (-84):
                        print("Warning: CI value is lower than -83 dB. Auto Approach might be very slow.")
                else: # better, only one get: -- not sure, is this a one per cycle action or always processing while extending, make it once/cycle!
                    ci=gxsm.get('dsp-fbs-ci')
                    ### anyways what's that for ?!?!?
                    if ci > -83:
                        print("Warning: CI value is greater than -83 dB. Auto Approach might be dangerously fast.")
                    if ci < -84:
                        print("Warning: CI value is lower than -83 dB. Auto Approach might be very slow.")
            self.extend()
    
        elif line.startswith("RETRACT"):
            if self.running:
                self.retract()
    
        elif line.startswith("ZMAX"):
            if "p" in line:
                self.old_zmax = 1
            elif "0" in line:
                self.old_zmax = 0
            elif "m" in line:
                self.old_zmax = -1

    def periodic_update(self):
        self.label_I.config(text=f"{self._I:.1e} nA")
        self.label_VZ.config(text=f"{self._VZ:.1f} V")
        self.label_I_setpoint.config(text=f"{self.I_setpoint:.1e} nA")
        for sig in self.indicators:
            c = self.indicators[sig]
            c.delete("all")
            if sig == "ZMAX":
                if self.old_zmax == 1: color = "green"
                elif self.old_zmax == -1: color = "red"
                else: color = "yellow"
            else:
                color = "green" if self.signals.get(sig) else "red"
            c.create_oval(2, 2, 18, 18, fill=color)
        self.U_RANGE_button.config(text=f"U_RANGE:{'HIGH' if self.signals['U_RANGE'] else 'LOW'}")
        self.I_RANGE_button.config(text=f"I_RANGE:{'HIGH' if self.signals['I_RANGE'] else 'LOW'}")
        
            
            
        
        self.master.after(1, self.periodic_update)

    def gxsm_updater(self):
        while self.running:
            try:
                self._I = abs(gxsm.rtquery("f")[1])
                self.I_setpoint = gxsm.get("dsp-fbs-mx0-current-set") #### THIS IS A THREADDING CONFLICT!!! WITH ABOVE. BUT WITH MY LATEST THEAD SAFE PATCH/MUTEX THIS SHOULD BE OK NOW!
                self._VZ = gxsm.rtquery("z")[0]

                zmax_state = 1 if self._VZ <= -68.0 else -1 if self._VZ >= 68.0 else 0
                if zmax_state != self.old_zmax:
                    self.write_queue.put(f"ZMAX:{zmax_state}\n")

                # IZERO toggle logic
                if self._I >= 0.66*self.I_setpoint and not self.signals["IZERO_inv"]:
                    self.signals["IZERO_inv"] = True
                    self.write_queue.put("IZERO_inv:ON\n")
                    if self._VZ >-1:
                        print("Tip is extended only halfway or less. Retract, manually coarse retract and Extend again for safer scanning.")
            
                    if self._VZ < -40:
                        print("Tip is extended rather far. Retract, manually coarse approach and Extend again for safer scanning.")
                elif self._I < 0.15*self.I_setpoint and self.signals["IZERO_inv"]:
                    self.signals["IZERO_inv"] = False
                    self.write_queue.put("IZERO_inv:OFF\n")

            except Exception as e:
                print("GXSM update error:", e)
            time.sleep(0.001)

    def close(self):
        self.running = False
        try:
            self.master.after_cancel(self.periodic_update)
        except Exception:
            pass
        if self.receiver_thread.is_alive():
            self.receiver_thread.join(timeout=1)
        if self.updater_thread.is_alive():
            self.updater_thread.join(timeout=1)
        if self.writer_thread.is_alive():
            self.writer_thread.join(timeout=1)
        if hasattr(self, 'extend_thread') and self.extend_thread.is_alive():
            self.extend_thread.join(timeout=1)
        if hasattr(self, 'retract_thread') and self.retract_thread.is_alive():
            self.retract_thread.join(timeout=1)
        with self._serial_lock:
            try:
                self.serial.flush()
                self.serial.reset_input_buffer()
                self.serial.reset_output_buffer()
                #self.serial.close()
            except Exception as e:
                print("Serial close error:", e)
        time.sleep(0.2)
        self.master.destroy()
        
    def toggle_U_RANGE(self):
        self.signals["U_RANGE"] = not self.signals["U_RANGE"]
        state = "HIGH" if self.signals["U_RANGE"] else "LOW"
        self.write_queue.put(f"U_RANGE:{state}\n")
        self.write_queue.put("REQUEST:URANGE\n")

    def toggle_I_RANGE(self):
        self.signals["I_RANGE"] = not self.signals["I_RANGE"]
        state = "HIGH" if self.signals["I_RANGE"] else "LOW"
        self.write_queue.put(f"I_RANGE:{state}\n")
        self.write_queue.put("REQUEST:IRANGE\n")
        
    def retract(self):
        gxsm.set('dsp-fbs-mx0-current-level','{}'.format(self.I_setpoint))

    def extend(self):
        gxsm.set('dsp-fbs-mx0-current-level','0')



if __name__ == "__main__":
    root = tk.Tk()
    app = SignalController(root)
    root.protocol("WM_DELETE_WINDOW", app.close)
    root.mainloop()
