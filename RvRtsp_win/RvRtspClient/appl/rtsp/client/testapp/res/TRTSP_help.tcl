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
#                                 TRTSP_help.tcl
#
#   Help menu handling.
#   This file holds all the GUI procedures for the Help menu. This includes the script help
#   window and the about window.
#
#
##############################################################################################



##############################################################################################
#
#   ABOUT operations
#
##############################################################################################

# about:create
# Creation procedure of the About window
# This window is opened when Help|About is selected from the main menu
# input  : none
# output : none
# return : none
proc about:create {} {
    global tmp app

    if {[winfo exists .about]} {
        wm deiconify .about
        return
    }

    ###################
    # CREATING WIDGETS
    ###################
    toplevel .about -class Toplevel
    wm focusmodel .about passive
    wm geometry .about +350+400
    wm overrideredirect .about 0
    wm resizable .about 0 0
    wm deiconify .about
    wm title .about "About RADVISION RTSP Test Application"
    wm protocol .about WM_DELETE_WINDOW {
        .about.ok invoke
        focus .test
    }
    wm transient .about .dummy
    focus .dummy
    gui:centerWindow .about .test

    text .about.info -cursor arrow -borderwidth 0 -background [.about cget -background] \
        -width 50 -height 5
    .about.info tag configure header -font {Helvetica -14 bold} -foreground black -lmargin1 5
    .about.info tag configure normal -font {Helvetica -11}
    .about.info tag configure no -font {Helvetica -10}
    .about.info tag configure yes -font {Helvetica -11 bold}

    .about.info insert end "\n\nRTSP Test Application\n" header
    .about.info insert end "$tmp(version)\n" normal
    .about.info insert end "\n"
    .about.info insert end "Copyright © RADVISION LTD.\n"
    .about.info insert end "\n" normal

    .about.info configure -state disabled

    image create photo about -format gif -data $tmp(about)
    frame .about.pic -background black -borderwidth 0 -relief ridge
    label .about.pic.bmp -image about -borderwidth 0 -anchor s

    button .about.ok -text "OK" -pady 0 -width 8 -command {Window close .about
        focus .test}

    ###################
    # SETTING GEOMETRY
    ###################
    grid rowconf .about 0 -weight 1
    grid columnconf .about 0 -weight 1
    grid .about.info -in .about -column 0 -row 0 -sticky nesw -rowspan 2 -padx 5 -pady 5
    grid .about.pic -in .about -column 0 -row 0 -pady 8 -padx 8 -sticky ne
    grid .about.pic.bmp -in .about.pic -column 0 -row 0
    grid .about.ok -in .about -column 0 -row 1 -sticky se -padx 10 -pady 5

    ###########
    # BINDINGS
    ###########
    bind .about <Key-Return> {.about.ok invoke}
    bind .about <Control-Key-Return> { montage:display }
}

proc montage:display { } {
    global tmp app

    if {[winfo exists .mont]} {
        wm deiconify .mont
        return
    }

    ###################
    # CREATING WIDGETS
    ###################
    toplevel .mont -class Toplevel
    wm focusmodel .mont passive
    wm geometry .mont 494x384+250+300
    wm overrideredirect .mont 0
    wm resizable .mont 0 0
    wm deiconify .mont
    wm title .mont "The people who made this (and a few others)"
    wm protocol .mont WM_DELETE_WINDOW {
        Window close .mont
        focus .about
    }
    wm transient .mont .about

    label .mont.tl -image montage_tl -borderwidth 0 -anchor se
    label .mont.tr -image montage_tr -borderwidth 0 -anchor sw
    label .mont.bl -image montage_bl -borderwidth 0 -anchor ne
    label .mont.br -image montage_br -borderwidth 0 -anchor nw
    grid rowconf .mont 0 -weight 1
    grid rowconf .mont 1 -weight 1
    grid columnconf .mont 0 -weight 1
    grid columnconf .mont 1 -weight 1
    grid .mont.tl -in .mont -column 0 -row 0
    grid .mont.tr -in .mont -column 1 -row 0
    grid .mont.bl -in .mont -column 0 -row 1
    grid .mont.br -in .mont -column 1 -row 1
}
