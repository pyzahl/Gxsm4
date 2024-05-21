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

        static size_t bram_offset=0;

        static int count_stream=0;
        
        //self->debug_log ("WebSocket SPMC message received.");
	//self->status_append ("WebSocket SPMC message received.\n");
        
	if (type == SOUP_WEBSOCKET_DATA_TEXT) {
                gchar *p;
		contents = g_bytes_get_data (message, &len);
                if (contents && len < 100){
                        tmp = g_strdup_printf ("WEBSOCKET_DATA_TEXT: %s", contents);
                        //self->status_append (tmp);
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
                        count_stream=0;
                }
                
                if ((p=g_strrstr(contents, "Position:{0x"))){ // SIMPLE JSON BLOCK
                        position = strtoul (p+10, NULL, 16); // effin addr hacks pactches!!!
                        if ((p=g_strrstr (contents, "Count:{")))
                                count = atoi (p+7);
                }

                if (g_strrstr (contents, "FinishedNext:{true}")){
                        self->status_append ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received\n");
                        g_message ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received");
                        finished=true;
                }
                
                if ((p=g_strrstr(contents, "BRAMoffset:{0x"))){ // SIMPLE JSON BLOCK
                        bram_offset = strtoul(p+12, NULL, 16);
                }
                
                if (g_strrstr (contents, "vector = {")){
                        if ((p = g_strrstr (contents, "// Vector #"))){
                                self->last_vector_pc_confirmed = atoi (p+11);
                                g_message (p);
                        }
                }
               
	} else if (type == SOUP_WEBSOCKET_DATA_BINARY) {
		contents = g_bytes_get_data (message, &len);

                //tmp = g_strdup_printf ("\nSTREAMINFO: BLOCK %03d  Pos 0x%04x  AB %02d  %s\n", count, position, streamAB, finished?"_Fini":"_Cont");
                //tmp = g_strdup_printf ("WEBSOCKET_DATA_BINARY SPMC Bytes: 0x%04x,  Position: 0x%04x + AB=%d x BRAMSIZE/2, Count: %d\n", len, position, streamAB, count);
                //self->status_append (tmp);
                //g_message (tmp);
   
                //self->status_append_int32 (contents, 512, true, streamAB*len, true); // truncate, just a snap
                //self->status_append ("\n");
                //self->debug_log (tmp);
                //g_free (tmp);

#if 0
                FILE* pFile;
                //tmp = g_strdup_printf ("WS-BRAM-DATA-BLOCK_%03d_Pos0x%04x_AB_%02d%s.bin", count, position, streamAB, finished?"_Fini":"_Cont");
                tmp = g_strdup_printf ("DMA-DATA-NEW_%08d%s.bin", count_stream, finished?"_Fini":"_Cont");
                pFile = fopen(tmp, "wb");
                g_free (tmp);
                fwrite(contents, 1, len, pFile);
                fclose(pFile);
                // hexdump -v -e '"%08_ax: "' -e ' 16/4 "%08x_L[red:0x018ec108,green:0x018fffff] " " \n"' WS-BRAM-DATA-BLOCK_000_Pos0x1f7e_AB_00.bin
#endif

                streamAB = self->on_new_data (contents, len, count_stream, 0);
                count_stream += (len>>2);

                count_prev = count;

                //tmp = g_strdup_printf ("WEBSOCKET_DATA_BINARY SPMC Bytes: 0x%04x,  Position: 0x%04x + AB=%d x BRAMSIZE/2, Count: %d\n", len, position, streamAB, count);
                //self->status_append (tmp);
                //g_free (tmp);
        }
}

void  RP_stream::on_closed (SoupWebsocketConnection *ws, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        self->status_append ("WebSocket stream connection externally closed.\n");
        self->status_append ("--> auto reconnecting...\n");
        self->stream_connect_cb (TRUE);
}
