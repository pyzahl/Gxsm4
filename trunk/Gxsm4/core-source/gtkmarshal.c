
#include	<glib-object.h>


#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     g_value_get_char (v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
#define g_marshal_value_peek_long(v)     g_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
#define g_marshal_value_peek_float(v)    g_value_get_float (v)
#define g_marshal_value_peek_double(v)   g_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#define g_marshal_value_peek_param(v)    g_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_int
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */


/* BOOL:NONE (./gtkmarshal.list:1) */
void
gtk_marshal_BOOLEAN__VOID (GClosure     *closure,
                           GValue       *return_value,
                           guint         n_param_values,
                           const GValue *param_values,
                           gpointer      invocation_hint,
                           gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__VOID) (gpointer     data1,
                                                  gpointer     data2);
  register GMarshalFunc_BOOLEAN__VOID callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOL:POINTER (./gtkmarshal.list:2) */
void
gtk_marshal_BOOLEAN__POINTER (GClosure     *closure,
                              GValue       *return_value,
                              guint         n_param_values,
                              const GValue *param_values,
                              gpointer      invocation_hint,
                              gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER) (gpointer     data1,
                                                     gpointer     arg_1,
                                                     gpointer     data2);
  register GMarshalFunc_BOOLEAN__POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOL:POINTER,POINTER,INT,INT (./gtkmarshal.list:3) */
void
gtk_marshal_BOOLEAN__POINTER_POINTER_INT_INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER_POINTER_INT_INT) (gpointer     data1,
                                                                     gpointer     arg_1,
                                                                     gpointer     arg_2,
                                                                     gint         arg_3,
                                                                     gint         arg_4,
                                                                     gpointer     data2);
  register GMarshalFunc_BOOLEAN__POINTER_POINTER_INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER_POINTER_INT_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       g_marshal_value_peek_pointer (param_values + 2),
                       g_marshal_value_peek_int (param_values + 3),
                       g_marshal_value_peek_int (param_values + 4),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOL:POINTER,INT,INT (./gtkmarshal.list:4) */
void
gtk_marshal_BOOLEAN__POINTER_INT_INT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER_INT_INT) (gpointer     data1,
                                                             gpointer     arg_1,
                                                             gint         arg_2,
                                                             gint         arg_3,
                                                             gpointer     data2);
  register GMarshalFunc_BOOLEAN__POINTER_INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER_INT_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       g_marshal_value_peek_int (param_values + 2),
                       g_marshal_value_peek_int (param_values + 3),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOL:POINTER,INT,INT,UINT (./gtkmarshal.list:5) */
void
gtk_marshal_BOOLEAN__POINTER_INT_INT_UINT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER_INT_INT_UINT) (gpointer     data1,
                                                                  gpointer     arg_1,
                                                                  gint         arg_2,
                                                                  gint         arg_3,
                                                                  guint        arg_4,
                                                                  gpointer     data2);
  register GMarshalFunc_BOOLEAN__POINTER_INT_INT_UINT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER_INT_INT_UINT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       g_marshal_value_peek_int (param_values + 2),
                       g_marshal_value_peek_int (param_values + 3),
                       g_marshal_value_peek_uint (param_values + 4),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOL:POINTER,STRING,STRING,POINTER (./gtkmarshal.list:6) */
void
gtk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER_STRING_STRING_POINTER) (gpointer     data1,
                                                                           gpointer     arg_1,
                                                                           gpointer     arg_2,
                                                                           gpointer     arg_3,
                                                                           gpointer     arg_4,
                                                                           gpointer     data2);
  register GMarshalFunc_BOOLEAN__POINTER_STRING_STRING_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER_STRING_STRING_POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       g_marshal_value_peek_string (param_values + 2),
                       g_marshal_value_peek_string (param_values + 3),
                       g_marshal_value_peek_pointer (param_values + 4),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* ENUM:ENUM (./gtkmarshal.list:7) */
