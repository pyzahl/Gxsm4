unit  = [ "dB", "dB" ]



class Meter(gtk.DrawingArea):

    def __init__(self, parent):
      
        self.par = parent
        super(Meter, self).__init__()
 
        self.range = range(-120, 20, 20)

        # display mode
        self.vumeter = 1
        
        if self.vumeter:
            self.vumeterpixbuf = gtk.gdk.pixbuf_new_from_file_at_size("vu-frame-small.png", 220, 220)
            self.set_size_request(220, 220)
        else:
            self.set_size_request(50, -1)

        self.connect("expose-event", self.expose)


    def set_range (range_new):
        self.range = range_new


    def expose(self, widget, event):
        cr = widget.window.cairo_create()
        if self.vumeter:
            cr.set_source_pixbuf(self.vumeterpixbuf,0,0)
            cr.paint()
            cr.stroke()
            cr.set_line_width(0.8)

            cr.select_font_face("Courier", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            #cr.select_font_face("Ubuntu Mono", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(10)
            
#            vrange = (self.range[len(self.range)-1] - self.range[0])

            maxdb = 28
            mindb = -20*log10(1<<16)
            angle_scale = math.pi/2./(maxdb-mindb)   #15.7
            angle_offset = 0.7853975
            needle_len = 98.0
            needle_hub = 21.0

#            lp = lp * 0.8 + env * bias * 0.2  # low pass!
#            self.par.get_cur_value()
#            self.par.get_hi_value()
#            self.par.get_lo_value()


            theta = ((self.par.get_cur_value()-mindb) * angle_scale) - angle_offset

            if theta < -0.7853975:
                theta = -0.7853975
            if theta > 0.7853975:
                theta = 0.7853975

            x1 = 108 + round (needle_hub * math.sin(theta))
            y1 = 169 - round (needle_hub * math.cos(theta))
            x2 = 108 + round (needle_len * math.sin(theta))
            y2 = 169 - round (needle_len * math.cos(theta))

            cr.set_source_rgb(0.35, 0.31, 0.24)
            cr.move_to(x1, y1)
            cr.line_to(x2, y2)
            cr.stroke()

            theta = ((self.par.get_hi_value()-mindb) * angle_scale) - angle_offset
#            theta = math.pi/2*.7 - angle_offset

            if theta < -0.7853975:
                theta = -0.7853975
            if theta > 0.7853975:
                theta = 0.7853975

            x1 = 108 + round (needle_hub * math.sin(theta))
            y1 = 169 - round (needle_hub * math.cos(theta))
            x2 = 108 + round (needle_len * math.sin(theta))
            y2 = 169 - round (needle_len * math.cos(theta))

            cr.set_source_rgb(0.85, 0.01, 0.4)
            cr.move_to(x1, y1)
            cr.line_to(x2, y2)
            cr.stroke()

            cr.set_source_rgb(0.35, 0.31, 0.24)
            reading = "%+5.1f dB"%(self.par.get_cur_value())
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(60-width/2, 160)
            cr.text_path(reading)
            cr.stroke()

            cr.set_source_rgb(0.85, 0.01, 0.4)
            reading = "%+5.1f dB"%(self.par.get_hi_value())
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(220-60-width/2, 160)
            cr.text_path(reading)
            cr.stroke()


        else:
            cr.set_line_width(0.8)

            cr.select_font_face("Courier", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(8)

            (x, y, wminus, height, dx, dy) = cr.text_extents("--")
            pad   = 2*wminus
            width = self.allocation.width - 2*pad
            vrange = (self.range[len(self.range)-1] - self.range[0])
            fac = float(width)/vrange
            self.cur_pos = fac * self.par.get_cur_value()
            self.hi = fac * self.par.get_hi_value()
            self.lo = fac * self.par.get_lo_value()
            d = 1.+width/100.
            till = self.cur_pos
            full = width*1.0
            center = pad+width/2
            h=5

            if (self.hi != 0 and self.lo != 0 ):
                cr.set_source_rgb(0.0, 1.0, 0.00)
                cr.rectangle(center+self.lo, 0, self.hi - self.lo, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()

                cr.set_source_rgb(1.0, 0.2, 0.00)
                cr.rectangle(center+self.hi-d, 0, d, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()
            
                cr.set_source_rgb(1.0, 0.2, 0.00)
                cr.rectangle(center+self.lo+d, 0, -d, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()

            if (self.cur_pos >= full):
            
                cr.set_source_rgb(0.0, 1.0, 0.0)
                cr.rectangle(center, 0, full, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()
                
                cr.set_source_rgb(1.0, 0.0, 0.0) # red
                cr.rectangle(center+full, 0, till-full, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()

            else:
                if (self.cur_pos <= -full):
            
                    cr.set_source_rgb(1.0, 1.0, 0.72)
                    cr.rectangle(center, 0, -full, h)
                    cr.save()
                    cr.clip()
                    cr.paint()
                    cr.restore()
                    
                    cr.set_source_rgb(1.0, 0.68, 0.68)
                    cr.rectangle(center-full, 0, till+full, h)
                    cr.save()
                    cr.clip()
                    cr.paint()
                    cr.restore()
                    
                else:     
                    cr.set_source_rgb(0.0, 0.0, 1.0) # blue
                    cr.rectangle(center, 0, till, h)
                    cr.save()
                    cr.clip()
                    cr.paint()
                    cr.restore()
                    
            

                cr.set_source_rgb(0.35, 0.31, 0.24)
                for v in self.range:
                    xp = fac * (v - self.range[0])
                    cr.move_to(pad+xp, 0)
                    cr.line_to(pad+xp, 5)
                    cr.stroke()
                    lab = "%d"%v
                    (x, y, width, height, dx, dy) = cr.text_extents(lab)
                    if (v < 0):
                        width = width+wminus
                        cr.move_to(pad+xp-width/2, 15)
                        cr.text_path(lab)
                        cr.stroke()

class Instrument(gtk.Label):
        def __init__(self, parent, vb):
		
                #Initialize the Widget
                gtk.Widget.__init__(self)
		self.set_use_markup(True)
		self.cur_value = 0
		self.cur_value_lo = 0
		self.cur_value_hi = 0
		self.unit = "dB"
		self.format = "%8.1f " + self.unit
		self.meter = Meter(self)
		vb.pack_start(self, False, False, 0)
		vb.pack_start(self.meter, False, False, 0)
		vb.show_all()
		self.show_all()
		
	def set_format (self, str):
		self.format = str + " " + self.unit

	def set_unit (self, str):
		self.unit = str

	def set_reading (self, value):
		spantag  = "<span size=\"24000\" font_family=\"monospace\"><b>"   # .. color=\"#ff0000\"
		self.set_markup (spantag + self.format %value + "</b></span>")
		self.cur_value = value	
		self.meter.queue_draw()

	def set_reading_lohi (self, value, lo, hi):
		spantag  = "<span size=\"24000\" font_family=\"monospace\"><b>"   # .. color=\"#ff0000\"
		spantag2 = "<span size=\"10000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
		self.set_markup (spantag + self.format %value + "</b></span>"
				 + "\n" + spantag2
				 + self.format %lo
				 + "  "
				 + self.format %hi
				 + "</span>"
				 )
		self.cur_value = value	
		self.cur_value_lo = lo
		self.cur_value_hi = hi
		self.meter.queue_draw()

        def set_reading_vu (self, value, hi):
		self.cur_value = value	
		self.cur_value_lo = value
		self.cur_value_hi = hi
		self.meter.queue_draw()
        
	def get_cur_value(self):
		return self.cur_value
	def get_lo_value(self):
		return self.cur_value_lo
	def get_hi_value(self):
		return self.cur_value_hi
    

def xx():
    v = gobject.new(gtk.VBox(spacing=2))
    c1 = Instrument( gobject.new(gtk.Label), v)
    c1.show()
    table.attach(v, 1, 2, tr, tr+1)


    def update_meter(vu1set, vu2set, data_address=data_address, count=[0]):
        # get current Gain parameters
        sr = open (sr_dev_path, "rw")
        sr.seek (data_address, 1)
        fmt = ">hhhh"
        data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        count[0] = count[0] + 1
        db = 2*array(data)+1
        for i in range(0,4):
            db[i] = -96.33+20.*math.log10(db[i])
            vu1set (db[0], db[1])
            vu2set (db[2], db[3])
            return gtk.TRUE

    gobject.timeout_add (88, update_meter, c1.set_reading_vu, c2.set_reading_vu)
