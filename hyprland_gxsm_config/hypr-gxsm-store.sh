#!/bin/bash

# A script to generate windowrulev2 lines from hyprctl clients output related to Gxsm4 windows.

## add this to your hyprland.conf and create/install the two new files
#source= hypr-gxsm-static.conf # Gxsm4 tools window geometry and a few more
#source= hypr-gxsm.conf # Gxsm4 window geometry
## GXSM4

#hyprctl clients -j | jq -r '.[] | select((.class | test("gxsm4")) and (.title | test("Multi"))) | "windowrulev2 = move " + (.at[0]|tostring) + " " + (.at[1]|tostring) + ", class:" + .class + ", initialTitle:" + .title'

## declare an array variable
declare -a gxsm_windows=("Gxsm4" "Channel Selector" "RP-SPM Control Window" "RPSPMC PACPLL Control for RedPitaya" "GXSM Activity Monitor and Logbook" "Pan View and OSD" "HUD Probe Indicator" "SPM Scan Control" "Multi Dimensional Movie Control" "Python Remote Control Console")

## loop through above array
for title_key in "${gxsm_windows[@]}"
do
	window_id=`hyprctl clients -j | jq -r '.[] | select((.class | test("gxsm4")) and (.title | test("'"$title_key"'"))) | "class:" + .class + ", initialTitle:" + .title'`
	move=`hyprctl clients -j | jq -r '.[] | select((.class | test("gxsm4")) and (.title | test("'"$title_key"'"))) | "move " + (.at[0]|tostring) + " " + (.at[1]|	tostring)'`
	size=`hyprctl clients -j | jq -r '.[] | select((.class | test("gxsm4")) and (.title | test("'"$title_key"'"))) | "size " + (.size[0]|tostring) + " " + (.size[1]|tostring)'`
 
 	echo '# default geometry for GXSM4 WINDOW with title:' $title_key
	echo windowrulev2 = 'float, '$window_id
	echo windowrulev2 = $size', '$window_id
	echo windowrulev2 = $move', '$window_id
	echo

done

