/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: spm_scancontrol.h
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

#ifndef __RPSPMC_STREAM_H
#define __RPSPMC_STREAM_H

#define USE_WEBSOCKETPP // libsoup otherwise


#include <config.h>
#include "../control/jsmn.h"


#include <cstdio>
#include <iosfwd>
#include <iostream>
#include <vector>
#include <zconf.h>
#include <zlib.h>
#include <iomanip>
#include <cassert>

#include "rpspmc_hwi_structs.h"


#ifdef USE_WEBSOCKETPP
# include <websocketpp/config/asio_no_tls_client.hpp>
# include <websocketpp/client.hpp>
# include <iostream>
#else
# include <libsoup/soup.h>
#endif




#ifdef USE_WEBSOCKETPP
        typedef websocketpp::client<websocketpp::config::asio_client> wsppclient;
        typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
#endif


class RP_stream{
public:
        RP_stream (){
                /* create a new connection, init */

                listener=NULL;
                port=9003;

#ifdef USE_WEBSOCKETPP
                client = new wsppclient;

                // Set logging to be pretty verbose (everything except message payloads)
                client->set_access_channels(websocketpp::log::alevel::all);
                client->clear_access_channels(websocketpp::log::alevel::frame_payload);
                client->set_error_channels(websocketpp::log::elevel::all);
                
                // Initialize ASIO
                client->init_asio();
                //client->start_perpetual(); //*
                //client->reset(new websocketpp::lib::thread(&wsppclient::run, &client)); //*
                
                // Register our message handler
                //client->set_message_handler(&on_message);

                // Set the on_message handler with a lambda that captures the user data
                RP_stream *a=this;
                client->set_message_handler([a](websocketpp::connection_hdl hdl, wsppclient::message_ptr msg) {
                        on_message(a, hdl, msg);
                });
    
                con=NULL;
#else                
                session=NULL;
                msg=NULL;
                client=NULL;
#endif
                client_error=NULL;
                error=NULL;
                last_vector_pc_confirmed = -1;
        };
        ~RP_stream (){};
        void stream_connect_cb (gboolean connect); // connect/dissconnect

#ifdef USE_WEBSOCKETPP
        static void got_client_connection (GObject *object, gpointer user_data);
        static void on_message(RP_stream* self, websocketpp::connection_hdl hdl, wsppclient::message_ptr msg);
        static void on_closed (GObject *object, gpointer user_data);
#else
        static void got_client_connection (GObject *object, GAsyncResult *result, gpointer user_data);
        static void on_message(SoupWebsocketConnection *ws,
                               SoupWebsocketDataType type,
                               GBytes *message,
                               gpointer user_data);
        static void on_closed (SoupWebsocketConnection *ws, gpointer user_data);
#endif

        virtual const gchar *get_rp_address (){ return NULL; };
        virtual int get_debug_level() { return 0; };

        virtual int on_new_data (gconstpointer contents, gsize len, int position, int new_count=1, bool last=false) {};
        
        virtual void status_append (const gchar *msg, bool schedule_from_thread=false){
                g_message (msg);
        };
        virtual void update_health (const gchar *msg=NULL){
                g_message (msg);
        };
        virtual void on_connect_actions (){}; // called on connect, setup instrument, send all parameters, etc

        void resetVPCconfirmed (){
                last_vector_pc_confirmed = -1;
        };
        int getVPCconfirmed (){
                return last_vector_pc_confirmed;
        };
        
        void debug_log (const gchar *msg){
                if (get_debug_level () > 4)
                        g_message ("%s", msg);
                if (get_debug_level () > 2){
                        if (msg){
                                status_append (msg);
                                status_append ("\n");
                        }
                }
        };
        

        // Compression utils
        
        void add_buffer_to_vector(std::vector<char> &vector, const char *buffer, uLongf length) {
                for (int character_index = 0; character_index < length; character_index++) {
                        char current_character = buffer[character_index];
                        vector.push_back(current_character);
                }
        };

        int compress_vector(std::vector<char> source, std::vector<char> &destination) {
                unsigned long source_length = source.size();
                uLongf destination_length = compressBound(source_length);

                char *destination_data = (char *) malloc(destination_length);
                if (destination_data == nullptr) {
                        return Z_MEM_ERROR;
                }

                Bytef *source_data = (Bytef *) source.data();
                int return_value = compress2((Bytef *) destination_data, &destination_length, source_data, source_length,
                                             Z_BEST_COMPRESSION);
                add_buffer_to_vector(destination, destination_data, destination_length);
                free(destination_data);
                return return_value;
        };

