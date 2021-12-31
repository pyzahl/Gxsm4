#!/bin/awk
# create gconf schemas from variable listing "schema_raw.list"
# run:
# awk -f create_schema.awk schema_raw.list > gxsm.schemas
# install:
# gconftool --install-schema-file=gxsm.schema

BEGIN {
  print "<?xml version=\"1.0\"?>";
  print "<gconfschemafile>";
  print "  <schemalist>";
  dir = "State/";
}
{
  key = $1;
  type = $3;
  split ($0,A,/[\"]/);
  default = A[2];
  shorthelp = A[4];
  longhelp = A[6];
  if (shorthelp == "") shorthelp = "NA";
  if (longhelp == "") longhelp = "sorry not available yet";
  longhelp = longhelp ", Gxsm var name hint: " $2;
  print   "    <schema>";
  printf ("      <applyto>/apps/gxsm/%s%s</applyto>\n", dir, key);
  printf ("      <key>/schemas/apps/gxsm/%s%s</key>\n", dir, key);
  print   "      <owner>gxsm</owner>";
  printf ("      <type>%s</type>\n", type);
  print   "      <locale name=\"C\">";
  printf ("        <default>%s</default>\n", default);
  printf ("        <short>%s</short>\n", shorthelp);
  printf ("        <long>%s</long>\n", longhelp);
  print   "      </locale>";
  print   "    </schema>";
}
END {
  print "  </schemalist>"
    print "</gconfschemafile>"
  }
