/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_stream.cpp
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#include <gtk/gtk.h>
#include <gio/gio.h>
#include <zlib.h>

#include "config.h"

/* *** RP_stream module *** */
#include "rpspmc_stream.h"

#include "rpspmc_hwi_dev.h"
extern rpspmc_hwi_dev *rpspmc_hwi;

// ONLY USE OF HwI global rpspmc here TO ADD STATUS INFO
void RP_stream::status_append (const gchar *msg, bool schedule_from_thread=false){
        if (send_msg_func)
                send_msg_func (msg, schedule_from_thread);
}
// ***

gchar* get_ip_from_hostname(const gchar *host){
        struct addrinfo* result;
        struct addrinfo* res;
        int error;
        struct addrinfo  hints;
        gchar *g_hostip = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; //AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_DGRAM; /* Datagram socket -- limit to sockets */
        hints.ai_flags = 0;
        hints.ai_protocol = 0;          /* Any protocol */

        // hints may be = NULL for all
    
        /* resolve the domain name into a list of addresses */
        error = getaddrinfo(host, NULL, &hints, &result);
        if (error != 0) {   
                if (error == EAI_SYSTEM) {
                        perror("getaddrinfo");
                } else {
                        g_error ("error in getaddrinfo: %s\n", gai_strerror(error));
                }   
                return NULL;
        }

        /* loop over all returned results and do inverse lookup */
        for (res = result; res != NULL; res = res->ai_next) {   
                char hostip[NI_MAXHOST];
                error = getnameinfo(res->ai_addr, res->ai_addrlen, hostip, NI_MAXHOST, NULL, 0, 0); 
                if (error != 0) {
                        g_error ("error in getnameinfo: %s\n", gai_strerror(error));
                        continue;
                }
                if (*hostip != '\0'){
                        g_message ("%s => %s\n", host, hostip);
                        g_hostip = g_strdup (hostip);
                        break; // take first result!
                }
        }
        freeaddrinfo(result);
        return g_hostip; // give away, must g_free
}

gpointer RP_stream::wspp_asio_thread (void *ptr_rp_stream){
        RP_stream* rps = (RP_stream*)ptr_rp_stream;
        try {

                // then connect to Stream Socket on RP
                gchar *uri = g_strdup_printf ("ws://%s:%u", rps->get_rp_address (), rps->port);
                rps->status_append (uri, true);
                rps->status_append ("\n", true);
                g_free (uri);
                rps->status_append (" * resolved IP: ", true);
                uri = g_strdup_printf ("ws://%s:%u", get_ip_from_hostname (rps->get_rp_address ()), rps->port);
                rps->status_append (uri, true);
                rps->status_append ("\n", true);

                websocketpp::lib::error_code ec;
                rps->con = rps->client->get_connection(uri, ec);
                g_free (uri);
                        
                if (ec) {
                        rps->status_append (" * EEE: could not create connection because:", true);
                        rps->status_append (ec.message().c_str(), true);
                        rps->status_append ("\n", true);
                        return NULL;
                }

                // Note that connect here only requests a connection. No network messages are
                // exchanged until the event loop starts running in the next line.
                rps->client->connect(rps->con);

                // Create a thread to run the ASIO io_service event loop
                //websocketpp::lib::thread asio_thread(&wsppclient::run, client);
                // Detach the thread (or join if you want to wait for it to finish)
                rps->asio_thread = websocketpp::lib::thread(&wsppclient::run, rps->client);
                rps->status_append (" * RPSPMC DMA stream thread: Connected. ASIO thread running next...\n", true);

                //rps->asio_thread.detach();
                rps->asio_thread.join();

                rps->status_append (" * RPSPMC DMA stream thread: ASIO thread terminated.\n", true);
                
        } catch (websocketpp::exception const & e) {
                rps->status_append (e.what(), true);
        }

        rps->status_append (" * Connected via ASIO, RPSPMC DMA stream thread: ASIO run finished.\n", true);
        
        return NULL;
}

