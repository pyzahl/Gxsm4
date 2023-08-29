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

#ifndef __RPSPMC_JSON_TALK_H
#define __RPSPMC_JSON_TALK_H

#include <config.h>
#include "../control/jsmn.h"
#include <libsoup/soup.h>

#include <cstdio>
#include <iosfwd>
#include <iostream>
#include <vector>
#include <zconf.h>
#include <zlib.h>
#include <iomanip>
#include <cassert>

#include "rpspmc_hwi_structs.h"
#include "rpspmc_pacpll_json_data.h""

// forward defs
extern PACPLL_parameters pacpll_parameters;
extern PACPLL_signals pacpll_signals;
extern SPMC_parameters spmc_parameters;
extern SPMC_signals spmc_signals;
extern JSON_parameter PACPLL_JSON_parameters[];
extern JSON_signal PACPLL_JSON_signals[];


class RP_JSON_talk{
public:
        RP_JSON_talk (){
                /* create a new connection, init */

                listener=NULL;
                port=9002;

                session=NULL;
                msg=NULL;
                client=NULL;
                client_error=NULL;
                error=NULL;
        };
        ~RP_JSON_talk (){};
        void json_talk_connect_cb (gboolean connect); // connect/dissconnect
        static void got_client_connection (GObject *object, GAsyncResult *result, gpointer user_data);
        static void on_message(SoupWebsocketConnection *ws,
                               SoupWebsocketDataType type,
                               GBytes *message,
                               gpointer user_data);
        static void on_closed (SoupWebsocketConnection *ws, gpointer user_data);
        
        void write_parameter (const gchar *parameter_id, double value, const gchar *fmt=NULL, gboolean dbg=FALSE);
        void write_parameter (const gchar *parameter_id, int value, gboolean dbg=FALSE);

        void write_array (const gchar *parameter_id[], int i_size, int *i_vec, int d_size, double *d_vec); // *** custom array set 

        void write_signal (const gchar *parameter_id, int size, double *value, const gchar *fmt=NULL, gboolean dbg=FALSE);
        void write_signal (const gchar *parameter_id, int size, int *value, gboolean dbg=FALSE);

        virtual const gchar *get_rp_address (){ return NULL; };
        virtual int get_debug_level() { return 0; };

        virtual void on_new_data (){
                g_message ("New data for displap or review!");
        };
        
        virtual void status_append (const gchar *msg){
                g_message (msg);
        };
        virtual void update_health (const gchar *msg=NULL){
                g_message (msg);
        };
        virtual void on_connect_actions (){}; // called on connect, setup instrument, send all parameters, etc
        
        void debug_log (const gchar *msg){
                if (get_debug_level () > 4)
                        g_message ("%s", msg);
                if (get_debug_level () > 2){
                        status_append (msg);
                        status_append ("\n");
                }
        };
        
        static int json_dump(const char *js, jsmntok_t *t, size_t count, int indent) {
                int i, j, k;
                if (count == 0) {
                        return 0;
                }
                if (t->type == JSMN_PRIMITIVE) {
                        g_print("%.*s", t->end - t->start, js+t->start);
                        return 1;
                } else if (t->type == JSMN_STRING) {
                        g_print("'%.*s'", t->end - t->start, js+t->start);
                        return 1;
                } else if (t->type == JSMN_OBJECT) {
                        g_print("\n");
                        j = 0;
                        for (i = 0; i < t->size; i++) {
                                for (k = 0; k < indent; k++) g_print("  ");
                                j += json_dump(js, t+1+j, count-j, indent+1);
                                g_print(": ");
                                j += json_dump(js, t+1+j, count-j, indent+1);
                                g_print("\n");
                        }
                        return j+1;
                } else if (t->type == JSMN_ARRAY) {
                        j = 0;
                        g_print("\n");
                        for (k = 0; k < indent-1; k++) g_print("  ");
                        g_print("[");
                        for (i = 0; i < t->size; i++) {
                                j += json_dump(js, t+1+j, count-j, indent+1);
                                g_print(", ");
                        }
                        g_print("]\n");
                        return j+1;
                }
                return 0;
        };

