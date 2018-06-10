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
#                                 TRTSP_balloon.tcl
#
#  This file handles widget balloons.
#  It is responsible for creating tooltips whenever a specific button or any other visual
#  component is pointed by the mouse.
#
#
#
#
##############################################################################################


# Global balloon bindings and internal functions
#################################################

# Entering a widget with a balloon will cause it display its tooltip after 500ms
bind aballoon <Enter> {
    set is(balloon,set) 0
    set is(balloon,first) 1
    set is(balloon,id) [after 500 {balloon %W $is(balloon,%W)}]
}

# Pressing any mouse button on a widget with a balloon removes its tooltip from view
bind aballoon <Button> {
    set is(balloon,first) 0
    kill_balloon
}

# Leaving a widget with a balloon removes its tooltip from view
bind aballoon <Leave> {
    set is(balloon,first) 0
    kill_balloon
}

# Moving inside a widget with a baloon will make the 500ms to restart if tooltip not in view
bind aballoon <Motion> {
    if {$is(balloon,set) == 0} {
        # restart the alarm clock
        after cancel $is(balloon,id)
        set is(balloon,id) [after 500 {balloon %W $is(balloon,%W)}]
    }
}


# kill_balloon
# This procedure is called automatically to remove the tooltip of a widget from view
# input  : none
# output : none
# return : none
proc kill_balloon {} {
    global is
    after cancel $is(balloon,id)
    if {[winfo exists .balloon] == 1} {
        destroy .balloon
    }
    set is(balloon,set) 0
}

# balloon
# This procedure is called automatically to display a tooltip of a widget
# input  : target   - The widget we're handling
#          message  - The tooltip to display
# output : none
# return : none
proc balloon {target message} {
    global is
    if {[winfo exists $target] && ($is(balloon,first) == 1)} {
        set is(balloon,first) 2
        set x [expr [winfo rootx $target] + ([winfo width $target]/2)]
        set y [expr [winfo rooty $target] + [winfo height $target] + 4]
        toplevel .balloon -bg black
        wm overrideredirect .balloon 1
        label .balloon.l \
            -text $message -relief flat -justify left \
            -bg #ffffe1 -fg black -padx 2 -pady 0 -anchor w
        pack .balloon.l -side left -padx 0 -pady 0
        wm geometry .balloon +${x}+${y}
        set is(balloon,set) 1
    }
}






# Baloon functions
###################

# balloon:set
# This procedure sets a tooltip for a specific widget.
# It should be called when creating a window for all widgets that the user wants tooltips for
# input  : target   - The target widget
#          message  - The tooltip message to display for widget
# output : none
# return : none
proc balloon:set {target message} {
    global is
    if {[array names is "balloon,$target"] == ""} {
        bindtags $target "[bindtags $target] aballoon"
    }
    set is(balloon,$target) $message
}

# balloon:unset
# This procedure unsets a tooltip for a specific widget.
# It should be called when destorying a window for all widgets that the user wants tooltips for
# on windows which will never be created again
# input  : target   - The target widget
#          message  - The tooltip message to display for widget
# output : none
# return : none
proc balloon:unset {target message} {
    global is
    unset is(balloon,$target)
}

