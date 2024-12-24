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
#include <zlib.h>

#include "config.h"

#include "rpspmc_pacpll.h"

extern SPMC_parameters spmc_parameters;

/* *** RP_stream module *** */

void RP_stream::status_append (const gchar *msg, bool schedule_from_thread=false){
        rpspmc->status_append (msg, schedule_from_thread);
}

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
                
#ifdef USE_WEBSOCKETPP
                wspp_asio_gthread = g_thread_new ("wspp_asio_gthread", wspp_asio_thread, this);
#else
                // new soup session
                session = soup_session_new ();

                // then connect to Stream Socket on RP
                gchar *url = g_strdup_printf ("ws://%s:%u", get_rp_address (), port);
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
#endif
        } else {
#ifdef USE_WEBSOCKETPP
                if (con){
                        status_append ("Dissconnecting...\n ", true);
                        std::error_code ec;
                        client->close(con, websocketpp::close::status::normal, "", ec);
                        //client->stop();
                        // close(websocketpp::connection_hdl, websocketpp::close::status::value, const std::string&, std::error_code&)
                        
                        update_health ("Dissconnected RP stream.\n");
                        con = NULL;
                }
#else
                // tear down connection
                status_append ("Dissconnecting...\n ", true);

                //g_clear_object (&listener);
                g_clear_object (&client);
                g_clear_error (&client_error);
                g_clear_error (&error);

                update_health ("Dissconnected.\n");
#endif
        }
}


#ifdef USE_WEBSOCKETPP
#if 0
//https://docs.websocketpp.org/md_tutorials_utility_client_utility_client.html
void RP_stream::on_close(client * c, websocketpp::connection_hdl hdl) {
    m_status = "Closed";
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    std::stringstream s;
    s << "close code: " << con->get_remote_close_code() << " (" 
      << websocketpp::close::status::get_string(con->get_remote_close_code()) 
      << "), close reason: " << con->get_remote_close_reason();
    m_error_reason = s.str();
}

void RP_stream::close(int id, websocketpp::close::status::value code) {
    websocketpp::lib::error_code ec;
    
    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
        std::cout << "> No connection found with id " << id << std::endl;
        return;
    }
    
    m_endpoint.close(metadata_it->second->get_hdl(), code, "", ec);
    if (ec) {
        std::cout << "> Error initiating close: " << ec.message() << std::endl;
    }
}

#endif

void  RP_stream::got_client_connection (GObject *object, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        g_message ("got_client_connection");
}
#else
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
        }
}
#endif

#ifdef USE_WEBSOCKETPP
void  RP_stream::on_message(RP_stream* self, websocketpp::connection_hdl hdl, wsppclient::message_ptr msg)
#else
void  RP_stream::on_message(SoupWebsocketConnection *ws,
                               SoupWebsocketDataType type,
                               GBytes *message,
                               gpointer user_data)
#endif
{
	gconstpointer contents;
	gsize len;
        gchar *tmp;

        static int position=0;
        static int count=0;
        static bool finished=false;

        static size_t bram_offset=0;

        //self->debug_log ("WebSocket SPMC message received.");
	//self->status_append ("WebSocket SPMC message received.\n", true);

#ifdef USE_WEBSOCKETPP
        //RP_stream *self = (RP_stream*)(*c)->get_user_data(hdl);

        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                contents = msg->get_payload().c_str();
                len = msg->get_payload().size();
#else 
        RP_stream *self = ( RP_stream *)user_data;
	if (type == SOUP_WEBSOCKET_DATA_TEXT) {
		contents = g_bytes_get_data (message, &len);
#endif
                gchar *p;
                /*
                if (contents && len > 0){ //< 100){
                        tmp = g_strdup_printf ("** WS TEXT MESSAGE: %s", contents);
                        self->status_append (tmp, true);
                        g_message (tmp);
                        g_free (tmp);
                }
                */
                /* else {
                        self->status_append ("WEBSOCKET_DATA_TEXT ------\n", true);
                        if (contents && len > 0)
                                self->status_append ((gchar*)contents, true);
                        else
                                self->status_append ("Empty text message received.", true);
                        if (g_strrstr (contents, "\n"))
                                self->status_append ("--------------------------\n", true);
                        else
                                self->status_append ("\n--------------------------\n", true);
                }
                  */
                //g_message ("WS Message: %s", (gchar*)contents);

                if (g_strrstr (contents, "RESET")){
                        self->status_append ("** WEBSOCKET STREAM TAG: RESET (GVP Start) Received.\n", true);
                        g_message ("** WEBSOCKET STREAM TAG: RESET (GVP Start) Received.");
                        finished=false;
                        position=0;
                        count = 0;
                        // TESTing:
                        self->on_new_data (NULL, 0, true);
                }
                
                if ((p=g_strrstr(contents, "Position:{0x"))){ // SIMPLE JSON BLOCK
                        position = strtoul (p+10, NULL, 16); // effin addr hacks pactches!!!
                        if ((p=g_strrstr (contents, "Count:{")))
                                count = atoi (p+7);
                        //g_message("*** ==> pos: 0x%06x #%d", position, count);
                        //{ gchar *tmp; self->status_append (tmp=g_strdup_printf("*** ==> pos: 0x%06x #%d\n", position, count), true); g_free(tmp); }
                }

                if (g_strrstr (contents, "FinishedNext:{true}")){
                        self->status_append ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received\n", true);
                        g_message ("** WEBSOCKET STREAM TAG: FINISHED NEXT (GVP completed, last package update is been send) Received");
                        finished=true;
                }
                
                if ((p=g_strrstr(contents, "BRAMoffset:{0x"))){ // SIMPLE JSON BLOCK
                        bram_offset = strtoul(p+12, NULL, 16);
                }
                
                if (g_strrstr (contents, "vector = {")){
                        if ((p = g_strrstr (contents, "// Vector #"))){
                                self->last_vector_pc_confirmed = atoi (p+11);
                                g_message ("** VECTOR #%02d confirmed.", self->last_vector_pc_confirmed);
                        }
                }
               
#ifdef USE_WEBSOCKETPP
        } else {
                contents = msg->get_payload().c_str();
                len = msg->get_payload().size();
#else
	} else if (type == SOUP_WEBSOCKET_DATA_BINARY) {
		contents = g_bytes_get_data (message, &len);
#endif

                self->on_new_data (contents, len);
        }
}

#ifdef USE_WEBSOCKETPP
void  RP_stream::on_closed (GObject *object, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        self->status_append ("WebSocket stream connection externally closed.\n", true);
        self->status_append ("--> auto reconnecting...\n", true);
        self->stream_connect_cb (TRUE);
}
#else
void  RP_stream::on_closed (SoupWebsocketConnection *ws, gpointer user_data){
        RP_stream *self = ( RP_stream *)user_data;
        self->status_append ("WebSocket stream connection externally closed.\n", true);
        self->status_append ("--> auto reconnecting...\n", true);
        self->stream_connect_cb (TRUE);
}
#endif




