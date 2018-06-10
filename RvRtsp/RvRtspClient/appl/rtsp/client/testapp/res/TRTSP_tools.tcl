##############################################################################################
#
# Notice:
# This document contains information that is proprietary to RADVISION LTD..
# No part of this publication may be reproduced in any form whatsoever without
# written prior approval by RADVISION LTD..
#
# RADVISION LTD. reserves the right to revise this publication and make changes
# without obligation to notify any person of such revisions or changes.
#
##############################################################################################

##############################################################################################
#                                 TRTSP_tools.tcl
#
#   Tools menu handling.
#   This file holds all the GUI procedures for the Tools menu. This includes the log window,
#   resources status information and the default options of the stack.
#
#
#
##############################################################################################



# options:chooseEditor
# Browse filelist to select an external editor program
# input  : type - Type of file being edited
# output : none
# return : none
proc options:chooseEditor {type} {
    global app tmp

    set ed [tk_getOpenFile -filetypes {{ "Programs" {*.exe} }} -title "Select editor"]

    if {($ed != "") && [file exists $ed]} {
        set app(editor,$type) $ed
        $tmp(optionTab).external.${type}Ed xview moveto 1
    }
}

##############################################################################################
#
#   CONFIG operations (Open config.val)
#
##############################################################################################


# config:open
# opens the config.val file to be edited buy the most common text editor in either
# windows (notepad) or unix (xemacs).
# Called from the options menu.
# input  : none
# output : none
# return : none
proc config:open {} {
    global tmp app

    exec $app(editor,config) $tmp(configFilename) > nul &

    test:activateTool config
    after 150 test:deactivateTool config
}



##############################################################################################
#
#   Log File operations (Open the current logfile)
#
##############################################################################################


# logfile:open
# opens the stacklog.txt file to be edited buy notepad. for now, does nothing under Unix
# Called from the ToolBar.
# input  : none
# output : none
# return : none
proc logfile:open {} {
    global tmp app tcl_platform

    if {$tcl_platform(platform) != "unix"} {
        exec $app(editor,log) "rtspClientLog.txt" < $app(editor,log) > nul &
    } else {
        exec $app(editor,log) "rtspClientLog.txt" &
    }

    test:activateTool logFile
    after 150 test:deactivateTool logFile
}


##############################################################################################
#
#   NOTEBOOK operations
#
##############################################################################################


# notebook:create
# Create a notebook-like widget
# input  : var  - Variable group to use
#          base - Base container widget to use for the notebook created
#          tabs - List of tabs to use in notebook
#                 Each tab is a list of 2 elements:
#                 name  - String to use on tab's name
#                 cb    - Callback function that draws the tab's contents (cb <base_frame> <tab_button>)
# output : none
# return : name of the created notebook "widget"
proc notebook:create {var base tabs} {
    global tmp app

    set base $base.canv
    set numTabs [llength $tabs]
    set i 1

    canvas $base -borderwidth 0 -relief flat -width 1 -height 1

    foreach tab $tabs {
        set tabName [lindex $tab 0]
        set tabCallback [lindex $tab 1]

        # Create a tab header
        button $base.but$i -borderwidth 0 -relief flat -text $tabName -width 0 -padx 1 -pady 2 \
            -command "notebook:changeSelection $var $i"

        # Create a frame for each tab
        frame $base.tab$i -borderwidth 0 -relief flat -height 1 -width 1 -takefocus 0
        eval {$tabCallback $base.tab$i $base.but$i}

        # Grid the button for tab
        grid $base.but$i -in $base -column $i -row 1 -padx 5

        # Make sure the tab frame knows where it should be
        grid $base.tab$i -in $base -column 0 -row 2 -columnspan [expr $numTabs+2] \
            -sticky news -padx 10 -pady 2

        incr i
    }

    # Set some global grid information for the notebook's canvas
    grid rowconf $base 0 -minsize 6
    grid rowconf $base 2 -weight 1
    grid rowconf $base 3 -minsize 4
    grid columnconf $base 0 -minsize 10
    grid columnconf $base [expr $numTabs+1] -weight 1 -minsize 20

    # Raise the current tab inside canvas
    raise $base.tab$app($var,currTab)

    # Bind expose event to make sure we draw the tabs correct
    bind $base <Expose> "
        $base delete border
        $base delete tabs
        notebook:border $var
        notebook:draw $var
    "

    return $base
}



