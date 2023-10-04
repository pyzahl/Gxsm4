/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_pacpll.C
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
#include <libsoup/soup.h>
#include <zlib.h>

#include "config.h"

#include "rpspmc_pacpll.h"

extern SPMC_parameters spmc_parameters;

/* *** RP_stream module *** */

void RP_stream::stream_connect_cb (gboolean connect){
        if (!get_rp_address ()) return;
        debug_log (get_rp_address ());

        if (connect){
                status_append (NULL); // clear
                status_append ("Connecting to SPM Stream on RedPitaya...\n");

                update_health ("Connecting stream...");

                // new soup session
                session = soup_session_new ();

                // then connect to Stream Socket on RP
                gchar *url = g_strdup_printf ("ws://%s:%u", get_rp_address (), port);
                status_append ("Connecting to: ");
                status_append (url);
                status_append ("\n");
                // g_message ("Connecting to: %s", url);

                msg = soup_message_new ("GET", url);
                g_free (url);
                // g_message ("soup_message_new - OK");
                soup_session_websocket_connect_async (session, msg, // SoupSession *session, SoupMessage *msg,
                                                      NULL, NULL, // const char *origin, char **protocols,
                                                      NULL,  RP_stream::got_client_connection, this); // GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data
                //g_message ("soup_session_websocket_connect_async - OK");
        } else {
                // tear down connection
                status_append ("Dissconnecting...\n ");
                update_health ("Dissconnected");

                //g_clear_object (&listener);
                g_clear_object (&client);
                g_clear_error (&client_error);
                g_clear_error (&error);
        }
}

void  RP_stream::got_client_connection (GObject *object, GAsyncResult *result, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        g_message ("got_client_connection");

	self->client = soup_session_websocket_connect_finish (SOUP_SESSION (object), result, &self->client_error);
        if (self->client_error != NULL) {
                self->status_append ("Connection failed: ");
                self->status_append (self->client_error->message);
                self->status_append ("\n");
                g_message ("%s", self->client_error->message);
        } else {
                self->status_append ("RedPitaya SPMC Stream Socket Connected!\n ");
		g_signal_connect(self->client, "closed",  G_CALLBACK( RP_stream::on_closed),  self);
		g_signal_connect(self->client, "message", G_CALLBACK( RP_stream::on_message), self);
		//g_signal_connect(connection, "closing", G_CALLBACK(on_closing_send_message), message);

                self->on_connect_actions (); // setup instrument, send all params, ...

                self->status_append ("RedPitaya SPM Control, SPMC Stream is ready.\n ");
        }
}

void  RP_stream::on_message(SoupWebsocketConnection *ws,
                               SoupWebsocketDataType type,
                               GBytes *message,
                               gpointer user_data)
{
        RP_stream *self = ( RP_stream *)user_data;
	gconstpointer contents;
	gsize len;
        gchar *tmp;

        static int position=0;
        static int count=0;
        static int count_prev=-1;
        static int streamAB=0;
        static bool finished=false;
        
        //self->debug_log ("WebSocket SPMC message received.");
	//self->status_append ("WebSocket SPMC message received.\n");
        
	if (type == SOUP_WEBSOCKET_DATA_TEXT) {
                gchar *p;
		contents = g_bytes_get_data (message, &len);
                if (contents && len < 100){
                        tmp = g_strdup_printf ("WEBSOCKET_DATA_TEXT: %s", contents);
                        self->status_append (tmp);
                        g_message (tmp);
                        g_free (tmp);
                } else {
                        self->status_append ("WEBSOCKET_DATA_TEXT ------\n");
                        if (contents && len > 0)
                                self->status_append ((gchar*)contents);
                        else
                                self->status_append ("Empty text message received.");
                        if (g_strrstr (contents, "\n"))
                                self->status_append ("--------------------------\n");
                        else
                                self->status_append ("\n--------------------------\n");
                }
                //g_message ("WS Message: %s", (gchar*)contents);

                if (g_strrstr (contents, "RESET")){
                        self->status_append ("** WEBSOCKET STREAM TAG: RESET (GVP Start) Received.\n");
                        g_message ("** WEBSOCKET STREAM TAG: RESET (GVP Start) Received");
                        finished=false;
                        position=0;
                        streamAB=0;
                        count = 0;
                        count_prev = -1;

                }
                
                if (g_strrstr (contents, "FinishedNext:{true}")){
                        self->status_append ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received\n");
                        g_message ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received");
                        finished=true;
                }
                
                if ((p=g_strrstr(contents, "Position:{"))){ // SIMPLE JSON BLOCK
                        position = atoi (p+10);
                        if ((p=g_strrstr (contents, "Count:{")))
                                count = atoi (p+7);
                }

                if (g_strrstr (contents, "vector = {")){
                        if ((p = g_strrstr (contents, "// Vector #"))){
                                self->last_vector_pc_confirmed = atoi (p+11);
                                g_message (p);
                        }
                }
               
	} else if (type == SOUP_WEBSOCKET_DATA_BINARY) {
		contents = g_bytes_get_data (message, &len);

                tmp = g_strdup_printf ("WEBSOCKET_DATA_BINARY SPMC Bytes: 0x%04x,  Position: 0x%04x + AB=%d x BRAMSIZE/2, Count: %d\n", len, position, streamAB, count);
                self->status_append (tmp);
                g_message (tmp);
   
                //self->status_append_int32 (contents, 512, true, streamAB*len, true); // truncate, just a snap
                //self->status_append ("\n");
                //self->debug_log (tmp);
                g_free (tmp);

#if 1
                FILE* pFile;
                tmp = g_strdup_printf ("WS-BRAM-DATA-BLOCK_%03d_Pos0x%04x_AB_%02d%s.bin", count, position, streamAB, finished?"F":"C");
                pFile = fopen(tmp, "wb");
                g_free (tmp);
                fwrite(contents, 1, len, pFile);
                fclose(pFile);
                // hexdump -v -e '"%08_ax: "' -e ' 16/4 "%08x_L[red:0x018ec108,green:0x018fffff] " " \n"' WS-BRAM-DATA-BLOCK_000_Pos0x1f7e_AB_00.bin
#endif
                streamAB = self->on_new_data (contents, len, position, count-count_prev, finished); // process data
                count_prev = count;
        }
}