void RP_stream::stream_connect_cb (gboolean connect){
        if (!get_rp_address ()) return;
        debug_log (get_rp_address ());

        if (connect){
                status_append (NULL, true); // clear
                status_append ("4. Connecting to RPSPMC Data Stream on RedPitaya: ", true);

                update_health ("Connecting stream...");
                
                wspp_asio_gthread = g_thread_new ("wspp_asio_gthread", wspp_asio_thread, this);
        } else {
                if (con){
                        status_append ("Dissconnecting...\n ", true);
                        std::error_code ec;
                        client->close(con, websocketpp::close::status::normal, "", ec);
                        //client->stop();
                        // close(websocketpp::connection_hdl, websocketpp::close::status::value, const std::string&, std::error_code&)
                        
                        update_health ("Dissconnected RP stream.\n");
                        con = NULL;
                }
        }
}



void  RP_stream::got_client_connection (GObject *object, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        g_message ("got_client_connection");
}


void Z85_decode_double(const char* source, unsigned int size, double* dest)
{
        char* src = (char*)source;
        unsigned int count = size; // (sizeof (double)=8) => 2x5 =: 10 bytes in Z85
        union { double d; struct { uint32_t u,v; } ii; } u;
        double* dst = dest;
        uint32_t num;

	// BASE85 REVERSE LOOKUP TABLE FROM BASE256:
	static char base256[] = {
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 07
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0f
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 17
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 1f
                0x00,0x44,0x00,0x54,0x53,0x52,0x48,0x00, // 27
                0x4b,0x4c,0x46,0x41,0x00,0x3f,0x3e,0x45, // 2f
                0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, // 37
                0x08,0x09,0x40,0x00,0x49,0x42,0x4a,0x47, // 3f
                0x51,0x24,0x25,0x26,0x27,0x28,0x29,0x2a, // 47
                0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32, // 4f
                0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a, // 57
                0x3b,0x3c,0x3d,0x4d,0x00,0x4e,0x43,0x00, // 5f
                0x00,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10, // 67
                0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18, // 6f
                0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20, // 77
                0x21,0x22,0x23,0x4f,0x00,0x50,0x00,0x00, // 7f
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 87 *** for safety sake up to 0xff ***
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 8f
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 97
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 9f
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // a7
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // af
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // b7
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // bf
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // c7
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // cf
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // d7
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // df
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // e7
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // ef
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // f7
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  // ff
	};
	
        for (; count--; src += 10, dst++){
                // decode Z85
                num =            base256[src[0]];
                num = num * 85 + base256[src[1]];
                num = num * 85 + base256[src[2]];
                num = num * 85 + base256[src[3]];
                num = num * 85 + base256[src[4]];
                // unpack frame, low 32
                num = u.ii.u = num;
                // decode Z85
                num =            base256[src[5]];
                num = num * 85 + base256[src[6]];
                num = num * 85 + base256[src[7]];
                num = num * 85 + base256[src[8]];
                num = num * 85 + base256[src[9]];
                // unpack frame, high 32
                num = u.ii.v = num;
                // store resulting double
                *dst = u.d;
                
                //std::cout << u.d << " <= "
                //          << u.ii.u << ", "
                //          << u.ii.v
                //          << std::endl;
        }
}


// SHM VECTOR COMPONENTS, SIZES
#define VEC_VNUM 20  // vectors/block

#define VEC___T  0     
#define VEC___X  1 // 1,2: MAX, MIN peal readings, slow
#define VEC___Y  4 // 5,6: MAX, MIN
#define VEC___Z  7 // 8,9: MAX, MIN
#define VEC___U  10
#define VEC_IN1  11
#define VEC_IN2  12
#define VEC_IN3  13
#define VEC_IN4  14
#define VEC__AMPL 15
#define VEC__EXEC 16
#define VEC_DFREQ 17
#define VEC_PHASE 18
#define VEC_ZSSIG 19
        
#define VEC_LEN  20 // num components