void
gtk_marshal_ENUM__ENUM (GClosure     *closure,
                        GValue       *return_value,
                        guint         n_param_values,
                        const GValue *param_values,
                        gpointer      invocation_hint,
                        gpointer      marshal_data)
{
  typedef gint (*GMarshalFunc_ENUM__ENUM) (gpointer     data1,
                                           gint         arg_1,
                                           gpointer     data2);
  register GMarshalFunc_ENUM__ENUM callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_ENUM__ENUM) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_enum (param_values + 1),
                       data2);

  g_value_set_enum (return_value, v_return);
}

/* INT:POINTER (./gtkmarshal.list:8) */
void
gtk_marshal_INT__POINTER (GClosure     *closure,
                          GValue       *return_value,
                          guint         n_param_values,
                          const GValue *param_values,
                          gpointer      invocation_hint,
                          gpointer      marshal_data)
{
  typedef gint (*GMarshalFunc_INT__POINTER) (gpointer     data1,
                                             gpointer     arg_1,
                                             gpointer     data2);
  register GMarshalFunc_INT__POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       data2);

  g_value_set_int (return_value, v_return);
}

/* INT:POINTER,CHAR,CHAR (./gtkmarshal.list:9) */
void
gtk_marshal_INT__POINTER_CHAR_CHAR (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data)
{
  typedef gint (*GMarshalFunc_INT__POINTER_CHAR_CHAR) (gpointer     data1,
                                                       gpointer     arg_1,
                                                       gchar        arg_2,
                                                       gchar        arg_3,
                                                       gpointer     data2);
  register GMarshalFunc_INT__POINTER_CHAR_CHAR callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__POINTER_CHAR_CHAR) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       g_marshal_value_peek_char (param_values + 2),
                       g_marshal_value_peek_char (param_values + 3),
                       data2);

  g_value_set_int (return_value, v_return);
}

/* NONE:BOOL (./gtkmarshal.list:10) */

/* NONE:BOXED (./gtkmarshal.list:11) */

/* NONE:ENUM (./gtkmarshal.list:12) */

/* NONE:ENUM,FLOAT (./gtkmarshal.list:13) */
void
gtk_marshal_VOID__ENUM_FLOAT (GClosure     *closure,
                              GValue       *return_value,
                              guint         n_param_values,
                              const GValue *param_values,
                              gpointer      invocation_hint,
                              gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM_FLOAT) (gpointer     data1,
                                                 gint         arg_1,
                                                 gfloat       arg_2,
                                                 gpointer     data2);
  register GMarshalFunc_VOID__ENUM_FLOAT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ENUM_FLOAT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_enum (param_values + 1),
            g_marshal_value_peek_float (param_values + 2),
            data2);
}

/* NONE:ENUM,FLOAT,BOOL (./gtkmarshal.list:14) */
void
gtk_marshal_VOID__ENUM_FLOAT_BOOLEAN (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM_FLOAT_BOOLEAN) (gpointer     data1,
                                                         gint         arg_1,
                                                         gfloat       arg_2,
                                                         gboolean     arg_3,
                                                         gpointer     data2);
  register GMarshalFunc_VOID__ENUM_FLOAT_BOOLEAN callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ENUM_FLOAT_BOOLEAN) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_enum (param_values + 1),
            g_marshal_value_peek_float (param_values + 2),
            g_marshal_value_peek_boolean (param_values + 3),
            data2);
}

/* NONE:INT (./gtkmarshal.list:15) */

/* NONE:INT,INT (./gtkmarshal.list:16) */
void
gtk_marshal_VOID__INT_INT (GClosure     *closure,
                           GValue       *return_value,
                           guint         n_param_values,
                           const GValue *param_values,
                           gpointer      invocation_hint,
                           gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT) (gpointer     data1,
                                              gint         arg_1,
                                              gint         arg_2,
                                              gpointer     data2);
  register GMarshalFunc_VOID__INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            data2);
}

