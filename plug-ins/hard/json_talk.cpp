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


/* *** RP_JSON_talk module *** */

void RP_JSON_talk::json_talk_connect_cb (gboolean connect){
        if (!get_rp_address ()) return;
        debug_log (get_rp_address ());

        if (connect){
                status_append ("Connecting to RedPitaya...\n");

                update_health ("Connecting...");

                // new soup session
                session = soup_session_new ();

                // request to fire up RedPitaya PACPLLL NGNIX server
                gchar *urlstart = g_strdup_printf ("http://%s/bazaar?start=rpspmc",get_rp_address ());
                status_append ("1. Requesting NGNIX RedPitaya RPSPMC-PACPLL Server Startup:\n");
                status_append (urlstart);
                status_append ("\n ");
                msg = soup_message_new ("GET", urlstart);
                g_free (urlstart);
                GInputStream *istream = soup_session_send (session, msg, NULL, &error);

                if (error != NULL) {
                        g_warning ("%s", error->message);
                        status_append (error->message);
                        status_append ("\n ");
                        update_health (error->message);
                        return;
                } else {
                        gchar *buffer = g_new0 (gchar, 100);
                        gssize num = g_input_stream_read (istream,
                                                          (void *)buffer,
                                                          100,
                                                          NULL,
                                                          &error);   
                        if (error != NULL) {
                                update_health (error->message);
                                g_warning ("%s", error->message);
                                status_append (error->message);
                                status_append ("\n ");
                                g_free (buffer);
                                return;
                        } else {
                                status_append ("Response: ");
                                status_append (buffer);
                                status_append ("\n ");
                                update_health (buffer);
                        }
                        g_free (buffer);
                }

                // then connect to NGNIX WebSocket on RP
                status_append ("2. Connecting to NGNIX RedPitaya RPSPMC-PACPLL WebSocket...\n");
                gchar *url = g_strdup_printf ("ws://%s:%u", get_rp_address (), port);
                status_append (url);
                status_append ("\n");
                // g_message ("Connecting to: %s", url);
                
                msg = soup_message_new ("GET", url);
                g_free (url);
                // g_message ("soup_message_new - OK");
                soup_session_websocket_connect_async (session, msg, // SoupSession *session, SoupMessage *msg,
                                                      NULL, NULL, // const char *origin, char **protocols,
                                                      NULL,  RP_JSON_talk::got_client_connection, this); // GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data
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

void  RP_JSON_talk::got_client_connection (GObject *object, GAsyncResult *result, gpointer user_data){
        RP_JSON_talk *self = ( RP_JSON_talk *)user_data;
        g_message ("got_client_connection");

	self->client = soup_session_websocket_connect_finish (SOUP_SESSION (object), result, &self->client_error);
        if (self->client_error != NULL) {
                self->status_append ("Connection failed: ");
                self->status_append (self->client_error->message);
                self->status_append ("\n");
                g_message ("%s", self->client_error->message);
        } else {
                self->status_append ("RedPitaya WebSocket Connected!\n ");
		g_signal_connect(self->client, "closed",  G_CALLBACK( RP_JSON_talk::on_closed),  self);
		g_signal_connect(self->client, "message", G_CALLBACK( RP_JSON_talk::on_message), self);
		//g_signal_connect(connection, "closing", G_CALLBACK(on_closing_send_message), message);

                self->on_connect_actions (); // setup instrument, send all params, ...

                self->status_append ("RedPitaya SPM Control, PAC-PLL is ready.\n ");
        }
}

void  RP_JSON_talk::on_message(SoupWebsocketConnection *ws,
                               SoupWebsocketDataType type,
                               GBytes *message,
                               gpointer user_data)
{
        RP_JSON_talk *self = ( RP_JSON_talk *)user_data;
	gconstpointer contents;
	gsize len;
        gchar *tmp;
        
        self->debug_log ("WebSocket message received.");
        
	if (type == SOUP_WEBSOCKET_DATA_TEXT) {
		contents = g_bytes_get_data (message, &len);
		self->status_append ("WEBSOCKET_DATA_TEXT\n");
		self->status_append ((gchar*)contents);
		self->status_append ("\n");
                g_message ("%s", (gchar*)contents);
	} else if (type == SOUP_WEBSOCKET_DATA_BINARY) {
		contents = g_bytes_get_data (message, &len);

                tmp = g_strdup_printf ("WEBSOCKET_DATA_BINARY NGNIX JSON ZBytes: %ld", len);
                self->debug_log (tmp);
                g_free (tmp);

#if 0
                // dump to file
                FILE *f;
                f = fopen ("/tmp/gxsm-rpspmc-json.gz","wb");
                fwrite (contents, len, 1, f);
                fclose (f);
                // -----------
                //$ pzahl@phenom:~$ zcat /tmp/gxsm-rp-json.gz 
                //{"parameters":{"DC_OFFSET":{"value":-18.508743,"min":-1000,"max":1000,"access_mode":0,"fpga_update":0},"CPU_LOAD":{"value":5.660378,"min":0,"max":100,"access_mode":0,"fpga_update":0},"COUNTER":{"value":4,"min":0,"max":1000000000000,"access_mode":0,"fpga_update":0}}}pzahl@phenom:~$ 
                //$ pzahl@phenom:~$ file /tmp/gxsm-rp-json.gz 
                // /tmp/gxsm-rp-json.gz: gzip compressed data, max speed, from FAT filesystem (MS-DOS, OS/2, NT)
                // GZIP:  zlib.MAX_WBITS|16
#endif
                self->debug_log ("Uncompressing...");
                gsize size=len*100+1000;
                gchar *json_buffer = g_new0 (gchar, size);

                // inflate buffer into json_buffer
                z_stream zInfo ={0};
                zInfo.total_in  = zInfo.avail_in  = len;
                zInfo.total_out = zInfo.avail_out = size;
                zInfo.next_in  = (Bytef*)contents;
                zInfo.next_out = (Bytef*)json_buffer;
      
                int ret= -1;
                ret = inflateInit2 (&zInfo, MAX_WBITS + 16);
                if ( ret == Z_OK ) {
                        ret = inflate( &zInfo, Z_FINISH );     // zlib function
                        // inflate() returns
                        // Z_OK if some progress has been made (more input processed or more output produced),
                        // Z_STREAM_END if the end of the compressed data has been reached and all uncompressed output has been produced,
                        // Z_NEED_DICT if a preset dictionary is needed at this point,
                        // Z_DATA_ERROR if the input data was corrupted (input stream not conforming to the zlib format or incorrect check value, in which case strm->msg points to a string with a more specific error),
                        // Z_STREAM_ERROR if the stream structure was inconsistent (for example next_in or next_out was Z_NULL, or the state was inadvertently written over by the application),
                        // Z_MEM_ERROR if there was not enough memory,
                        // Z_BUF_ERROR if no progress was possible or if there was not enough room in the output buffer when Z_FINISH is used. Note that Z_BUF_ERROR is not fatal, and inflate() can be called again with more input and more output space to continue decompressing. If
                        // Z_DATA_ERROR is returned, the application may then call inflateSync() to look for a good compression block if a partial recovery of the data is to be attempted. 
                        switch ( ret ){
                        case Z_STREAM_END:
                                tmp = NULL;
                                if (self->get_debug_level () > 2)
                                        tmp = g_strdup_printf ("Z_STREAM_END out = %ld, in = %ld, ratio=%g\n",zInfo.total_out, zInfo.total_in, (double)zInfo.total_out / (double)zInfo.total_in);
                                break;
                        case Z_OK:
                                tmp = g_strdup_printf ("Z_OK out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break;
                        case Z_NEED_DICT:
                                tmp = g_strdup_printf ("Z_NEED_DICT out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break;
                        case Z_DATA_ERROR:
                                self->status_append (zInfo.msg);
                                tmp = g_strdup_printf ("\nZ_DATA_ERROR out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break; 
                                break;
                        case Z_STREAM_ERROR:
                                tmp = g_strdup_printf ("Z_STREAM_ERROR out = %ld\n",zInfo.total_out); break;
                        case Z_MEM_ERROR:
                                tmp = g_strdup_printf ("Z_MEM_ERROR out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break;
                        case Z_BUF_ERROR:
                                tmp = g_strdup_printf ("Z_BUF_ERROR out = %ld, in = %ld  ratio=%g\n",zInfo.total_out, zInfo.total_in, (double)zInfo.total_out / (double)zInfo.total_in); break;
                        default:
                                tmp = g_strdup_printf ("ERROR ?? inflate result = %d,  out = %ld, in = %ld\n",ret,zInfo.total_out, zInfo.total_in); break;
                        }
                        self->status_append (tmp);
                        g_free (tmp);
                }
                inflateEnd( &zInfo );   // zlib function
                if (self->get_debug_level () > 0){
                        self->status_append (json_buffer);
                        self->status_append ("\n");
                }

                self->json_parse_message (json_buffer);

                g_free (json_buffer);

                self->on_new_data ();
        }
	//g_bytes_unref (message); // OK, no unref by ourself!!!!
                       
}

void  RP_JSON_talk::on_closed (SoupWebsocketConnection *ws, gpointer user_data){
        RP_JSON_talk *self = ( RP_JSON_talk *)user_data;
        self->status_append ("WebSocket connection externally closed.\n");
}

void  RP_JSON_talk::json_parse_message (const char *json_string){
        jsmn_parser p;
        jsmntok_t tok[10000]; /* We expect no more than 10000 tokens, signal array is 1024 * 5*/

        // typial data messages:
        // {"signals":{"SIGNAL_CH3":{"size":1024,"value":[0,0,...,0.543632,0.550415]},"SIGNAL_CH4":{"size":1024,"value":[0,0,... ,-94.156487]},"SIGNAL_CH5":{"size":1024,"value":[0,0,.. ,-91.376022,-94.156487]}}}
        // {"parameters":{"DC_OFFSET":{"value":-18.591045,"min":-1000,"max":1000,"access_mode":0,"fpga_update":0},"COUNTER":{"value":2.4,"min":0,"max":1000000000000,"access_mode":0,"fpga_update":0}}}

        jsmn_init(&p);
        int ret = jsmn_parse(&p, json_string, strlen(json_string), tok, sizeof(tok)/sizeof(tok[0]));
        if (ret < 0) {
                g_warning ("JSON PARSER:  Failed to parse JSON: %d\n%s\n", ret, json_string);
                return;
        }
        /* Assume the top-level element is an object */
        if (ret < 1 || tok[0].type != JSMN_OBJECT) {
                g_warning("JSON PARSER:  Object expected\n");
                return;
        }

#if 0
        json_dump (json_string, tok, p.toknext, 0);
#endif

        json_fetch (json_string, tok, p.toknext, 0);
        if  (get_debug_level () > 1)
                dump_parameters (get_debug_level());
}



void  RP_JSON_talk::write_parameter (const gchar *parameter_id, double value, const gchar *fmt, gboolean dbg){
        //soup_websocket_connection_send_text (self->client, "text");
        //soup_websocket_connection_send_binary (self->client, gconstpointer data, gsize length);
        //soup_websocket_connection_send_text (client, "{ \"parameters\":{\"GAIN1\":{\"value\":200.0}}}");

        if (client){
                gchar *json_string=NULL;
                if (fmt){
                        gchar *format = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%s}}}", parameter_id, fmt);
                        json_string = g_strdup_printf (format, value);
                        g_free (format);
                } else
                        json_string = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%.10g}}}", parameter_id, value);
                soup_websocket_connection_send_text (client, json_string);
                if  (debug_level > 0 || dbg)
                        g_print ("%s\n",json_string);
                g_free (json_string);
        }
}