void  RP_stream::on_message(RP_stream* self, websocketpp::connection_hdl hdl, wsppclient::message_ptr msg)
{
	gconstpointer contents;
	gconstpointer contents_next;
	gsize len;
        gchar *tmp;

        static int position=0;
        static int count=0;
        static bool finished=false;

        static size_t bram_offset=0;

        //self->debug_log ("WebSocket SPMC message received.");
	//self->status_append ("WebSocket SPMC message received.\n", true);

        //RP_stream *self = (RP_stream*)(*c)->get_user_data(hdl);

        if (msg->get_opcode() == websocketpp::frame::opcode::text) { // TEXT MESSAGE RECEIVED
                contents = msg->get_payload().c_str();
                len = msg->get_payload().size();

                //puts(contents);

                contents_next = contents;
                gchar *p;
                do {
                // NOTE: ADDED UNIQUE "_" to string IDs, what is NOT part if Gxsm's Z85 encode characters, as Z85 theoretically could genertate any key as part of data
                        if (p=g_strrstr (contents_next, "{_Z85DVector[")){
                                gsize size = atoi (p+13);
                                // ENCODED Z85 SIZE IS: unsigned int sizeZ85 = vec.size()*2*5; // Z85 encoding is 2x5 bytes by 8 bytes as of double
                                if ((p=g_strrstr (p+13, "]: {")) && (len-(gsize)(p-(gchar*)contents)) > (4+size*2*5)){
                                        double vec[size];
                                        //g_message ("Z85DVector=%s", p+4);
                                        Z85_decode_double(p+4, size, vec);
                                        //self->on_new_Z85Vector_message (vec, size); // process vector message
#if 0
                                        for (int k=0; k<size; k+=VEC_LEN){
                                                printf ("Z85DVector [%d] #%03d [@ %.3f s, XYZ: %g %g %g V, U: %g V "
                                                        "IN34: %g %g V DF %g Hz AMEX: %g %g V]\n",
                                                        size, k,
                                                        vec[k+VEC___T],  vec[k+VEC___X],vec[k+VEC___Y],vec[k+VEC___Z],  vec[k+VEC___U],
                                                        vec[k+VEC_IN3], vec[k+VEC_IN3],  vec[k+VEC_DFREQ],  vec[k+VEC__AMPL],vec[k+VEC__EXEC]);
                                        }
                                        //self->on_new_Z85Vector_message (vec, size); // process vector message ** badly broken
#endif
                                        if (self->shm_memory){
                                                if (size == 400){ // Enforce Verify Length to current setup of 20 x 20 vectors
                                                        memcpy  (self->shm_memory, &vec[VEC_LEN*(VEC_VNUM-1)], 10*sizeof(double));
                                                        memcpy  (self->shm_memory+100*sizeof(double), &vec[0], size*sizeof(double));
                                                        for (int k=0; k<size; k+=20)
                                                                rpspmc_hwi->push_history_vector (self->shm_memory+(100+k)*sizeof(double), 20);
                                                        // advance
                                                        contents_next += 13+4+size*sizeof(double);
                                                } else {
                                                        g_warning ("Z85DVector Error. Size Mismatch Config <%s>", g_strrstr (contents, "{Z85Vector[")); break;
                                                }
                                        } else {
                                                g_warning ("Z85DVector Push SHM Invalid Error. Message=<%s>", g_strrstr (contents, "{Z85Vector[")); break;
                                        }
                                } else {
                                        g_warning ("Z85DVector Format Data Error. Message=<%s>", g_strrstr (contents, "{Z85Vector[")); break;
                                }

                        }
                } while (p=g_strrstr (contents_next, "{_Z85DVector["));

                /* why or from where is this printed to stdout???
_SPMC_GVP_CONTROL_MODE: {_EXECUTE: RESET, start stream server, out of RESET * _ROpt=0}}}
{_Z85DVector[400]: {p>JgWk/:]=00000kK:>c00000kLeB[00000kK:>c00000kHzSY00000kHzSYkMy=:kGe/&Fb/MHZ.u:oFb/MHZ.u:oZYjumZ.u:-00000ZWW)50000000000000000000000000kD:m^00000Zt={ddaU4?kzi/Gp^94)kAs(Jq?44jkQ0Qqf6>>LkTtTN06O-5kD:j%H$Ru-k/:]=00000kK:>caohxwkLey%00000kK:>c00000kHzSY00000kHzSYaohxwkGe{!Fb/MHZ.u:oFb/MHZ.u:oZYjumZ.u:-00000ZWW/E0000000000000000000000000kD+Hh00000kiS4EdhZ#vkzj#m[&9w>kFbuRX:A&9kQmOP9VYVekTO=H06S4EkD+(VZ%xbGk/:]=00000kK:>cPA1@(kLew200000kK:>c00000kHzSY00000kHzSYaohxwkGf4-Fb/MHZ.u:oFb/MHZ.u:oZYjumZ.u:-00000ZWW-00000000000000000000000000kD={&00000kh%uTde](qkzjI.@3HBOkEkuAk)9hGkQAP!LSss>kT:lL06UtpkD^37{/.O{k/:]=00000kK:>cu&QfbkLet800000kK:>c00000kHzSY00000kHzSYkMy=:kGfeVFb/MHZ.u:oFb/MHZ.u:oPA1@(Z.u:-00000ZWWUJ0000000000000000000000000kD!r.00000kl0>FdgCcjkzj=g{8}ULkD[=}!W-D}kQq]0IwNwSkTS{V06XtukD!JQm*m={k/:]^00000kK:>ckMy=:kLeqe00000kK:>c00000kHzSY00000kHzSYPA1@(kGfoOFb/MHZ.u:oFb/MHZ.u:oPA1@(Z.u:-00000ZWWLu0000000000000000000000000kD*or00000ZtIlRde!V]kzjH3@590FkF3b>5BefNkP@.A7eP5LkTp]E06-HkkD*QLE!f.ek/:]^00000kK:>cFb/MHkLenk00000kK:>c00000kHzSY00000kHzSYaohxwkGfyHFb/MHZ.u:oFb/MHZ.u:oPA1@(Z.u:-00000ZWWE+0000000000000000000000000kD?:200000kjp-pde=zekzjGU{=r(/kDNd>wN(5DkP[D)4bR+*kTl/#06=aukD?]QWui@uk/:]^00000kK:>c00000kLekr00000kK:>c00000kHzSY00000kHzSYZYjumkGfIyFb/MHZ.u:oFb/MHZ.u:oFb/MHZ.u:-00000ZWWyz0000000000000000000000000kD<6G00000kloahddeiQkzjj(0W?wRkEmNA4(*=[kQfQxR{VWUkTI6m06!*EkD<nV)3FUMk/:]^00000kK:>cPA1@(kLehx00000kK:>c00000kHzSY00000kHzSYPA1@(kGfSpFb/MHZ.u:oFb/MHZ.u:oFb/MHZ.u:-00000ZWWr}0000000000000000000000000kD>sx00000kky&KdeK0xkzjDn@3)02kEAScM5-uOkQv#a^UZO>kTX/703zS0kD>qRbO>v=k/:]!00000kK:>cPA1@(kLeeE00000kK:>c00000kHzSY00000kHzSYFb/MHkGf:fFb/MHZ.u:oFb/MHZ.u:oFb/MHZ.u:-00000ZWWlu0000000000000000000000000kD>$]00000kj!*Pdft2$kzjOM]D&rEkCEOj42zy[kQwD8UjJL5kTYmH03Bl(kD(aMtCPYnk/:]!00000kK:>cZYjumkLebL00000kK:>c00000kHzSY00000kHzSYPA1@(kGf>4Fb/MHZ.u:oFb/MHZ.u:ou&QfbZ.u:-00000ZWWe(0000000000000000000000000kD(.i00000klfQwdc=sXkzjeA0Z2E$kFuRP?L2-9kQj5FxJrHBkTLx103CMfkD(=iLqwXik/:]!00000kK:>ckMy=:kLe8T00000kK:>c00000kHzSY00000kHzSY00000kGf$]Fb/MHZ.u:oFb/MHZ.u:ou&QfbZ.u:-00000ZWW8p0000000000000000000000000kD)B800000ZrMYUdej].kzjy}@3@atkC:5Q-lQWGkP#RdUE{^jkTsXW03D[JkD)Cx+j<jNk/:]!00000kK:>c?#A-SkLe5.00000kK:>c00000kHzSY00000kHzSYu&QfbkGg8+Fb/MHZ.u:oFb/MHZ.u:ou&QfbZ.u:-00000ZWW1Y0000000000000000000000000kD[dd00000kh?1(dc0M<kzj3HFg&wOkuB9J:+NwakP}4u/Y>E+kTop}03FDJkD[jx0^<)rk/:]/00000kK:>c?#A-SkLe2*00000kK:>c00000kHzSY00000kHzSY00000kGgiQFb/MHZ.u:oFb/MHZ.u:okMy=:Z.u:-00000ZWV}a0000000000000000000000000kD[*]00000kj-dZdbd0+kzi)D2S3o(kE[iIMWzQfkQ9OfRCw(7kTCn)03G(0kD[{RiHQjek/:]/00000kK:>c00000kLd#{00000kK:>c00000kHzSY00000kHzSYZYjumkGgsBFb/MHZ.u:oFb/MHZ.u:okMy=:Z.u:-00000ZWV<T0000000000000000000000000kD]C<00000ke40%deUh=kzjE%@3B8TkEhf@*Pe9&kQq@H1IbF9kTS$403I4+kD]IHAvQW&k/:]/00000kK:>cFb/MHkLd%200000kK:>c00000kHzSY00000kHzSYZYjumkGgCmFb/MHZ.u:oFb/MHZ.u:okMy=:Z.u:-00000ZWV^50000000000000000000000000kD{m800000kk6HbddgFvkzjkm0X?A&kE+93bJMumkQy:3(HciJkT.D)03JXJkD{txSh$c{k/:]/00000kK:>caohxwkLd{b00000kK:>c00000kHzSY00000kHzSYZYjumkGgM6Fb/MHZ.u:oFb/MHZ.u:oaohxwZ.u:-00000ZWVYO0000000000000000000000000kD{{x00000klt^7dfj@%kzjN5}xJOIkE63yi?hMqkQrb{P[:Y6kTTc<03Lg*kD}83&h]%/k/:]/00000kK=Sm?#A-SkLd)wZYjumkK:>o00000kHQo+00000kHQo+PA1@(kGgYY00000Z.u:700000Z.u:7aohxwZ.u:-00000ZWVR(0000000000000000000000000kD}Q<00000Zs-F^dg6xWkzjY]{-JNEkD(SxT8>=JkP@QO4e2}ZkTp/203MOTkD}^C8w{Phk/:]*00000kL0qrPA1@(kLd(ju&QfbkK:)200000kJs>Z00000kJs>ZZYjumkGhik00000Z.u..00000Z.u..00000Z.u:-00000ZWVLf0000000000000000000000000kD@sC00000kkVJ6dc%h%kzjh51uj2&kDAU?!ZijOkP{KOVD2%IkTn/?03O2+kD@HHq-GZ@k/:]*00000kLiZ%00000kLiZ%00000kK:{s00000kKp2b00000kKp2bFb/MHkGi3rFb/MHZ.uZIFb/MHZ.uZIPA1@(Z.u:.00000ZWVEu0000000000000000000000000kD%5C00000Zr3uPdd/=5kzjt903X!wkE1x/n[.33kQ7#clTe9dkTA-b03PCEkD%i<I*ig1k/:]*00000kLx@B00000kLx@Bu&QfbkK:$]00000kKcaAZYjumkKp12kMy=:kGi!}00000Z.uZ.Fb/MHZ.uZIu&QfbZ.u:.00000ZWVx+0000000000000000000000000kD%=x00000kk}hPdcGI^kzjaE2Sg?tkDn<50aR8<kQwlLwsD1+kTY5T03Q*(kD%[M}
                */
                
                gchar *pe=contents;
                while (pe && (p=g_strrstr (pe, "#_***"))){
                        p+=2;
                        pe=g_strrstr (p, "}");
                        if (pe){
                                std::string info = std::string(p, pe);
                                std::cout << info << std::endl; 
                                self->status_append (info.c_str(), true);
                                //tmp = g_strdup_printf ("%.*s", (int)(pe-p), p);
                                //self->status_append (tmp, true);
                                //g_message (tmp);
                                //g_free (tmp);
                        }
                }
                pe=contents;
                while (pe && (p=g_strrstr (pe, "_Info: {"))){
                        p+=8;
                        pe=g_strrstr (p, "}");
                        if (pe){
                                std::string info = std::string(p, pe);
                                std::cout << info << std::endl; 
                                self->status_append (info.c_str(), true);
                                //tmp = g_strdup_printf ("II: %.*s", (int)(pe-p), p);
                                //self->status_append (tmp, true);
                                //g_message (tmp);
                                //g_free (tmp);
                        }
                }
                //g_message ("WS Message: %s", (gchar*)contents);

                if (g_strrstr (contents, "_RESET")){
                        std::cout << "** WEBSOCKET STREAM TAG: RESET (GVP Start) Received. **" << std::endl; 
                        //self->status_append ("** WEBSOCKET STREAM TAG: RESET (GVP Start) Received.\n", true);
                        //g_message ("** WEBSOCKET STREAM TAG: RESET (GVP Start) Received.");
                        finished=false;
                        position=0;
                        count = 0;
                        // TESTing:
                        self->on_new_data (NULL, 0, true); // WS-PP data stream
                }
                
                if ((p=g_strrstr(contents, "_Position:{0x"))){ // SIMPLE JSON BLOCK
                        position = strtoul (p+11, NULL, 16);
                        if ((p=g_strrstr (contents, "_Count:{")))
                                count = atoi (p+8);
                        //g_message("*** ==> pos: 0x%06x #%d", position, count);
                        //g_message ("** POS: %s **", contents);
                        //puts(contents);
                        //{ gchar *tmp; self->status_append (tmp=g_strdup_printf("*** ==> pos: 0x%06x #%d\n", position, count), true); g_free(tmp); }
                }

                if (g_strrstr (contents, "_FinishedNext:{true}")){
                        std::cout << "** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received" << std::endl;
                        //self->status_append ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received\n", true);
                        //g_message ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received");
                        finished=true;
                }
                
                if ((p=g_strrstr(contents, "_BRAMoffset:{0x"))){ // SIMPLE JSON BLOCK
                        bram_offset = strtoul(p+13, NULL, 16);
                }

                pe = contents;
                while (p=g_strrstr (pe, "_vector = {")){
                        p+=11;
                        if ((pe = g_strrstr (p, "// Vector #"))){
                                self->last_vector_pc_confirmed = atoi (pe+11);
                                std::cout << "** VECTOR #" << self->last_vector_pc_confirmed << " confirmed. V := {" << std::string(p, pe+13) << std::endl;
                                //g_message ("** VECTOR #%02d confirmed. V := [%.*s]", self->last_vector_pc_confirmed, (int)(pe-p), p);
                                //{ gchar *tmp; self->status_append (tmp=g_strdup_printf("VECTOR: %.*s", (int)(pe-p), p), true); g_free(tmp); } // ASIO danger here!
                        } else {
                                g_warning ("No Vector # found, last confirmed: %d. %s", self->last_vector_pc_confirmed, contents);
                                break;
                        }
                }
                
        } else { // BINARY DATA RECEIVED
                contents = msg->get_payload().c_str();
                len = msg->get_payload().size();
                if (len)
                        self->on_new_data (contents, len); // WS-PP data stream
        }
}

void  RP_stream::on_closed (GObject *object, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        self->status_append ("WebSocket stream connection externally closed.\n", true);
        self->status_append ("--> auto reconnecting...\n", true);
        self->stream_connect_cb (TRUE);
}