        int decompress_vector(std::vector<char> source, std::vector<char> &destination) {
                unsigned long source_length = source.size();
                uLongf destination_length = compressBound(source_length);

                char *destination_data = (char *) malloc(destination_length);
                if (destination_data == nullptr) {
                        return Z_MEM_ERROR;
                }

                Bytef *source_data = (Bytef *) source.data();
                int return_value = uncompress((Bytef *) destination_data, &destination_length, source_data, source.size());
                add_buffer_to_vector(destination, destination_data, destination_length);
                free(destination_data);
                return return_value;
        };

        void add_string_to_vector(std::vector<char> &uncompressed_data,
                                  const char *my_string) {
                int character_index = 0;
                while (true) {
                        char current_character = my_string[character_index];
                        uncompressed_data.push_back(current_character);

                        if (current_character == '\00') {
                                break;
                        }
                       character_index++;
                }
        };

        // https://stackoverflow.com/a/27173017/3764804
        void print_bytes(std::ostream &stream, const unsigned char *data, size_t data_length, bool format = true) {
                stream << std::setfill('0');
                for (size_t data_index = 0; data_index < data_length; ++data_index) {
                        stream << std::hex << std::setw(2) << (int) data[data_index];
                        if (format) {
                                stream << (((data_index + 1) % 32 == 0) ? "\n" : ((data_index + 1) % 4 == 0) ? "  " : " ");
                        }
                }
                stream << std::endl;
        };

        void status_appends_bytes(const unsigned char *data, size_t data_length, bool format = true) {
                if (data_length < 1)
                        return;
                std::ostringstream stream;
                stream << std::setfill('0');
                for (size_t data_index = 0; data_index < data_length; ++data_index) {
                        stream << std::hex << std::setw(2) << (int) data[data_index];
                        if (format) {
                                stream << (((data_index + 1) % 32 == 0) ? "\n" : ((data_index + 1) % 4 == 0) ? "  " : " ");
                        }
                }
                stream << std::endl;
                std::string str =  stream.str();
                status_append (str.c_str());
        };

#define SPMC_AD5791_REFV 5.0 // DAC AD5791 Reference Volatge is 5.000000V (+/-5V Range)
        double rpspmc_to_volts (int value){ return SPMC_AD5791_REFV*(double)value / ((1<<31)-1); }

        void status_append_int32(const guint32 *data, size_t data_length, bool format = true, int offset=0, bool also_gprint = false) {
                if (data_length < 1)
                        return;
                std::ostringstream stream;
                stream << std::setfill('0');

                int wpl=16;
                for (size_t data_index = 0; data_index < data_length; ++data_index) {
                        if (data_index % wpl == 0)
                                stream << std::hex << std::setw(8) << (data_index+offset) << ": ";
                        stream << std::hex << std::setw(8) << data[data_index];
                        if (format) {
                                stream << (((data_index + 1) % wpl == 0) ? "\n": " ");
                        }
                }
                stream << std::endl;

                std::string str =  stream.str();

                if (also_gprint){
                        g_print (str.c_str());
                        status_append (str.c_str(), true); // MUST SCHEDULE -- NOT THREAD SAFE
                } else
                        status_append (str.c_str()); // WARNING -- NOT THREAD SAFE
                
        };

        void test_compression() {
                std::vector<char> uncompressed(0);
                auto *my_string = (char *) "Hello, world!";
                add_string_to_vector(uncompressed, my_string);

                std::vector<char> compressed(0);
                int compression_result = compress_vector(uncompressed, compressed);
                assert(compression_result == F_OK);

                std::vector<char> decompressed(0);
                int decompression_result = decompress_vector(compressed, decompressed);
                assert(decompression_result == F_OK);

                printf("Uncompressed: %s\n", uncompressed.data());
                printf("Compressed: ");
                std::ostream &standard_output = std::cout;
                print_bytes(standard_output, (const unsigned char *) compressed.data(), compressed.size(), false);
                printf("Decompressed: %s\n", decompressed.data());
        };

        int last_vector_pc_confirmed;

        
        /* Socket Connection */
	GSocket *listener;
	gushort port;

#ifdef USE_WEBSOCKETPP
        wsppclient *client;
        wsppclient::connection_ptr con;
        websocketpp::lib::thread asio_thread;
#else
	SoupSession *session;
	SoupSocket *socket;
	SoupMessage *msg;
	SoupWebsocketConnection *client;
#endif
        GIOStream *JSON_raw_input_stream;
	GError *client_error;
	GError *error;
        
};


#endif