/* NONE:INT,INT,POINTER (./gtkmarshal.list:17) */
void
gtk_marshal_VOID__INT_INT_POINTER (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT_POINTER) (gpointer     data1,
                                                      gint         arg_1,
                                                      gint         arg_2,
                                                      gpointer     arg_3,
                                                      gpointer     data2);
  register GMarshalFunc_VOID__INT_INT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_pointer (param_values + 3),
            data2);
}

/* NONE:NONE (./gtkmarshal.list:18) */

/* NONE:OBJECT (./gtkmarshal.list:19) */

/* NONE:POINTER (./gtkmarshal.list:20) */

/* NONE:POINTER,INT (./gtkmarshal.list:21) */
void
gtk_marshal_VOID__POINTER_INT (GClosure     *closure,
                               GValue       *return_value,
                               guint         n_param_values,
                               const GValue *param_values,
                               gpointer      invocation_hint,
                               gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_INT) (gpointer     data1,
                                                  gpointer     arg_1,
                                                  gint         arg_2,
                                                  gpointer     data2);
  register GMarshalFunc_VOID__POINTER_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            data2);
}

/* NONE:POINTER,POINTER (./gtkmarshal.list:22) */
void
gtk_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_POINTER) (gpointer     data1,
                                                      gpointer     arg_1,
                                                      gpointer     arg_2,
                                                      gpointer     data2);
  register GMarshalFunc_VOID__POINTER_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            data2);
}

/* NONE:POINTER,POINTER,POINTER (./gtkmarshal.list:23) */
void
gtk_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_POINTER_POINTER) (gpointer     data1,
                                                              gpointer     arg_1,
                                                              gpointer     arg_2,
                                                              gpointer     arg_3,
                                                              gpointer     data2);
  register GMarshalFunc_VOID__POINTER_POINTER_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            g_marshal_value_peek_pointer (param_values + 3),
            data2);
}

/* NONE:POINTER,STRING,STRING (./gtkmarshal.list:24) */
void
gtk_marshal_VOID__POINTER_STRING_STRING (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_STRING_STRING) (gpointer     data1,
                                                            gpointer     arg_1,
                                                            gpointer     arg_2,
                                                            gpointer     arg_3,
                                                            gpointer     data2);
  register GMarshalFunc_VOID__POINTER_STRING_STRING callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            data2);
}

/* NONE:POINTER,UINT (./gtkmarshal.list:25) */
void
gtk_marshal_VOID__POINTER_UINT (GClosure     *closure,
                                GValue       *return_value,
                                guint         n_param_values,
                                const GValue *param_values,
                                gpointer      invocation_hint,
                                gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_UINT) (gpointer     data1,
                                                   gpointer     arg_1,
                                                   guint        arg_2,
                                                   gpointer     data2);
  register GMarshalFunc_VOID__POINTER_UINT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            data2);
}

/* NONE:POINTER,UINT,ENUM (./gtkmarshal.list:26) */
void
gtk_marshal_VOID__POINTER_UINT_ENUM (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_UINT_ENUM) (gpointer     data1,
                                                        gpointer     arg_1,
                                                        guint        arg_2,
                                                        gint         arg_3,
                                                        gpointer     data2);
  register GMarshalFunc_VOID__POINTER_UINT_ENUM callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_UINT_ENUM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            g_marshal_value_peek_enum (param_values + 3),
            data2);
}

/* NONE:POINTER,POINTER,UINT,UINT (./gtkmarshal.list:27) */
void
gtk_marshal_VOID__POINTER_POINTER_UINT_UINT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_POINTER_UINT_UINT) (gpointer     data1,
                                                                gpointer     arg_1,
                                                                gpointer     arg_2,
                                                                guint        arg_3,
                                                                guint        arg_4,
                                                                gpointer     data2);
  register GMarshalFunc_VOID__POINTER_POINTER_UINT_UINT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_POINTER_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            g_marshal_value_peek_uint (param_values + 4),
            data2);
}