void  RP_JSON_talk::write_parameter (const gchar *parameter_id, int value, gboolean dbg){
        if (client){
                gchar *json_string = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%d}}}", parameter_id, value);
                soup_websocket_connection_send_text (client, json_string);
                if  (get_debug_level () > 0 || dbg)
                        g_print ("%s\n",json_string);
                g_free (json_string);
        }
}

void  RP_JSON_talk::write_array (const gchar *parameter_id[], int i_size, int *i_vec, int d_size, double *d_vec){
        if (client){
                int i=0;
                GString *list = g_string_new (NULL);
                g_string_append_printf (list, "{\"parameters\":{");
                for (; i<i_size && parameter_id[i]; ++i){
                        if (i) g_string_append (list, ",");
                        g_string_append_printf (list, "\"%s\":{\"value\":%d}",  parameter_id[i], i_vec[i]);
                }
                for (int j=0; j<d_size && parameter_id[i]; ++i, ++j){
                        if (i) g_string_append (list, ",");
                        g_string_append_printf (list, "\"%s\":{\"value\":%.10g}",  parameter_id[i], d_vec[j]);
                }
                g_string_append_printf (list, "}}");
                soup_websocket_connection_send_text (client, list->str);
                //g_print ("%s\n",list->str);
                g_string_free (list, true);
        }
}


