# GXSM3 - MK3DSPEmu package
This was a attempt a few years ago to create a "Linux Level" SRanger-DSP device emulation via DSP code running and simulating thread. It has only a very basic functionality:

  * shared memory area for simulating "DSP" data access working via read/write compatible to the device driver
  * base DSP code working, but still the performance is poor (even on 4GHz machines)
  * several mutual access problems to be solved and managed
  * independent process in user space
  * And more plans to simulate even most simple tunneling seams very tricky on software level.

## Compilation and installation
In addition to the lightly adapted DSP code... pre RTEngine version so... this is the "main" file to build:
<https://sourceforge.net/p/gxsm/svn/HEAD/tree/trunk/Gxsm-3.0/plug-ins/hard/MK3-EMU/dspemu_app.c>

To automatically configure, compile and install it, please add the folder hard/MK3-EMU to the file

<https://sourceforge.net/p/gxsm/svn/HEAD/tree/trunk/Gxsm-3.0/plug-ins/Makefile.am>

in the SUBDIR line and reconfigure the source code via

```bash
cd <your_gxsm3_source>
./autogen.sh
make -j
sudo make install
```

To modify the Makefile.am you can also apply the patch enable_mk3dspemu.diff, which you find in 

<https://sourceforge.net/p/gxsm/svn/HEAD/tree/trunk/Gxsm-3.0/debian/patches>

The patch is also applied during Ubuntu packaging. So you may try to install the package gxsm3-mk3dspemu from ppa:totto/gxsm (hosted on <launchpad.net>)

The trick is then to create a special shared memory mapped file 

```bash
shm_fd = shm_open (sr_emu_dev_file, O_CREAT | O_TRUNC | O_RDWR, 0666);
```

which represents the virtual "DSP's" memory just like the real DSP and simply native system read/write accesses (all what GXSM is actually using is seek, read/write -- no IOCTRL for normal operations) replaces the kernel usb-sranger module with no extra needs -- ideally so, that is the idea.

## Usage
So what you do, is fire up the EMU-App (mk3dspemu) and "Start" the virtual DSP via it. It is a native Linux process.
 
Then you have a new "special file" created representing the shared memory.

Then you put this path/file in Gxsm via preferences as DSP Device File.... however, I guess Gxsm will complain about version mismatch right now if you would try...

## Known issues
The DSP Emu then via some pointer hacks links C data structures directly into this memory region for communications. The crux lays in heavy heavy concurrencies and leads quickly to data corruption when partial data is read or written. Right now, there is no safe mutual access controls for this.

The real DSP is not that heavy or not at all multithreading, at least it's all under my control via the DSP kernel for atomic and non atomic read/writes and fully blocking any other executions. And on the kernel module side I do have a very well organized mutex and access queue in a first come first serve and complete manner. The mmap has nothing like that and access is wild as it comes.... 

Here I have not found a suitable and compatible way to emulate the DSP communication in this context. I tried to added stuff for mutual controls, but it's ugly and inefficient and still going wrong at the time I stopped working on this side project.

## Project status and history
**Totally experimental!** Do not use on a productive system.

### changes 13.02.2021
  * adding this README.md

## Authors and acknowledgment
This README summarizes some email correspondents between Percy Zahl and Thorsten Wagner.

Authors: Percy Zahl <zahl@users.sf.net> (mk3dspemu and related files), Thorsten Wagner <stm@users.sf.net> (this README)

DSP part: <http://sranger.sf.net>

Gxsm part: <http://gxsm.sf.net>

## License
Copyright (c) 2020 Percy Zahl

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