        static JSON_parameter * check_parameter (const char *string, int len){
                //g_print ("[[check_parameter=%s]]", string);
                for (JSON_parameter *p=PACPLL_JSON_parameters; p->js_varname; ++p)
                        if (strncmp (string, p->js_varname, len) == 0)
                                return p;
                return NULL;
        };
        static JSON_signal * check_signal (const char *string, int len){
                for (JSON_signal *p=PACPLL_JSON_signals; p->js_varname; ++p){
                        //g_print ("[[check_signal[%d]=%.*s]=?=%s]", len, len, string,p->js_varname);
                        if (strncmp (string, p->js_varname, len) == 0)
                                return p;
                }
                return NULL;
        };
        static void dump_parameters (int dbg=0){
                for (JSON_parameter *p=PACPLL_JSON_parameters; p->js_varname; ++p)
                        g_print ("%s=%g\n", p->js_varname, *(p->value));
                if (dbg > 4)
                        for (JSON_signal *p=PACPLL_JSON_signals; p->js_varname; ++p){
                                g_print ("%s=[", p->js_varname);
                                for (int i=0; i<p->size; ++i)
                                        g_print ("%g, ", p->value[i]);
                                g_print ("]\n");
                        }
        };
        static int json_fetch(const char *js, jsmntok_t *t, size_t count, int indent){
                static JSON_parameter *jp=NULL;
                static JSON_signal *jps=NULL;
                static gboolean store_next=false;
                static int array_index=0;
                int i, j, k;
                if (count == 0) {
                        return 0;
                }
                if (indent == 0){
                        jp=NULL;
                        store_next=false;
                }
                if (t->type == JSMN_PRIMITIVE) {
                        //g_print("%.*s", t->end - t->start, js+t->start);
                        if (store_next){
                                if (jp){
                                        *(jp->value) = atof (js+t->start);
                                        //g_print ("ATOF[%.*s]=%g %14.8f\n", 20, js+t->start, *(jp->value), *(jp->value));
                                        jp=NULL;
                                        store_next = false;
                                } else if (jps){
                                        jps->value[array_index++] = atof (js+t->start);
                                        if (array_index >= jps->size){
                                                jps=NULL;
                                                store_next = false;
                                        }
                                }
                        }
                        return 1;
                } else if (t->type == JSMN_STRING) {
                        if (indent == 2){
                                jps=NULL;
                                jp=check_parameter ( js+t->start, t->end - t->start);
                                if (!jp){
                                        jps=check_signal ( js+t->start, t->end - t->start);
                                        if (jps) array_index=0;
                                }
                        }
                        if (indent == 3) if (strncmp (js+t->start, "value", t->end - t->start) == 0 && (jp || jps)) store_next=true;
                        //g_print("S[%d] '%.*s' [%s]", indent, t->end - t->start, js+t->start, jp || jps?"ok":"?");
                        return 1;
                } else if (t->type == JSMN_OBJECT) {
                        //g_print("\n O\n");
                        j = 0;
                        for (i = 0; i < t->size; i++) {
                                //for (k = 0; k < indent; k++) g_print("  ");
                                j += json_fetch(js, t+1+j, count-j, indent+1);
                                //g_print(": ");
                                j += json_fetch(js, t+1+j, count-j, indent+1);
                                //g_print("\n");
                        }
                        return j+1;
                } else if (t->type == JSMN_ARRAY) {
                        j = 0;
                        //g_print("\n A ");
                        //for (k = 0; k < indent-1; k++) g_print("  ");
                        //g_print("[");
                        for (i = 0; i < t->size; i++) {
                                j += json_fetch(js, t+1+j, count-j, indent+1);
                                //g_print(", ");
                        }
                        //g_print("]");
                        return j+1;
                }
                return 0;
        };

        void json_parse_message (const char *json_string);

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
                                stream << (((data_index + 1) % 16 == 0) ? "\n" : " ");
                        }
                }
                stream << std::endl;
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




        
        /* Socket Connection */
	GSocket *listener;
	gushort port;

	SoupSession *session;
	SoupMessage *msg;
	SoupWebsocketConnection *client;
        GIOStream *JSON_raw_input_stream;
	GError *client_error;
	GError *error;
};


#endif