# notebook:changeSelection
# Change the selection between the notebook tabs inside the canvas
# input  : top  - Top level window name (without the point)
#          new  - New tab to use (number of tab to select)
# output : none
# return : none
proc notebook:changeSelection {top new} {
    global tmp app

    # Change the frame inside the options window
    raise .$top.canv.tab$new

    # Make sure we know which frame we're in
    set app($top,currTab) $new
    .$top.canv delete tabs
    notebook:draw $top
}


# notebook:border
# Draw the border lines of the notebook
# input  : top  - Top level window name (without the point)
# output : none
# return : none
proc notebook:border {top} {
    global tmp app

    set canv .$top.canv

    #################
    # CANVAS DRAWING
    #################
    set Left [expr { [winfo x $canv.tab$app($top,currTab)] - 2 }]
    set Right [expr { [winfo x $canv.tab$app($top,currTab)] + [winfo width $canv.tab$app($top,currTab)] + 2 }]
    set Top [expr { [winfo y $canv.tab$app($top,currTab)] - 2 }]
    set Bottom [expr { [winfo y $canv.tab$app($top,currTab)] + [winfo height $canv.tab$app($top,currTab)] + 2 }]
    # top - left sides line
    $canv create line $Left $Bottom $Left $Top $Right $Top        -fill white -tags border
    # bottom - right sides line
    $canv create line $Right $Top $Right $Bottom $Left $Bottom    -fill black -tags border
    # bottom - right sides line, one inside
    incr Left
    incr Right -1
    incr Bottom -1
    incr Top
    $canv create line $Right $Top $Right $Bottom $Left $Bottom    -fill DarkGrey -tags border
}


# notebook:draw
# Draw the notebook when necessary
# input  : top  - Top level window name (without the point)
# output : none
# return : none
proc notebook:draw {top} {
    global tmp app

    set canv .$top.canv

    set Left [expr { [winfo x $canv.but$app($top,currTab)] - 4 }]
    set Right [expr { [winfo x $canv.but$app($top,currTab)] + [winfo width $canv.but$app($top,currTab)] + 4 }]
    set Top [expr { [winfo y $canv.but$app($top,currTab)] - 4 }]
    set Bottom [expr { [winfo y $canv.tab$app($top,currTab)] - 2 }]
    set canvFace [$canv cget -background]
    $canv create line $Left $Bottom $Right $Bottom -fill $canvFace -tags tabs
    $canv create line $Left $Bottom $Left $Top  $Right $Top -fill White -tags tabs
    $canv create line $Right $Top $Right $Bottom -fill Black -tags tabs
    incr Right -1
    incr Top
    $canv create line $Right $Top $Right $Bottom -fill DarkGrey -tags tabs
    foreach tab $tmp($top,otherTabs) {
        if [expr {$tab != $app($top,currTab)}] {
            set Left [expr { [winfo x $canv.but$tab] - 2 }]
            set Right [expr { [winfo x $canv.but$tab] + [winfo width $canv.but$tab] + 2 }]
            set Top [expr { [winfo y $canv.but$tab] - 2 }]
            $canv create line $Left $Bottom $Left $Top  $Right $Top -fill White -tags tabs
            $canv create line $Right $Top $Right $Bottom -fill Black -tags tabs
            incr Right -1
            incr Top
            $canv create line $Right $Top $Right $Bottom -fill DarkGrey -tags tabs
        }
    }
}


proc notebook:refresh {top} {
    set base .$top.canv
    $base delete border
    $base delete tabs
    notebook:border $top
    notebook:draw $top
}