void  RP_stream::on_closed (SoupWebsocketConnection *ws, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        self->status_append ("WebSocket connection externally closed.\n");
}

/*
*** DEBUGGING STREAM boundary cross over issue

WS-BRAM-DATA-BLOCK_000_Pos0x1f53_AB_00C.bin  
WS-BRAM-DATA-BLOCK_000_Pos0x2156_AB_00C.bin  
WS-BRAM-DATA-BLOCK_001_Pos0x0056_AB_01C.bin  << ????
WS-BRAM-DATA-BLOCK_001_Pos0x235e_AB_00C.bin  << ????
WS-BRAM-DATA-BLOCK_001_Pos0x2566_AB_01C.bin  

** Message: 20:33:06.923: WS Message: {Position:{8454},Count:{0}}

** Message: 20:33:06.924: WEBSOCKET_DATA_BINARY SPMC Bytes: 0x8000,  Position: 0x2106 + AB=0 x BRAMSIZE/2, Count: 0

** Message: 20:33:06.924: on_new_data ** AB=0 pos=8454  buffer_pos=0x00002106  new_count=0  ...
** Message: 20:33:06.925: VP: Waiting for Section Header [0] StreamPos=0x00002106
** Message: 20:33:06.925: VP: section header ** reading pos[2106] off[1ff1] #AB=0
** Message: 20:33:06.925: Reading VP section header...
00001fd1: 00000000 ffd7828a f3a67848 ffd77efc 0c3de678 00000000 02a0c108 f399ab7c ffd77efc 0c3fc354 00000000 029fc108 f38cdeb0 ffd77a09 0c41a030 0c59d75c
00001fe1: 029ec108 ffd7692c ffd77a09 0c437d0c 00000000 029dc108 f3734518 ffd7797d 0c4559e8 00000000 029cc108 f366784c ffd7797d 0c4736c4 00000000 00000000
00001ff1: f359ab80 0c30dc74 0c4913a0 00000000 029ac108 f34cdeb4 ffd78012 0c4af07c 00000000 0299c108 f34011e8 ffd78012 0c4ccd58 00000000 0298c108 028bc108
00002001: ffd7a132 0c4eb786 00000000 0297c108 e8c5abce ffd7a132 0c509462 00000000 0296c108 e8b8df02 ffd79cae 0c52713e 00000000 0295c108 e8ac1236 00000000
00002011: 0c544e1a 00000000 0294c108 e89f456a ffd7981e 0c562af6 00000000 0293c108 e892789e ffd7981e 0c5807d2 00000000 0292c108 e885abd2 ffd794a0 02a1c108


** (gxsm4:1892279): WARNING **: 20:33:06.925: ERROR: read_GVP_data_block_to_position_vector: Reading offset 00001ff1, write position 00002106. Expecting full header but found srcs=ab80, i=-3239
** Message: 20:33:06.925: *** GVP: STREAM ERROR, NO HEADER WHEN EXPECTED -- aborting. ***
===>>>> SET-VP  i[1571] sec=1 t=1301.33 ms   #valid sec{1572}
** Message: 20:33:06.925: FR::FINISH-OK
** Message: 20:33:06.925: ProbeFifoReadThread ** Finished ** FF=True

Thread 1 "gxsm4" received signal SIGSEGV, Segmentation fault.













===>>>> SET-VP  i[1024] sec=1 t=100.09 ms   #valid sec{1025}
** Message: 22:23:58.075: [00001ba8] *** end of new data at ch=3 ** Must wait for next page and retry.
** Message: 22:23:58.075: VP: waiting for data ** section: 1 gvpi[0687] Pos:{0x22ef} Offset:0x1ba8
** Message: 22:23:58.130: WS Message: {Info: { position: 12265}}

** Message: 22:23:58.132: WS Message: {Position:{12265},Count:{0}}

** Message: 22:23:58.132: WEBSOCKET_DATA_BINARY SPMC Bytes: 0x8000,  Position: 0x2fe9 + AB=0 x BRAMSIZE/2, Count: 0

** Message: 22:23:58.133: on_new_data ** AB=0 pos=12265  buffer_pos=0x00002fe9  new_count=0  ...
** Message: 22:23:58.176: VP: Waiting for Section Header [0] StreamPos=0x00002fe9
** Message: 22:23:58.176: VP: section header ** reading pos[2fe9] off[1fe1] #AB=0
** Message: 22:23:58.176: Reading VP section header...
00001fc1: 00000000 ffd787b6 0a842828 ffd785f9 0124ae68 00000000 01dcc108 0a840764 ffd785f9 0124de20 00000000 01dbc108 0a83e6a0 ffd785f9 01250dd8 00000000
00001fd1: 01dac108 01277a30 ffd785f9 01253d90 00000000 01d9c108 0a83a518 ffd785f9 01256d48 00000000 01d8c108 0a838454 ffd785f9 01259d00 00000000 00000000
00001fe1: 0a836390 01236060 0125ccb8 00000000 01d6c108 0a8342cc ffd785f9 0125fc70 00000000 01d5c108 0a832208 ffd785f9 01262c28 00000000 01d4c108 01c7c108
00001ff1: ffd785f9 00000000 00000000 01d3c108 0a82e080 ffd785f9 01268b98 00000000 01d2c108 0a82bfbc ffd785f9 0126bb50 00000000 01d1c108 0a829ef8 00b14464
00002001: cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc cccccccc


** (gxsm4:1900768): WARNING **: 22:23:58.176: ERROR: read_GVP_data_block_to_position_vector: Reading offset 00001fe1, write position 00002fe9. Expecting full header but found srcs=6390, i=2691
** Message: 22:23:58.176: *** GVP: STREAM ERROR, NO HEADER WHEN EXPECTED -- aborting. ***
===>>>> SET-VP  i[1576] sec=1 t=100.09 ms   #valid sec{1577}
** Message: 22:23:58.176: FR::FINISH-OK
** Message: 22:23:58.176: ProbeFifoReadThread ** Finished ** FF=True
** Message: 22:23:58.231: WS Message: {Info: { position: 1068}}

** Message: 22:23:58.232: WS Message: {Position:{1068},Count:{1}}

** Message: 22:23:58.235: WEBSOCKET_DATA_BINARY SPMC Bytes: 0x8000,  Position: 0x042c + AB=0 x BRAMSIZE/2, Count: 1

** Message: 22:23:58.235: on_new_data ** AB=1 pos=1068  buffer_pos=0x0000242c  new_count=1  ...
** Message: 22:23:58.334: WS Message: {Info: { position: 6255}}
















** Message: 13:18:16.110: on_new_data ** AB=0 pos=8197  buffer_pos=0x00002005  new_count=0  ...
** Message: 13:18:16.140: VP: Waiting for Section Header [0] StreamPos=0x00002005
** Message: 13:18:16.140: VP: section header ** reading pos[2005] off[2002] #AB=0
** Message: 13:18:16.140: Reading VP section header...
00001fe2: 06025290 0693c038 061a5a3a fffffe31 000015cc 8d82e194 00000002 0692c038 061c8978 fffffe26 00001741 8e0209b0 00000002 0691c038 cccccccc fffffe44
00001ff2: 00000002 8e8131cc 00000002 0690c038 0620e7f4 fffffe38 00000d95 8f0059e8 00000002 068fc038 06231732 fffffe45 0000042a 8f7f8204 00000000 00000000
00002002: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
00002012: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
00002022: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000


** (gxsm4:2043331): WARNING **: 13:18:16.140: ERROR: read_GVP_data_block_to_position_vector: Reading offset 00002002, write position 00002005. Expecting full header but found srcs=0000, i=0
** Message: 13:18:16.140: *** GVP: DATA STREAM ERROR, NO VALID HEDAER AS EXPECTED -- aborting. EE ret = -97 ***
===>>>> SET-VP  i[1322] sec=0 t=0 ms   #valid sec{1323}



  
 */