// {"signals":{"SPMC_GVP_VECTOR":{"size":16,"value":[1,2,3,4,5.5,6.6,7.7,8.8,9.9,10,11,12,13,14,15,16]}}}
// {"signals":{"SIGNAL_CH3":{"size":1024,"value":[0,0,...,0.543632,0.550415]},"SIGNAL_CH4":{"size":1024,"value":[0,0,... ,-94.156487]},"SIGNAL_CH5":{"size":1024,"value":[0,0,.. ,-91.376022,-94.156487]
void  RP_JSON_talk::write_signal (const gchar *parameter_id, int size, double *value, const gchar *fmt=NULL, gboolean dbg=FALSE){
        if (client){
                std::vector<char> uncompressed(0);
#if 1
                GString *list = g_string_new (NULL);
                if (fmt) // MUST INCLUDE COMMA: default is "%g,"
                        for (int i=0; i<size; ++i)
                                g_string_append_printf (list, fmt, value[i]);
                else
                        for (int i=0; i<size; ++i)
                                g_string_append_printf (list, "%.10g,", value[i]);
                
                list->str[list->len-1]=']';
                gchar *json_string = g_strdup_printf ("{\"signals\":{\"%s\":{\"size\":%d,\"value\":[%s}}}", parameter_id, size, list->str);
                g_string_free (list, true);

#else // add to vector directly -- bogus, 2nd call to add not working ??!?

                gchar *jstmp = g_strdup_printf ("{\"signals\":{\"%s\":{\"size\":%d,\"value\":[", parameter_id, size);
                add_string_to_vector(uncompressed, jstmp);
                g_free (jstmp);

                if (fmt){ // MUST INCLUDE COMMA: default is "%g,"
                        for (int i=0; i<size; ++i){
                                jstmp = g_strdup_printf (fmt, value[i]);
                                add_string_to_vector(uncompressed, jstmp);
                                g_free (jstmp);
                        }
                } else {
                        for (int i=0; i<size-1; ++i){
                                jstmp = g_strdup_printf ("%g,", value[i]);
                                add_string_to_vector(uncompressed, jstmp);
                                g_free (jstmp);
                        }
                        jstmp = g_strdup_printf ("%g", value[size-1]);
                        add_string_to_vector(uncompressed, jstmp);
                        g_free (jstmp);
                }

                jstmp = g_strdup_printf ("]}}}");
                add_string_to_vector(uncompressed, jstmp);
                g_free (jstmp);
#endif
                

#if 0 // send as plain text -- does nothing ?!?!
                soup_websocket_connection_send_text (client, json_string);
#else // send compressed, -- causing the server to trap :( send uncompressed see below 
                add_string_to_vector(uncompressed, json_string);
#endif                

                if  (get_debug_level () > 0 || dbg)
                        g_print ("%s\n",json_string);
                g_free (json_string);

                std::vector<char> compressed(0);
                int compression_result = compress_vector(uncompressed, compressed);
                assert(compression_result == F_OK);

                std::vector<char> decompressed(0);
                int decompression_result = decompress_vector(compressed, decompressed);
                assert(decompression_result == F_OK);

                printf("Uncompressed [%d]: %s\n", uncompressed.size(), uncompressed.data());
                printf("Compressed   [%d]: ", compressed.size());
                std::ostream &standard_output = std::cout;
                print_bytes(standard_output, (const unsigned char *) compressed.data(), compressed.size(), false);
                printf("Decompressed [%d]: %s\n", decompressed.size(), decompressed.data());

                //soup_websocket_connection_send_binary (client, compressed.data(), compressed.size()); // crashes server
                soup_websocket_connection_send_text (client, uncompressed.data());
                
        }
}

void  RP_JSON_talk::write_signal (const gchar *parameter_id, int size, int *value, gboolean dbg=FALSE){
        if (client){
                GString *list = g_string_new (NULL);
                for (int i=0; i<size; ++i)
                        g_string_append_printf (list, "%d,", value[i]);
                list->str[list->len-1]=']';
                gchar *json_string = g_strdup_printf ("{ \"signals\":{\"%s\":{\"size\":%d,\"value\":[%s}}}", parameter_id, size, list->str);
                g_string_free (list, true);
                
                soup_websocket_connection_send_text (client, json_string);
                if  (get_debug_level () > 0 || dbg)
                        g_print ("%s\n",json_string);
                g_free (json_string);
        }
}