/* NONE:POINTER,INT,INT,POINTER,UINT,UINT (./gtkmarshal.list:28) */
void
gtk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_INT_INT_POINTER_UINT_UINT) (gpointer     data1,
                                                                        gpointer     arg_1,
                                                                        gint         arg_2,
                                                                        gint         arg_3,
                                                                        gpointer     arg_4,
                                                                        guint        arg_5,
                                                                        guint        arg_6,
                                                                        gpointer     data2);
  register GMarshalFunc_VOID__POINTER_INT_INT_POINTER_UINT_UINT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 7);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_INT_INT_POINTER_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_int (param_values + 3),
            g_marshal_value_peek_pointer (param_values + 4),
            g_marshal_value_peek_uint (param_values + 5),
            g_marshal_value_peek_uint (param_values + 6),
            data2);
}

/* NONE:POINTER,UINT,UINT (./gtkmarshal.list:29) */
void
gtk_marshal_VOID__POINTER_UINT_UINT (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_UINT_UINT) (gpointer     data1,
                                                        gpointer     arg_1,
                                                        guint        arg_2,
                                                        guint        arg_3,
                                                        gpointer     data2);
  register GMarshalFunc_VOID__POINTER_UINT_UINT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            data2);
}

/* NONE:POINTER,UINT,UINT (./gtkmarshal.list:30) */

/* NONE:STRING (./gtkmarshal.list:31) */

/* NONE:STRING,INT,POINTER (./gtkmarshal.list:32) */
void
gtk_marshal_VOID__STRING_INT_POINTER (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT_POINTER) (gpointer     data1,
                                                         gpointer     arg_1,
                                                         gint         arg_2,
                                                         gpointer     arg_3,
                                                         gpointer     data2);
  register GMarshalFunc_VOID__STRING_INT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_pointer (param_values + 3),
            data2);
}

/* NONE:UINT (./gtkmarshal.list:33) */

/* NONE:UINT,POINTER,UINT,ENUM,ENUM,POINTER (./gtkmarshal.list:34) */
void
gtk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER) (gpointer     data1,
                                                                          guint        arg_1,
                                                                          gpointer     arg_2,
                                                                          guint        arg_3,
                                                                          gint         arg_4,
                                                                          gint         arg_5,
                                                                          gpointer     arg_6,
                                                                          gpointer     data2);
  register GMarshalFunc_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 7);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            g_marshal_value_peek_enum (param_values + 4),
            g_marshal_value_peek_enum (param_values + 5),
            g_marshal_value_peek_pointer (param_values + 6),
            data2);
}

/* NONE:UINT,POINTER,UINT,UINT,ENUM (./gtkmarshal.list:35) */
void
gtk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_POINTER_UINT_UINT_ENUM) (gpointer     data1,
                                                                  guint        arg_1,
                                                                  gpointer     arg_2,
                                                                  guint        arg_3,
                                                                  guint        arg_4,
                                                                  gint         arg_5,
                                                                  gpointer     data2);
  register GMarshalFunc_VOID__UINT_POINTER_UINT_UINT_ENUM callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_POINTER_UINT_UINT_ENUM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            g_marshal_value_peek_uint (param_values + 4),
            g_marshal_value_peek_enum (param_values + 5),
            data2);
}

/* NONE:UINT,STRING (./gtkmarshal.list:36) */
void
gtk_marshal_VOID__UINT_STRING (GClosure     *closure,
                               GValue       *return_value,
                               guint         n_param_values,
                               const GValue *param_values,
                               gpointer      invocation_hint,
                               gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_STRING) (gpointer     data1,
                                                  guint        arg_1,
                                                  gpointer     arg_2,
                                                  gpointer     data2);
  register GMarshalFunc_VOID__UINT_STRING callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            data2);
}

