#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import sys
import time

# connect to gxsm4 process and start wrapper class
import gxsm4process as gxsm4

gxsm = gxsm4.gxsm_process(2)

def plot_life():
    f0 = 28524.000 # Hz

    f20 = 168000.0 # 181800.0
    spn = 100.0
    num = 600
    
    fig, ax = plt.subplots()
    x = np.linspace(f20-spn/2, f20+spn/2, num)
    am = np.zeros(num)
    ph = np.zeros(num)
    am_line, = ax.plot([], [], lw=2)
    ph_line, = ax.plot([], [], lw=2)

    ax.set_xlim(f20-spn/2, f20+spn/2)
    ax.set_ylim(-180, 1000.)
    ax.set_title("Heterodyne KPFM Tune&Expore")
    ax.set_xlabel("HTD: F2-F0 in Hz")
    ax.set_ylabel("Am,Ph")

    # 3. Initialization function: sets up the background of each frame
    def init():
        am_line.set_data(x, am)
        ph_line.set_data(x, ph)
        return am_line, ph_line

    # 4. Animation function: called sequentially to update data
    def update(frame):
        # Shift the sine wave slightly based on the current frame index
        #gxsm.read_status()
        #am[frame] = gxsm.rpspmc['htd_am']
        am[frame] = float(gxsm.get('rp-pacpll-HTD-AMPL-MONITOR'))
        ph[frame] = float(gxsm.get('rp-pacpll-HTD-PHASE-MONITOR'))
        print (frame, am[frame], ph[frame])
        gxsm.set('rp-pacpll-HTD-FREQ', f20-spn/2+spn*frame/num)
        am_line.set_data(x, am)
        ph_line.set_data(x, ph)
        return am_line, ph_line

    # 5. Create the animation object
    # frames=100 defines the length, interval=20 sets the speed (in milliseconds)
    # blit=True optimizes rendering by only redrawing parts that have changed
    ani = FuncAnimation(fig, update, init_func=init, frames=num, interval=20, blit=True)

    # 6. Display the animated plot window
    plt.show()



def fetch_vpdata_analysis(verbose=True, plot=False, start_idx=600, end_idx=1600):
    """
    Fetch VP-data from GXSM4 and return as numpy arrays for analysis.
    
    Parameters:
    -----------
    verbose : bool
        Print debug information
    plot : bool
        Show matplotlib plot (requires display)
    start_idx : int
        Starting index for data slicing
    end_idx : int
        Ending index for data slicing
    
    Returns:
    --------
    dict : Dictionary containing:
        - 'vpdata': dict of numpy arrays indexed by label
        - 'vpunits': dict of units indexed by label
        - 'xy': position [x, y, ix, iy]
        - 'labels': list of data labels
        - 'columns': full raw data array (all points)
    """
    
    try:
        gxsm = gxsm4.gxsm_process(verbose=0)
        
        # MAKE SURE TO HAVE A SCAN COMPLETED BEFORE AND OPEN AS MASTER DATA TARGET
        if verbose:
            print('*** Executing VP measurement on master scan...')
        gxsm.action('DSP_VP_VP_EXECUTE')
        
        # wait to complete
        time.sleep(4)
        
        ch = 0
        # fetch vpdata of last probe -- if exists, else error
        if verbose:
            print('*** Getting last vpdata set from master scan in ch=', ch)
        
        probe_result = gxsm.get_probe_event(ch, -1)
        if verbose:
            print('Probe result:', probe_result)
        
        if probe_result is None:
            print("ERROR: No probe event data available!")
            return None
            
        columns, labels, units, xy = probe_result
        
        # zip together for convenient data access (sliced to remove initial ramp points)
        vpdata = dict(zip(labels, columns[:, start_idx:end_idx]))
        vpunits = dict(zip(labels, units))
        
        # we got:
        if verbose:
            print('VP Data     @XY:', xy)
            print('VP Units[Sets] :', vpunits)
            print('Set Size       :', vpdata[labels[0]].shape)
            print('Available labels:', labels)
        
        # Package data for return
        result = {
            'vpdata': vpdata,
            'vpunits': vpunits,
            'xy': xy,
            'labels': labels,
            'columns': columns,  # Full raw data (not sliced)
            'start_idx': start_idx,
            'end_idx': end_idx
        }
        
        if plot:
            # Create VPDATA plot
            x = 'Time-Mon'
            y = 'Current'
            
            plt.figure(figsize=(6, 4))
            plt.title('VP-Data Plot')
            plt.plot(vpdata[x], vpdata[y], '-', alpha=0.8, label=y)
            plt.xlabel('{} in {}'.format(x, vpunits[x]))
            plt.ylabel('{} in {}'.format(y, vpunits[y]))
            plt.legend()
            plt.grid()
            plt.show()
            #plt.savefig('/tmp/vpdata-plot-demo.png')
        
        return result
        
    except FileNotFoundError as e:
        print(f"ERROR: Could not connect to GXSM4: {e}")
        print("Make sure GXSM4 is running and a scan is open as master data target.")
        return None
    except Exception as e:
        print(f"ERROR: {type(e).__name__}: {e}")
        return None


if __name__ == '__main__':

    plot_life()
    
    # Fetch and analyze data
    if 0:
        data = fetch_vpdata_analysis(verbose=True, plot=False)
    
        if data:
            print("\n" + "="*60)
            print("DATA AVAILABLE FOR ANALYSIS")
            print("="*60)

            # Example analysis - you can customize this
            vpdata = data['vpdata']
            vpunits = data['vpunits']
            labels = data['labels']

            print("\nQuick Statistics:")
            for label in labels:
                arr = vpdata[label]
                print(f"{label:15s}: min={arr.min():12.6f}, max={arr.max():12.6f}, "
                      f"mean={arr.mean():12.6f}, std={arr.std():12.6f} [{vpunits[label]}]")

            print("\n" + "="*60)
            print("Accessing data from Python:")
            print("  data['vpdata']['Current']  -> numpy array of current values")
            print("  data['vpunits']['Current'] -> units string")
            print("  data['xy']                 -> position [x, y, ix, iy]")
            print("  data['columns']            -> raw array (all points)")
            print("="*60)



