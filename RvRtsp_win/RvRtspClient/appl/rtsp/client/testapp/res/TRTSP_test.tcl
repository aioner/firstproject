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
#                                 TRTSP_test.tcl
#
# This file contains the main window GUI along with some of its procedures, which are not
# related to any other specific window (such as calls, tools, etc).
#
##############################################################################################



##############################################################################################
#
#   MAIN WINDOW operations
#
##############################################################################################


# test:create
# This procedure creates the main window of the test application
# input  : none
# output : none
# return : none
proc test:create {} {
    global tmp app

    ###################
    # CREATING WIDGETS
    ###################
    toplevel .test -class Toplevel -menu .test.main
    wm iconify .test
    wm focusmodel .test passive
    wm geometry .test $app(test,size)
    wm overrideredirect .test 0
    wm resizable .test 1 1
    wm title .test $tmp(version)
    wm protocol .test WM_DELETE_WINDOW {
        set app(test,size) [winfo geometry .test]
        test.Quit
    }

    # Create the menu for this window
    test:createMenu

    # Tool buttons
    frame .test.line1 -relief sunken -border 1 -height 2
    frame .test.tools
    button .test.tools.log -state disabled -relief flat -border 1 -command {Window toggle .log} -image LogBut -tooltip "Stack Log"
    button .test.tools.logFile -relief flat -border 1 -command {logfile:open} -image LogFileBut -tooltip "Open Log File"
    button .test.tools.raise -relief flat -border 1 -command {focus .dummy} -image RaiseBut -tooltip "Raise Windows"
    label .test.tools.marker1 -relief sunken -border 1
    set tools {{log logFile raise}}

    # top bar
    image create photo topbar -format gif -data $tmp(topbarFade)
    label .test.tools.topbar -image topbar -borderwidth 0 -anchor e
    frame .test.line2 -relief sunken -border 1 -height 2

    # connections
    frame .test.conns -borderwidth 2 -relief groove
    listbox .test.conns.connlist -selectmode single -exportselection 0 -height 1 -background White \
        -yscrollcommand {.test.conns.sb set} -xscrollcommand {.test.conns.xsb set}
    scrollbar .test.conns.sb -orient vertical -command {.test.conns.connlist yview}
    scrollbar .test.conns.xsb -orient horizontal -command {.test.conns.connlist xview}
    listbox .test.conns.sesslist -selectmode single -exportselection 0 -height 1 -background White \
        -yscrollcommand {.test.conns.ssb set} -xscrollcommand {.test.conns.xssb set}
    scrollbar .test.conns.ssb -orient vertical -command {.test.conns.sesslist yview}
    scrollbar .test.conns.xssb -orient horizontal -command {.test.conns.sesslist xview}
    label .test.conns.hideTop -borderwidth 0

    # Tabs Section
    set tmp(test,otherTabs) { 1 2 3 4 5 6 }

    set tabs [
        notebook:create test .test {
            { "Client"  test:client}
            { "Connection" test:connection}
            { "Session"  test:session}
            { "Requests" test:requests}
            { "Response" test:response}
            { "Options" test:optionTab }
        }
    ]

    # Messages
    frame .test.msg -borderwidth 2 -relief groove
    listbox .test.msg.list -selectmode single -exportselection 0 -height 1 -background White \
        -yscrollcommand {.test.msg.yscrl set} -xscrollcommand {.test.msg.xscrl set}
    scrollbar .test.msg.xscrl -orient horizontal -command {.test.msg.list xview}
    scrollbar .test.msg.yscrl -command {.test.msg.list yview}
    button .test.msg.clear -borderwidth 2 -command {.test.msg.list delete 0 end} \
        -image bmpClear -tooltip "Clear message box (Ctrl-m)"

    # Manual frame
    frame .test.manual -relief groove -borderwidth 0

    ###################
    # SETTING GEOMETRY
    ###################
    grid columnconf .test 0 -weight 1
    grid columnconf .test 1 -weight 0 -minsize 275
    grid rowconf .test 3 -weight 0 -minsize 6
    grid rowconf .test 4 -weight 0 -minsize 135
    grid rowconf .test 5 -weight 0 -minsize 7
    grid rowconf .test 6 -weight 0 -minsize 135
    grid rowconf .test 7 -weight 0 -minsize 5
    grid rowconf .test 8 -weight 1

    # Displaying the tool buttons
    grid .test.line1 -in .test -column 0 -row 0 -columnspan 3 -sticky new -pady 1
    grid .test.tools -in .test -column 0 -row 0 -columnspan 3 -sticky ew -pady 4
    set first 1
    foreach toolGroup $tools {
        foreach tool $toolGroup {
            pack .test.tools.$tool -in .test.tools -ipadx 2 -side left
        }
        if {$first == 1} {
            pack .test.tools.marker1 -in .test.tools -pady 2 -padx 1 -side left -fill y
            set first 0
        }
    }

    # top bar
    pack .test.tools.topbar -in .test.tools -side right
    grid .test.line2 -in .test -column 0 -row 0 -columnspan 3 -sticky sew -pady 1

    # connections Tree
    grid .test.conns -in .test -column 0 -row 4 -rowspan 3 -sticky nesw -pady 2
    grid columnconf .test.conns 0 -weight 1
    grid rowconf .test.conns 0 -weight 0 -minsize 1
    grid rowconf .test.conns 1 -weight 1
    grid rowconf .test.conns 3 -weight 1
    grid .test.conns.connlist -in .test.conns -column 0 -row 1 -sticky nesw -padx 1 -pady 1
    grid .test.conns.sb -in .test.conns -column 1 -row 1 -sticky nesw -pady 1
    grid .test.conns.xsb -in .test.conns -column 0 -row 2 -sticky nesw -pady 1
    grid .test.conns.sesslist -in .test.conns -column 0 -row 3 -sticky nesw -padx 1 -pady 1
    grid .test.conns.ssb -in .test.conns -column 1 -row 3 -sticky nesw -pady 1
    grid .test.conns.xssb -in .test.conns -column 0 -row 4 -sticky nesw -pady 1
    grid .test.conns.hideTop -in .test.conns -column 1 -row 0 -sticky nesw

    # Tabs Section
    grid $tabs -in .test -column 1 -row 4 -rowspan 3 -sticky nesw -pady 2

    # Messages
    grid .test.msg -in .test -column 0 -row 8 -columnspan 3 -sticky nesw -pady 4 -padx 2
    grid columnconf .test.msg 0 -weight 1
    grid rowconf .test.msg 0 -weight 0 -minsize 5
    grid rowconf .test.msg 1 -weight 1
    grid .test.msg.list -in .test.msg -column 0 -row 1 -sticky nesw -padx 1 -pady 1
    grid .test.msg.xscrl -in .test.msg -column 0 -row 2 -sticky we
    grid .test.msg.yscrl -in .test.msg -column 1 -row 1 -sticky ns
    grid .test.msg.clear -in .test.msg -column 1 -row 2 -sticky news

    ###########
    # BINDINGS
    ###########
    bind .test <Control-Key-m> {.test.msg.clear invoke}
    bind .test <Control-Key-l> {.test.tools.logFile invoke}
    bind .test.msg.list <<ListboxSelect>> {.test.msg.list selection clear 0 end}
    bind .test.conns.connlist <<ListboxSelect>> "test:GetSessionsOnConn"

    foreach toolGroup $tools {
        foreach tool $toolGroup {
            bindtags .test.tools.$tool "[bindtags .test.tools.$tool] toolbar"
        }
    }

    ########
    # OTHER
    ########

    placeHeader .test.conns "Connections"
    placeHeader .test.msg "Messages"
    after idle {wm geometry .test $app(test,size)}

}


proc test:client {base tabButton} {
    global tmp app
    set tmp(client) $base
    label $base.maxConnections        -borderwidth 0 -text "Max Connections:"
    entry $base.maxConnectionsEnt     -borderwidth 1 -textvariable app(config,maxConnections) -width 6
    label $base.maxRequests           -borderwidth 0 -text "Max Requests:"
    entry $base.maxRequestsEnt        -borderwidth 1 -textvariable app(config,maxRequests) -width 6
    label $base.maxResponses          -borderwidth 0 -text "Max Responses:"
    entry $base.maxResponsesEnt       -borderwidth 1 -textvariable app(config,maxResponses) -width 6
    label $base.memoryElementSize     -borderwidth 0 -text "Memory Element Size:"
    entry $base.memoryElementSizeEnt  -borderwidth 1 -textvariable app(config,memoryElementSize) -width 6
    label $base.memoryElementNum      -borderwidth 0 -text "Memory Element Number:"
    entry $base.memoryElementNumEnt   -borderwidth 1 -textvariable app(config,memoryElementNum) -width 6
    label $base.maxHeaders            -borderwidth 0 -text "Max Headers:"
    entry $base.maxHeadersEnt         -borderwidth 1 -textvariable app(config,maxHeaders) -width 6
    button $base.connect              -borderwidth 2 -text "Connect" -highlightthickness 1 -width 7 \
            -command {test.Connect $app(config,stackIp)} -tooltip "Connect to a remote Media Gateway stack"
    entry $base.stackIp               -borderwidth 1 -textvariable app(config,stackIp) -width 15
    label $base.dnsAddr               -borderwidth 0 -text "DNS Address:"
    entry $base.dnsAddrEnt            -borderwidth 1 -textvariable app(config,dnsAddr) -width 15
    checkbutton $base.autoAnswer      -text "Auto Answer" -variable app(config,autoAnswer)

    grid rowconf $base 7 -weight 1
    grid columnconf $base 1 -weight 1

    grid $base.maxConnections         -in $base -column 0 -row 0  -sticky w -padx 3 -pady 3
    grid $base.maxConnectionsEnt      -in $base -column 1 -row 0  -sticky w -padx 3 -pady 3
    grid $base.maxRequests            -in $base -column 0 -row 1  -sticky w -padx 3 -pady 3
    grid $base.maxRequestsEnt         -in $base -column 1 -row 1  -sticky w -padx 3 -pady 3
    grid $base.maxResponses           -in $base -column 0 -row 2  -sticky w -padx 3 -pady 3
    grid $base.maxResponsesEnt        -in $base -column 1 -row 2  -sticky w -padx 3 -pady 3
    grid $base.memoryElementSize      -in $base -column 0 -row 3  -sticky w -padx 3 -pady 3
    grid $base.memoryElementSizeEnt   -in $base -column 1 -row 3  -sticky w -padx 3 -pady 3
    grid $base.memoryElementNum       -in $base -column 0 -row 4  -sticky w -padx 3 -pady 3
    grid $base.memoryElementNumEnt    -in $base -column 1 -row 4  -sticky w -padx 3 -pady 3
    grid $base.maxHeaders             -in $base -column 0 -row 5  -sticky w -padx 3 -pady 3
    grid $base.maxHeadersEnt          -in $base -column 1 -row 5  -sticky w -padx 3 -pady 3
    grid $base.dnsAddr                -in $base -column 0 -row 6  -sticky w -padx 3 -pady 3
    grid $base.dnsAddrEnt             -in $base -column 1 -row 6  -sticky w -padx 3 -pady 3
    grid $base.stackIp                -in $base -column 0 -row 7  -sticky ew -padx 3 -pady 3
    grid $base.connect                -in $base -column 1 -row 7  -sticky w -padx 3 -pady 3
    grid $base.autoAnswer             -in $base -column 0 -row 10 -sticky w -columnspan 2 -padx 2 -pady 2

}


proc test:connection {base tabButton} {
    global tmp app
    set tmp(connection) $base

    frame $base.conf -borderwidth 2 -relief groove

    label $base.conf.maxSession        -borderwidth 0 -text "Max Sessions:"
    entry $base.conf.maxSessionEnt     -borderwidth 1 -textvariable app(config,maxSession) -width 6
    label $base.conf.maxDescReq        -borderwidth 0 -text "Max Waiting Desc Request:"
    entry $base.conf.maxDescReqEnt     -borderwidth 1 -textvariable app(config,maxDescReq) -width 6
    label $base.conf.transmitQSize     -borderwidth 0 -text "Transmit Queue Size:"
    entry $base.conf.transmitQSizeEnt  -borderwidth 1 -textvariable app(config,transmitQSize) -width 6
    label $base.conf.maxUri            -borderwidth 0 -text "Max URIs:"
    entry $base.conf.maxUriEnt         -borderwidth 1 -textvariable app(config,maxUri) -width 6
    label $base.conf.dnsMaxResults     -borderwidth 0 -text "Max DNS results:"
    entry $base.conf.dnsMaxResultsEnt  -borderwidth 1 -textvariable app(config,dnsMaxResults) -width 6
    label $base.conf.descTimeout       -borderwidth 0 -text "Desc Response Timeout:"
    entry $base.conf.descTimeoutEnt    -borderwidth 1 -textvariable app(config,descTimeout) -width 6

    frame $base.descRequest -borderwidth 2 -relief groove

    foreach n {1 2 3 4} \
            number {one two three four} {
       checkbutton $base.descRequest.${number}check -variable app(config,${number}check)
       entry $base.descRequest.${number}Ent     -borderwidth 2 -textvariable app(config,${number}Ent) -width 40
    }
    label $base.descRequest.reqDescUri   -text "URI"
    button $base.descRequest.sendRequest -text "DESCRIBE" -highlightthickness 1 -width 10 -borderwidth 1 -command {test:SendDescRequest $app(config,onecheck) $app(config,twocheck) $app(config,threecheck) $app(config,fourcheck) $app(config,oneEnt) $app(config,twoEnt) $app(config,threeEnt) $app(config,fourEnt)}

    button $base.destruct       -borderwidth 1 -text "DISCONNECT CONNECTION" -highlightthickness 1 -width 25 -command {test:DestructConnection} -tooltip "Disconnect the selected connection"
    entry $base.rawData         -borderwidth 1 -textvariable app(config,rawData) -width 40
    button $base.sendData       -borderwidth 1 -text "Send Raw Buffer" -highlightthickness 1 -width 15

    grid rowconf $base 7 -weight 1
    grid columnconf $base 1 -weight 1

    grid $base.conf -in $base -column 0 -row 0 -sticky nwse -columnspan 2
    grid columnconf $base.conf 1 -weight 1

    grid $base.conf.maxSession          -in $base.conf -column 0 -row 0 -sticky w -padx 3 -pady 3
    grid $base.conf.maxSessionEnt       -in $base.conf -column 1 -row 0 -sticky w -padx 3 -pady 3
    grid $base.conf.maxDescReq          -in $base.conf -column 0 -row 1 -sticky w -padx 3 -pady 3
    grid $base.conf.maxDescReqEnt       -in $base.conf -column 1 -row 1 -sticky w -padx 3 -pady 3
    grid $base.conf.transmitQSize       -in $base.conf -column 0 -row 2 -sticky w -padx 3 -pady 3
    grid $base.conf.transmitQSizeEnt    -in $base.conf -column 1 -row 2 -sticky w -padx 3 -pady 3
    grid $base.conf.maxUri              -in $base.conf -column 0 -row 4 -sticky w -padx 3 -pady 3
    grid $base.conf.maxUriEnt           -in $base.conf -column 1 -row 4 -sticky w -padx 3 -pady 3
    grid $base.conf.dnsMaxResults       -in $base.conf -column 0 -row 5 -sticky w -padx 3 -pady 3
    grid $base.conf.dnsMaxResultsEnt    -in $base.conf -column 1 -row 5 -sticky w -padx 3 -pady 3
    grid $base.conf.descTimeout         -in $base.conf -column 0 -row 6 -sticky w -padx 3 -pady 3
    grid $base.conf.descTimeoutEnt      -in $base.conf -column 1 -row 6 -sticky w -padx 3 -pady 3

    grid $base.descRequest -in $base -column 0 -row 1 -sticky nwse -columnspan 7
    grid columnconf $base.descRequest 3 -weight 1

    grid $base.descRequest.reqDescUri   -in $base.descRequest -column 0 -row 0 -sticky w -padx 3 -pady 3
    foreach  number {one two three four} r {1 2 3 4} {
        grid $base.descRequest.${number}Ent     -in $base.descRequest -column 0 -row $r -sticky w -padx 3 -pady 3
        grid $base.descRequest.${number}check   -in $base.descRequest -column 1 -row $r -sticky w -padx 3 -pady 3
    }
    grid $base.descRequest.sendRequest -in $base.descRequest -column 1 -row 5 -sticky w -padx 3 -pady 3
    grid $base.destruct -in $base -column 0 -row 6 -sticky w -padx 2 -pady 2

    grid $base.rawData  -in $base -column 0 -row 10 -sticky ew -padx 2 -pady 2
    grid $base.sendData -in $base -column 1 -row 10 -sticky w -padx 2 -pady 2
}


proc test:session {base tabButton} {
    global tmp app
    set tmp(session) $base

    frame $base.conf -borderwidth 2  -relief groove

    label $base.conf.responseTimeout         -borderwidth 0 -text "Response Timeout:"
    entry $base.conf.responseTimeoutEnt      -borderwidth 1 -textvariable app(config,responseTimeout) -width 6
    label $base.conf.pingTimeout             -borderwidth 0 -text "Ping Timeout:"
    entry $base.conf.pingTimeoutEnt          -borderwidth 1 -textvariable app(config,pingTimeout) -width 6

    frame $base.commands -borderwidth 2

    button $base.commands.setup         -borderwidth 1 -text "SETUP" -command {test:SetUpSession $app(config,setupURI)} -highlightthickness 1 -width 10
    label $base.commands.setupURI       -borderwidth 0 -text "URI:"
    entry $base.commands.setupURIEnt    -borderwidth 1 -textvariable app(config,setupURI) -width 40
    button $base.commands.play          -borderwidth 1 -text "PLAY" -command {test:PlaySession} -highlightthickness 1 -width 10
    button $base.commands.pause         -borderwidth 1 -text "PAUSE" -command {test:PauseSession} -highlightthickness 1 -width 10
    button $base.commands.teardown      -borderwidth 1 -text "TEARDOWN" -command {test:TeardownSession} -highlightthickness 1 -width 10

    grid rowconf $base 5 -weight 1
    grid columnconf $base 1 -weight 1

    grid $base.commands -in $base -column 0 -row 1 -sticky nwse -columnspan 3
    grid columnconf $base.commands 3 -weight 1

    grid $base.commands.setup              -in $base.commands -column 0 -row 0  -sticky w -padx 3 -pady 3
    grid $base.commands.setupURI           -in $base.commands -column 1 -row 0  -sticky w -padx 3 -pady 3
    grid $base.commands.setupURIEnt        -in $base.commands -column 2 -row 0  -sticky w -padx 3 -pady 3
    grid $base.commands.play               -in $base.commands -column 0 -row 1  -sticky w -padx 3 -pady 3
    grid $base.commands.pause              -in $base.commands -column 0 -row 2  -sticky w -padx 3 -pady 3
    grid $base.commands.teardown           -in $base.commands -column 0 -row 3  -sticky w -padx 3 -pady 3

    grid $base.conf -in $base -column 0 -row 0 -sticky nwse -columnspan 7
    grid columnconf $base.conf 1 -weight 1

    grid $base.conf.responseTimeout      -in $base.conf -column 0 -row 0 -sticky w -padx 3 -pady 3
    grid $base.conf.responseTimeoutEnt   -in $base.conf -column 1 -row 0 -sticky w -padx 3 -pady 3
    grid $base.conf.pingTimeout          -in $base.conf -column 0 -row 1 -sticky w -padx 3 -pady 3
    grid $base.conf.pingTimeoutEnt       -in $base.conf -column 1 -row 1 -sticky w -padx 3 -pady 3

}

proc test:requests {base tabButton} {
    global app tmp
    set tmp(requests) $base

    label $base.reqMethod          -borderwidth 0 -text "Method:"
    entry $base.reqMethodEnt       -borderwidth 1 -textvariable app(config,reqMethod) -width 6
    label $base.reqUri             -borderwidth 0 -text "URI:"
    entry $base.reqUriEnt          -borderwidth 1 -textvariable app(config,reqUri) -width 40
    label $base.reqCseq            -borderwidth 0 -text "CSeq:"
    entry $base.reqCseqEnt         -borderwidth 1 -textvariable app(config,reqCseq) -width 6
    label $base.reqAccept          -borderwidth 0 -text "Accept:"
    entry $base.reqAcceptEnt       -borderwidth 1 -textvariable app(config,reqAccept) -width 40
    label $base.reqRequire         -borderwidth 0 -text "Require:"
    entry $base.reqRequireEnt      -borderwidth 1 -textvariable app(config,reqRequire) -width 40
    button $base.sendRequest       -borderwidth 1 -text "Send Request" -command {test:SendRequest $app(config,reqMethod) $app(config,reqUri) $app(config,reqCseq) $app(config,reqTrackId) $app(config,reqAccept) $app(config,reqRequire) $app(config,reqSessionId) $app(config,reqSessionTimeout) $app(config,reqTrClientPortA) $app(config,reqTrClientPortB) $app(config,reqTrServerPortA) $app(config,reqTrServerPortB) $app(config,reqIsUnicast) $app(config,reqTrDest) $app(config,reqStartHTime) $app(config,reqStartMTime) $app(config,reqStartSTime) $app(config,reqEndHTime) $app(config,reqEndMTime) $app(config,reqEndSTime) } -highlightthickness 1 -width 12
    entry $base.reqTrackId         -borderwidth 0 -textvariable app(config,reqTrackId) -width 6

    frame $base.session -borderwidth 2
    label $base.session.name                 -borderwidth 0 -text "Session:"
    label $base.session.reqSessionId         -borderwidth 0 -text "Session Id"
    entry $base.session.reqSessionIdEnt      -borderwidth 1 -textvariable app(config,reqSessionId) -width 6
    label $base.session.reqSessionTimeout    -borderwidth 0 -text "Time out"
    entry $base.session.reqSessionTimeoutEnt -borderwidth 1 -textvariable app(config,reqSessionTimeout) -width 6

    frame $base.range -borderwidth 2
    label $base.range.name                     -borderwidth 0 -text "Range:"
    label $base.range.reqRangeStartTime        -borderwidth 0 -text "Start Time:"
    entry $base.range.reqRangeStartTimeHrsEnt  -borderwidth 1 -textvariable app(config,reqStartHTime) -width 2
    entry $base.range.reqRangeStartTimeMinEnt  -borderwidth 1 -textvariable app(config,reqStartMTime) -width 2
    entry $base.range.reqRangeStartTimeSecEnt  -borderwidth 1 -textvariable app(config,reqStartSTime) -width 2
    label $base.range.reqRangeEndTime          -borderwidth 0 -text "End Time:"
    entry $base.range.reqRangeEndTimeHrsEnt    -borderwidth 1 -textvariable app(config,reqEndHTime) -width 2
    entry $base.range.reqRangeEndTimeMinEnt    -borderwidth 1 -textvariable app(config,reqEndMTime) -width 2
    entry $base.range.reqRangeEndTimeSecEnt    -borderwidth 1 -textvariable app(config,reqEndSTime) -width 2

    frame $base.transport -borderwidth 2
    label $base.transport.name                  -borderwidth 0 -text "Transport:"
    label $base.transport.reqTrClientPortA      -borderwidth 0 -text "Client PortA"
    entry $base.transport.reqTrClientPortAEnt   -borderwidth 1 -textvariable app(config,reqTrClientPortA) -width 8
    label $base.transport.reqTrClientPortB      -borderwidth 0 -text "Client PortB"
    entry $base.transport.reqTrClientPortBEnt   -borderwidth 1 -textvariable app(config,reqTrClientPortB) -width 8
    label $base.transport.reqTrServerPortA      -borderwidth 0 -text "Server PortA"
    entry $base.transport.reqTrServerPortAEnt   -borderwidth 1 -textvariable app(config,reqTrServerPortA) -width 8
    label $base.transport.reqTrServerPortB      -borderwidth 0 -text "Server PortB"
    entry $base.transport.reqTrServerPortBEnt   -borderwidth 1 -textvariable app(config,reqTrServerPortB) -width 8
    checkbutton $base.transport.reqTrIsUnicast  -text "Unicast" -variable app(config,reqIsUnicast)
    label $base.transport.reqTrDest             -borderwidth 0 -text "Destination"
    entry $base.transport.reqTrDestEnt          -borderwidth 1 -textvariable app(config,reqTrDest) -width 15

    frame $base.addlH -borderwidth 2
    label $base.addlH.message       -borderwidth 0 -text "Message              "
    label $base.addlH.header        -borderwidth 0 -text "Header"
    entry $base.addlH.headerEnt     -borderwidth 1 -textvariable app(config,reqHeader) -width 10
    label $base.addlH.field1        -borderwidth 0 -text "Field1"
    label $base.addlH.value1        -borderwidth 0 -text "Value1"
    entry $base.addlH.field1Ent     -borderwidth 1 -textvariable app(config,reqField1) -width 5
    entry $base.addlH.value1Ent     -borderwidth 1 -textvariable app(config,reqValue1) -width 5
    label $base.addlH.field2        -borderwidth 0 -text "Field2"
    label $base.addlH.value2        -borderwidth 0 -text "Value2"
    entry $base.addlH.field2Ent     -borderwidth 1 -textvariable app(config,reqField2) -width 5
    entry $base.addlH.value2Ent     -borderwidth 1 -textvariable app(config,reqValue2) -width 5
    label $base.addlH.field3        -borderwidth 0 -text "Field3"
    label $base.addlH.value3        -borderwidth 0 -text "Value3"
    entry $base.addlH.field3Ent     -borderwidth 1 -textvariable app(config,reqField3) -width 5
    entry $base.addlH.value3Ent     -borderwidth 1 -textvariable app(config,reqValue3) -width 5
    label $base.addlH.delim         -borderwidth 0 -text "Delim"
    entry $base.addlH.delimEnt      -borderwidth 1 -textvariable app(config,reqDelim) -width 3
    button $base.addlH.add          -borderwidth 1 -text "Add" -highlightthickness 1 -width 4 -command {test:ReqRecordAdd $app(config,reqMessage) $app(config,reqHeader) $app(config,reqField1) $app(config,reqValue1) $app(config,reqField2) $app(config,reqValue2) $app(config,reqField3) $app(config,reqValue3) $app(config,reqDelim)} -tooltip "Add the record"
    button $base.addlH.del          -borderwidth 1 -text "Del" -highlightthickness 1 -width 4 -command {test:ReqRecordDel} -tooltip "Delete the selected record"
    histEnt:create $base.addlH message $base.addlH config,reqMessage "Message"

    frame $base.record -borderwidth 2 -relief groove
    listbox $base.record.list -selectmode single -exportselection 0 -height 1 -background White \
        -yscrollcommand {$tmp(requests).record.yscrl set} -xscrollcommand {$tmp(requests).record.xscrl set}
    scrollbar $base.record.xscrl -orient horizontal -command {$tmp(requests).record.list xview}
    scrollbar $base.record.yscrl -command {$tmp(requests).record.list yview}

    grid rowconf $base 12 -weight 1
    grid columnconf $base 2 -weight 1

    grid $base.reqMethod                -in $base -column 0 -row 0  -sticky w -padx 3 -pady 3
    grid $base.reqMethodEnt             -in $base -column 1 -row 0  -sticky w -padx 3 -pady 3
    grid $base.reqUri                   -in $base -column 0 -row 1  -sticky w -padx 3 -pady 3
    grid $base.reqUriEnt                -in $base -column 1 -row 1  -sticky w -padx 3 -pady 3
    grid $base.reqCseq                  -in $base -column 0 -row 2  -sticky w -padx 3 -pady 3
    grid $base.reqCseqEnt               -in $base -column 1 -row 2  -sticky w -padx 3 -pady 3
    grid $base.reqAccept                -in $base -column 0 -row 3  -sticky w -padx 3 -pady 3
    grid $base.reqAcceptEnt             -in $base -column 1 -row 3  -sticky w -padx 3 -pady 3
    grid $base.reqRequire               -in $base -column 0 -row 5  -sticky w -padx 3 -pady 3
    grid $base.reqRequireEnt            -in $base -column 1 -row 5  -sticky w -padx 3 -pady 3
    grid $base.sendRequest              -in $base -column 0 -row 13 -sticky w -padx 3 -pady 3

    grid $base.session -in $base -column 0 -row 6 -sticky nwse -columnspan 6
    grid columnconf $base.session 5 -weight 1

    grid $base.session.name                     -in $base.session -column 0 -row 0 -sticky w -padx 3 -pady 3
    grid $base.session.reqSessionId             -in $base.session -column 1 -row 0 -sticky w -padx 3 -pady 3
    grid $base.session.reqSessionIdEnt          -in $base.session -column 2 -row 0 -sticky w -padx 3 -pady 3
    grid $base.session.reqSessionTimeout        -in $base.session -column 3 -row 0 -sticky w -padx 3 -pady 3
    grid $base.session.reqSessionTimeoutEnt     -in $base.session -column 4 -row 0 -sticky w -padx 3 -pady 3

    grid $base.range -in $base -column 0 -row 7 -sticky nwse -columnspan 6
    grid columnconf $base.range 8 -weight 1

    grid $base.range.name                      -in $base.range -column 0 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeStartTime         -in $base.range -column 1 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeStartTimeHrsEnt   -in $base.range -column 2 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeStartTimeMinEnt   -in $base.range -column 3 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeStartTimeSecEnt   -in $base.range -column 4 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeEndTime           -in $base.range -column 5 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeEndTimeHrsEnt     -in $base.range -column 6 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeEndTimeMinEnt     -in $base.range -column 7 -row 0 -sticky w -padx 3 -pady 3
    grid $base.range.reqRangeEndTimeSecEnt     -in $base.range -column 8 -row 0 -sticky w -padx 3 -pady 3

    grid $base.transport -in $base -column 0 -row 8 -sticky nwse -columnspan 6
    grid columnconf $base.transport 7 -weight 1

    grid $base.transport.name                      -in $base.transport -column 0 -row 0 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrClientPortA          -in $base.transport -column 1 -row 0 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrClientPortAEnt       -in $base.transport -column 2 -row 0 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrClientPortB          -in $base.transport -column 3 -row 0 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrClientPortBEnt       -in $base.transport -column 4 -row 0 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrIsUnicast            -in $base.transport -column 5 -row 0 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrServerPortA          -in $base.transport -column 1 -row 1 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrServerPortAEnt       -in $base.transport -column 2 -row 1 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrServerPortB          -in $base.transport -column 3 -row 1 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrServerPortBEnt       -in $base.transport -column 4 -row 1 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrDest                 -in $base.transport -column 5 -row 1 -sticky w -padx 3 -pady 3
    grid $base.transport.reqTrDestEnt              -in $base.transport -column 6 -row 1 -sticky w -padx 3 -pady 3

    grid $base.addlH -in $base -column 0 -row 9 -sticky nwse -columnspan 6
    grid columnconf $base.addlH 1 -weight 1

    grid $base.addlH.message         -in $base.addlH -column 0 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.messageHistEnt  -in $base.addlH -column 0 -row 1 -sticky ew -padx 3 -pady 3
    grid $base.addlH.header          -in $base.addlH -column 2 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.headerEnt       -in $base.addlH -column 2 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.field1          -in $base.addlH -column 3 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.value1          -in $base.addlH -column 4 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.field1Ent       -in $base.addlH -column 3 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.value1Ent       -in $base.addlH -column 4 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.field2          -in $base.addlH -column 5 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.value2          -in $base.addlH -column 6 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.field2Ent       -in $base.addlH -column 5 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.value2Ent       -in $base.addlH -column 6 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.field3          -in $base.addlH -column 7 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.value3          -in $base.addlH -column 8 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.field3Ent       -in $base.addlH -column 7 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.value3Ent       -in $base.addlH -column 8 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.delim           -in $base.addlH -column 9 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.delimEnt        -in $base.addlH -column 9 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.add             -in $base.addlH -column 10 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.del             -in $base.addlH -column 11 -row 1 -sticky e -padx 3 -pady 3

    grid $base.record -in $base -column 0 -row 10 -columnspan 3 -sticky nesw -pady 4 -padx 2
    grid columnconf $base.record 0 -weight 1
    grid rowconf $base.record 0 -weight 0 -minsize 5
    grid rowconf $base.record 1 -weight 1
    grid $base.record.list    -in $base.record -column 0 -row 1 -sticky nesw -padx 1 -pady 1
    grid $base.record.xscrl   -in $base.record -column 0 -row 2 -sticky we
    grid $base.record.yscrl   -in $base.record -column 1 -row 1 -sticky ns

}

proc test:response {base tabButton} {
    global app tmp
    set tmp(response) $base

    label $base.resCseq                -borderwidth 0 -text "CSeq:"
    entry $base.resCseqEnt             -borderwidth 1 -textvariable app(config,resCseq) -width 6
    label $base.resPublic              -borderwidth 0 -text "Public:"
    entry $base.resPublicEnt           -borderwidth 1 -textvariable app(config,resPublic) -width 40
    label $base.resContentType         -borderwidth 0 -text "Content Type:"
    entry $base.resContentTypeEnt      -borderwidth 1 -textvariable app(config,resContentType) -width 40
    label $base.resContentLength       -borderwidth 0 -text "Content Length:"
    entry $base.resContentLengthEnt    -borderwidth 1 -textvariable app(config,resContentLength) -width 6

    frame $base.status -borderwidth 2
    label $base.status.resStatLine   -borderwidth 0 -text "StatusLine:"
    label $base.status.resStat       -borderwidth 0 -text "Status"
    entry $base.status.resStatEnt    -borderwidth 1 -textvariable app(config,resStat) -width 6
    label $base.status.resPhrase     -borderwidth 0 -text "Phrase"
    entry $base.status.resPhraseEnt  -borderwidth 1 -textvariable app(config,resPhrase) -width 6

    frame $base.addlH -borderwidth 2
    label $base.addlH.message       -borderwidth 0 -text "Message              "
    label $base.addlH.header        -borderwidth 0 -text "Header"
    entry $base.addlH.headerEnt     -borderwidth 1 -textvariable app(config,resHeader) -width 10
    label $base.addlH.field1        -borderwidth 0 -text "Field1"
    label $base.addlH.value1        -borderwidth 0 -text "Value1"
    entry $base.addlH.field1Ent     -borderwidth 1 -textvariable app(config,resField1) -width 5
    entry $base.addlH.value1Ent     -borderwidth 1 -textvariable app(config,resValue1) -width 5
    label $base.addlH.field2        -borderwidth 0 -text "Field2"
    label $base.addlH.value2        -borderwidth 0 -text "Value2"
    entry $base.addlH.field2Ent     -borderwidth 1 -textvariable app(config,resField2) -width 5
    entry $base.addlH.value2Ent     -borderwidth 1 -textvariable app(config,resValue2) -width 5
    label $base.addlH.field3        -borderwidth 0 -text "Field3"
    label $base.addlH.value3        -borderwidth 0 -text "Value3"
    entry $base.addlH.field3Ent     -borderwidth 1 -textvariable app(config,resField3) -width 5
    entry $base.addlH.value3Ent     -borderwidth 1 -textvariable app(config,resValue3) -width 5
    label $base.addlH.delim         -borderwidth 0 -text "Delim"
    entry $base.addlH.delimEnt      -borderwidth 1 -textvariable app(config,resDelim) -width 3
    button $base.addlH.add          -borderwidth 1 -text "Add" -highlightthickness 1 -width 4 -command {test:ResRecordAdd $app(config,resMessage) $app(config,resHeader) $app(config,resField1) $app(config,resValue1) $app(config,resField2) $app(config,resValue2) $app(config,resField3) $app(config,resValue3) $app(config,resDelim)} -tooltip "Add the record"
    button $base.addlH.del          -borderwidth 1 -text "Del" -highlightthickness 1 -width 4 -command {test:ResRecordDel} -tooltip "Delete the selected record"
    histEnt:create $base.addlH message $base.addlH config,resMessage "Message"

    frame $base.record -borderwidth 2 -relief groove
    listbox $base.record.list -selectmode single -exportselection 0 -height 1 -background White \
        -yscrollcommand {$tmp(response).record.yscrl set} -xscrollcommand {$tmp(response).record.xscrl set}
    scrollbar $base.record.xscrl -orient horizontal -command {$tmp(response).record.list xview}
    scrollbar $base.record.yscrl -command {$tmp(response).record.list yview}

    entry $base.wreqMethod    -borderwidth 0 -textvariable app(config,wreqMethod) -width 5
    entry $base.wreqURI       -borderwidth 0 -textvariable app(config,wreqURI) -width 5

    button $base.sendResponse -text "Send Response" -command {test:SendResponse $app(config,wreqMethod) $app(config,resStat) $app(config,resPhrase) $app(config,resCseq) $app(config,resPublic) $app(config,resContentType) $app(config,resContentLength) $app(config,wreqURI)} -highlightthickness 1 -width 12 -borderwidth 1

    grid rowconf $base 8 -weight 1
    grid rowconf $base 5 -weight 1
    grid columnconf $base 2 -weight 1

    grid $base.resCseq                   -in $base -column 0 -row 1  -sticky w -padx 3 -pady 3
    grid $base.resCseqEnt                -in $base -column 1 -row 1  -sticky w -padx 3 -pady 3
    grid $base.resPublic                 -in $base -column 0 -row 2  -sticky w -padx 3 -pady 3
    grid $base.resPublicEnt              -in $base -column 1 -row 2  -sticky w -padx 3 -pady 3
    grid $base.resContentType            -in $base -column 0 -row 3  -sticky w -padx 3 -pady 3
    grid $base.resContentTypeEnt         -in $base -column 1 -row 3  -sticky w -padx 3 -pady 3
    grid $base.resContentLength          -in $base -column 0 -row 4  -sticky w -padx 3 -pady 3
    grid $base.resContentLengthEnt       -in $base -column 1 -row 4  -sticky w -padx 3 -pady 3
    grid $base.sendResponse              -in $base -column 0 -row 9 -sticky w -padx 3 -pady 3

    grid $base.status -in $base -column 0 -row 0 -sticky nwse -columnspan 6
    grid columnconf $base.status 5 -weight 1

    grid $base.status.resStatLine        -in $base.status -column 0 -row 0 -sticky w -padx 3 -pady 3
    grid $base.status.resStat            -in $base.status -column 1 -row 0 -sticky w -padx 3 -pady 3
    grid $base.status.resStatEnt         -in $base.status -column 2 -row 0 -sticky w -padx 3 -pady 3
    grid $base.status.resPhrase          -in $base.status -column 3 -row 0 -sticky w -padx 3 -pady 3
    grid $base.status.resPhraseEnt       -in $base.status -column 4 -row 0 -sticky w -padx 3 -pady 3

    grid $base.addlH -in $base -column 0 -row 6 -sticky nwse -columnspan 6
    grid columnconf $base.addlH 1 -weight 1

    grid $base.addlH.message         -in $base.addlH -column 0 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.messageHistEnt  -in $base.addlH -column 0 -row 1 -sticky ew -padx 3 -pady 3
    grid $base.addlH.header          -in $base.addlH -column 2 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.headerEnt       -in $base.addlH -column 2 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.field1          -in $base.addlH -column 3 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.value1          -in $base.addlH -column 4 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.field1Ent       -in $base.addlH -column 3 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.value1Ent       -in $base.addlH -column 4 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.field2          -in $base.addlH -column 5 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.value2          -in $base.addlH -column 6 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.field2Ent       -in $base.addlH -column 5 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.value2Ent       -in $base.addlH -column 6 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.field3          -in $base.addlH -column 7 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.value3          -in $base.addlH -column 8 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.field3Ent       -in $base.addlH -column 7 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.value3Ent       -in $base.addlH -column 8 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.delim           -in $base.addlH -column 9 -row 0 -sticky e -padx 3 -pady 3
    grid $base.addlH.delimEnt        -in $base.addlH -column 9 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.add             -in $base.addlH -column 10 -row 1 -sticky e -padx 3 -pady 3
    grid $base.addlH.del             -in $base.addlH -column 11 -row 1 -sticky e -padx 3 -pady 3

    grid $base.record -in $base -column 0 -row 7 -columnspan 3 -sticky nesw -pady 4 -padx 2
    grid columnconf $base.record 0 -weight 1
    grid rowconf $base.record 0 -weight 0 -minsize 5
    grid rowconf $base.record 1 -weight 1
    grid $base.record.list    -in $base.record -column 0 -row 1 -sticky nesw -padx 1 -pady 1
    grid $base.record.xscrl   -in $base.record -column 0 -row 2 -sticky we
    grid $base.record.yscrl   -in $base.record -column 1 -row 1 -sticky ns

}

# sendResponseVariables
# This procedure sends the response values filled in to the remote stack
# input  : none
# output : none
# return : none
proc sendResponseVariables {} {
    global app

    test:SendResponse  $app(config,wreqMethod) $app(config,hAppConn) $app(config,hAppSess) $app(config,resStat) $app(config,resPhrase) $app(config,resCseq) $app(config,resConnection) $app(config,resPublic) $app(config,resContentLang) $app(config,resContentEncoding) $app(config,resContentBase) $app(config,resContentType) $app(config,resContentLength) $app(config,resSessionId) $app(config,resSessionTimeOut) $app(config,resTrClientPortAEnt) $app(config,resTrClientPortBEnt) $app(config,resTrServerPortAEnt) $app(config,resTrServerPortBEnt) $app(config,reqIsUnicast) $app(config,resTrDest) $app(config,wreqURI) $app(config,resRtpUri) $app(config,resRtpSeqEnt) $app(config,resRtpTimeEnt) $app(config,resRtpUri2) $app(config,resRtpSeqEnt2) $app(config,resRtpTimeEnt2)

}

# sendRequestVariables
# This procedure sends the request values  filled in to the remote stack
# input  : none
# output : none
# return : none
proc sendRequestVariables {} {
    global app

    test:SendRequest $app(config,reqMethod) $app(config,reqUri) $app(config,reqCseq) $app(config,reqTrackId) $app(config,reqAccept) $app(config,reqRequire) $app(config,reqSessionId) $app(config,reqSessionTimeout) $app(config,reqTrClientPortA) $app(config,reqTrClientPortB) $app(config,reqTrServerPortA) $app(config,reqTrServerPortB) $app(config,reqIsUnicast) $app(config,reqTrDest) $app(config,reqStartHTime) $app(config,reqStartMTime) $app(config,reqStartSTime) $app(config,reqEndHTime) $app(config,reqEndMTime) $app(config,reqEndSTime)

}



set tmp(levelList) { "exp" "err" "wrn" "inf" "dbg" "ent" "lve" "syn" }
set tmp(ccLogSourceList) { "ALLOC" "APP" "ARES" "CLOCK" "EHD" "EMA" "EPP" "HOST" \
    "LDAP" "LOCK" "MEMORY" "MUTEX" "PORT" "QUEUE" "RA" "RCACHE" "SCTP" "SELECT" "SEMA4" \
    "SNMP" "SOCKET" "THREAD" "TIMER" "TIMERMGR" "TIMESTAMP" "TLS" "TM" }
set tmp(stackLogSourceList) { "RTSP" "SDP" }

proc setLogSourceCheckBoxes {} {
    global app tmp

    foreach level $tmp(levelList) col { 1 2 3 4 5 6 7 8 } {
        grid forget $tmp(optionTab).logSources.c$app(options,prevLogSource)$level
        grid $tmp(optionTab).logSources.c$app(options,logSource)$level -column $col -row 1
    }
    set app(options,prevLogSource) $app(options,logSource)
}


proc setAllLogSourceFilters {level} {
    global app tmp

    foreach logSource $tmp(ccLogSourceList) {
        set app(logFilter,$logSource,$level) $app(logFilter,_ALL_,$level)
    }
    foreach logSource $tmp(stackLogSourceList) {
        set app(logFilter,$logSource,$level) $app(logFilter,_ALL_,$level)
    }
}

proc test:optionTab {base tabButton} {
    global app tmp
    set tmp(optionTab) $base


    checkbutton $base.catchLog -text "Catch log to message box" -variable app(options,catchStackLog)
    grid $base.catchLog -in $base -column 0 -columnspan 2 -row 0 -sticky nwse

    frame $base.logSources -borderwidth 0
    grid $base.logSources -in $base -column 0 -columnspan 2 -row 1
    menubutton $base.logSources.logSourceMenu -indicatoron 0  -width 8 -height 1 \
            -menu $base.logSources.logSourceMenu.01 -relief raised -text "_ALL_" -textvariable app(options,logSource)
    menu $base.logSources.logSourceMenu.01 -activeborderwidth 1 -tearoff 0
    menu $base.logSources.logSourceMenu.02 -activeborderwidth 1 -tearoff 0
    menu $base.logSources.logSourceMenu.03 -activeborderwidth 1 -tearoff 0
    grid $base.logSources.logSourceMenu -in $base.logSources -column 0 -row 1
    foreach level $tmp(levelList) col { 1 2 3 4 5 6 7 8 } {
        label $base.logSources.l$level -text $level -width 3
        grid $base.logSources.l$level -in $base.logSources -column $col -row 0
    }
    $base.logSources.logSourceMenu.01 add radiobutton -indicatoron 0 -label "_ALL_" -value "_ALL_" \
        -variable app(options,logSource) -command setLogSourceCheckBoxes
    $base.logSources.logSourceMenu.01 add cascade -label "CommonCore" -menu $base.logSources.logSourceMenu.02
    $base.logSources.logSourceMenu.01 add cascade -label "Stack" -menu $base.logSources.logSourceMenu.03

    foreach level $tmp(levelList) {
        checkbutton $base.logSources.c_ALL_$level -text "" -variable app(logFilter,_ALL_,$level) -command "setAllLogSourceFilters $level"
    }
    foreach logSource $tmp(ccLogSourceList) {
        $base.logSources.logSourceMenu.02 add radiobutton -indicatoron 0 -label $logSource -value $logSource \
            -variable app(options,logSource) -command setLogSourceCheckBoxes
        foreach level $tmp(levelList) {
            checkbutton $base.logSources.c$logSource$level -text "" -variable app(logFilter,$logSource,$level)
        }
    }
    foreach logSource $tmp(stackLogSourceList) {
        $base.logSources.logSourceMenu.03 add radiobutton -indicatoron 0 -label $logSource -value $logSource \
            -variable app(options,logSource) -command setLogSourceCheckBoxes
        foreach level $tmp(levelList) {
            checkbutton $base.logSources.c$logSource$level -text "" -variable app(logFilter,$logSource,$level)
        }
    }
    foreach level $tmp(levelList) col { 1 2 3 4 5 6 7 8 } {
        grid $base.logSources.c_ALL_$level -in $base.logSources -column $col -row 1
    }
    set app(options,prevLogSource) "_ALL_"

    button $base.resetLog -text "Reset Log" -highlightthickness 0 -width 10 -borderwidth 1 \
        -command {test.ResetLog} -tooltip "Reset the log-file"
    grid $base.resetLog -in $base -column 0 -columnspan 2 -row 2 -pady 2

    frame $base.external -borderwidth 2 -relief groove
    foreach type {log} \
            txt {"Log viewer"} \
            i {1} {
        entry $base.external.${type}Ed -textvariable app(editor,$type) -width 10
        $base.external.${type}Ed xview moveto 1
        button $base.external.${type}But -text "..." \
            -command "options:chooseEditor $type"
        label $base.external.${type}Lab -borderwidth 0 -text $txt
        grid $base.external.${type}Lab -in $base.external -column 0 -row $i -sticky w -padx 4
        grid $base.external.${type}Ed -in $base.external -column 1 -row $i -sticky ew
        grid $base.external.${type}But -in $base.external -column 2 -row $i -pady 1 -padx 4
    }
    grid $base.external -in $base -column 0 -columnspan 2 -row 3 -padx 2 -sticky nsew -ipadx 2 -ipady 2

    grid columnconf $base.external 1 -weight 1
    grid rowconf $base.external 0 -minsize 3
    grid rowconf $base.external 1 -weight 1
    grid rowconf $base.external 2 -weight 1
    grid rowconf $base.external 3 -weight 1
}

proc test:Start {conns elemSize elemNum requests responses headers dnsaddr session descreq trQSize uri dnsresult desctimeout resptimeout pingtimeout} {
    if {$conns == "" || $conns == 0} {
        tk_dialog .err "Error" "Enter value for max connections" "" 0 "Ok"
        return
    }
    if {$elemSize == "" || $elemSize == 0} {
        tk_dialog .err "Error" "Enter value for max element size " "" 0 "Ok"
        return
    }
    if {$elemNum == "" || $elemNum == 0} {
        tk_dialog .err "Error" "Enter value for max element number " "" 0 "Ok"
        return
    }
    if {$requests == "" || $requests == 0} {
        tk_dialog .err "Error" "Enter value formax request " "" 0 "Ok"
        return
    }
    if {$responses == "" || $responses == 0} {
        tk_dialog .err "Error" "Enter value for max response" "" 0 "Ok"
        return
    }
    if {$headers == "" || $headers == 0} {
        tk_dialog .err "Error" "Enter value for max headers" "" 0 "Ok"
        return
    }
    if {$dnsaddr == "" || $dnsaddr == 0} {
        tk_dialog .err "Error" "Enter value for DNS address" "" 0 "Ok"
        return
    }
    test.Start $conns $elemSize $elemNum $requests $responses $headers $dnsaddr $session $descreq $trQSize $uri $dnsresult $desctimeout $resptimeout $pingtimeout
}

proc test:Restart  {conns elemSize elemNum requests responses headers dnsaddr session descreq trQSize uri dnsresult desctimeout resptimeout pingtimeout} {
    global tmp

    if {$conns == "" || $conns == 0} {
        tk_dialog .err "Error" "Enter value for max connections" "" 0 "Ok"
        return
    }
    if {$elemSize == "" || $elemSize == 0} {
        tk_dialog .err "Error" "Enter value for max element size " "" 0 "Ok"
        return
    }
    if {$elemNum == "" || $elemNum == 0} {
        tk_dialog .err "Error" "Enter value for max element number " "" 0 "Ok"
        return
    }
    if {$requests == "" || $requests == 0} {
        tk_dialog .err "Error" "Enter value formax request " "" 0 "Ok"
        return
    }
    if {$responses == "" || $responses == 0} {
        tk_dialog .err "Error" "Enter value for max response" "" 0 "Ok"
        return
    }
    if {$headers == "" || $headers == 0} {
        tk_dialog .err "Error" "Enter value for max headers" "" 0 "Ok"
        return
    }
    if {$dnsaddr == "" || $dnsaddr == 0} {
        tk_dialog .err "Error" "Enter value for DNS address" "" 0 "Ok"
        return
    }

    .test.conns.connlist delete 0 end
    .test.conns.sesslist delete 0 end

    test.Restart $conns $elemSize $elemNum $requests $responses $headers $dnsaddr $session $descreq $trQSize $uri $dnsresult $desctimeout $resptimeout $pingtimeout
}

proc test:SendResponse {method Stat Phrase Cseq Public ContType ContLen uri } {
    global tmp app

    set curSel [.test.conns.connlist curselection]
    if {$curSel == ""} {
        tk_dialog .err "Error" "SendResponse:No connection selected" "" 0 "Ok"
        return
    }

    set hAppConn [lindex [.test.conns.connlist get $curSel] 1]
    set respAddl [$tmp(response).record.list get 0 end]

    test.SendResponse $method $hAppConn 0 $Stat $Phrase $Cseq $Public $ContType $ContLen $respAddl $uri
}

proc test:SendDescRequest {check1 check2 check3 check4 uri1 uri2 uri3 uri4} {

    global app tmp

    if {$app(config,maxSession) == "" || $app(config,maxSession) == 0} {
       tk_dialog .err "Error" "Enter max Sessions" "" 0 "Ok"
       return
    }
    if {$app(config,maxDescReq) == "" || $app(config,maxDescReq) == 0} {
       tk_dialog .err "Error" "Enter max describe request" "" 0 "Ok"
       return
    }
    if {$app(config,transmitQSize) == "" || $app(config,transmitQSize) == 0} {
       tk_dialog .err "Error" "Enter max transmit queue size" "" 0 "Ok"
       return
    }
    if {$app(config,maxUri) == "" || $app(config,maxUri) == 0} {
       tk_dialog .err "Error" "Enter max uris in message" "" 0 "Ok"
       return
    }
    if {$app(config,dnsMaxResults) == "" || $app(config,dnsMaxResults) == 0} {
       tk_dialog .err "Error" "Enter max dns results" "" 0 "Ok"
       return
    }
    if {$app(config,descTimeout) == "" || $app(config,descTimeout) == 0} {
       tk_dialog .err "Error" "Enter describe timeout" "" 0 "Ok"
       return
    }

    if {$check1 == 1 && $uri1 == ""} {
        tk_dialog .err "Error" "Enter URI to send the describe request" "" 0 "Ok"
        return
    }
    if {$check2 == 1 && $uri2 == ""} {
        tk_dialog .err "Error" "Enter URI to send the describe request" "" 0 "Ok"
        return
    }
    if {$check3 == 1 && $uri3 == ""} {
        tk_dialog .err "Error" "Enter URI to send the describe request" "" 0 "Ok"
        return
    }
    if {$check4 == 1 && $uri4 == ""} {
        tk_dialog .err "Error" "Enter URI to send the describe request" "" 0 "Ok"
        return
    }

    if {$check1 == 0 &&  $check2 == 0 && $check3 == 0 && $check4 == 0} {
        tk_dialog .err "Error" "Select a URI to send the describe request" "" 0 "Ok"
        return
    }

    set reqAddl [$tmp(requests).record.list get 0 end]
    set curSel [.test.conns.connlist curselection]
    if {$curSel == ""} {
        test.ConstructAndSendReq $check1 $uri1 $check2 $uri2 $check3 $uri3 $check4 $uri4 $reqAddl
        return
    }

    set hAppConn [lindex [.test.conns.connlist get $curSel] 1]

    test.SendDescRequest $hAppConn $check1 $uri1 $check2 $uri2 $check3 $uri3 $check4 $uri4 $reqAddl
}

proc test:DestructConnection {} {
    set curSel [.test.conns.connlist curselection]
    if {$curSel == ""} {
        tk_dialog .info "Info" "Select a connection to be destroyed" "" 0 "Ok"
        return
    }
    set hAppConn [lindex [.test.conns.connlist get $curSel] 1]

    test.DestructConnection $hAppConn
}

proc test:SendRequest {method uri cseq trackId accept require sessId sessTimeOut TrClientPortAEnt TrClientPortBEnt TrServerPortAEnt TrServerPortBEnt isUnicast TrDest startHTime startMTime startSTime endHTime endMTime endSTime } {

    global tmp app

    if {((![string compare $sessId ""]) && (![string compare $method "setup"])) || (![string compare $method "options"]) || (![string compare $method "describe"])} {
        set curSel [.test.conns.connlist curselection]

        if {$curSel == ""} {

            if {$app(config,maxSession) == "" || $app(config,maxSession) == 0} {
               tk_dialog .err "Error" "Enter max Sessions" "" 0 "Ok"
               return
            }
            if {$app(config,maxDescReq) == "" || $app(config,maxDescReq) == 0} {
               tk_dialog .err "Error" "Enter max describe request" "" 0 "Ok"
               return
            }
            if {$app(config,transmitQSize) == "" || $app(config,transmitQSize) == 0} {
               tk_dialog .err "Error" "Enter max transmit queue size" "" 0 "Ok"
               return
            }
            if {$app(config,maxUri) == "" || $app(config,maxUri) == 0} {
               tk_dialog .err "Error" "Enter max uris in message" "" 0 "Ok"
               return
            }
            if {$app(config,dnsMaxResults) == "" || $app(config,dnsMaxResults) == 0} {
               tk_dialog .err "Error" "Enter max dns results" "" 0 "Ok"
               return
            }
            if {$app(config,descTimeout) == "" || $app(config,descTimeout) == 0} {
               tk_dialog .err "Error" "Enter describe timeout" "" 0 "Ok"
               return
            }

            test.TestConstructConn $uri
            tk_dialog .info "Info" "Constructing a new connection. Select the new connection and send the request" "" 0 "Ok"
            return
        }
    } else {
        set curSel [.test.conns.sesslist curselection]

        if {$curSel == ""} {
            tk_dialog .info "Info" "Select a session to send the request" "" 0 "Ok"
            return
        }
    }

    if {((![string compare $sessId ""]) && (![string compare $method "setup"])) || (![string compare $method "options"]) || (![string compare $method "describe"])} {
        set hApp [lindex [.test.conns.connlist get $curSel] 1]
    } else {
        set hApp [lindex [.test.conns.sesslist get $curSel] 1]
    }
    set reqAddl [$tmp(requests).record.list get 0 end]

    test.SendRequest $method $hApp $uri $cseq $trackId $accept $require $sessId $sessTimeOut $TrClientPortAEnt $TrClientPortBEnt $TrServerPortAEnt $TrServerPortBEnt $isUnicast $TrDest $reqAddl $startHTime $startMTime $startSTime $endHTime $endMTime $endSTime
}

proc test:SetUpSession {uri} {
    global tmp
    if {$uri == ""} {
       tk_dialog .err "Error" "Enter URI to setup the connection" "" 0 "Ok"
        return
    }
    set curSelConn [.test.conns.connlist curselection]
    if {$curSelConn == ""} {
        tk_dialog .err "Error" "SetUpSession:No Connection selected" "" 0 "Ok"
        return
    }

    set curSelSess [.test.conns.sesslist curselection]
    if {$curSelSess != ""} {
        set hAppSess [lindex [.test.conns.sesslist get $curSelSess] 1]
    } else {
        set hAppSess ""
    }

    set reqAddl [$tmp(requests).record.list get 0 end]
    set hApp [lindex [.test.conns.connlist get $curSelConn] 1]
    test.SetUpSession $uri $hApp $hAppSess $reqAddl
}

proc test:PlaySession {} {
    global tmp
    set curSel [.test.conns.sesslist curselection]
    if {$curSel == ""} {
        tk_dialog .err "Error" "No Session selected" "" 0 "Ok"
        return
    }

    set reqAddl [$tmp(requests).record.list get 0 end]
    set hAppSess [lindex [.test.conns.sesslist get $curSel] 1]
    test.PlaySession $hAppSess $reqAddl
}

proc test:PauseSession {} {
    global tmp
    set curSel [.test.conns.sesslist curselection]
    if {$curSel == ""} {
        tk_dialog .err "Error" "No Session selected" "" 0 "Ok"
        return
    }

    set reqAddl [$tmp(requests).record.list get 0 end]
    set hAppSess [lindex [.test.conns.sesslist get $curSel] 1]
    test.PauseSession $hAppSess $reqAddl
}

proc test:TeardownSession {} {
    global tmp
    set curSel [.test.conns.sesslist curselection]
    if {$curSel == ""} {
        tk_dialog .err "Error" "No Session selected" "" 0 "Ok"
        return
    }

    set reqAddl [$tmp(requests).record.list get 0 end]
    set hAppSess [lindex [.test.conns.sesslist get $curSel] 1]
    test.TeardownSession $hAppSess $reqAddl
}

proc test:Stop {} {

    global tmp

    .test.conns.connlist delete 0 end
    .test.conns.sesslist delete 0 end

    test.Stop
}

# test:createMenu
# Creates the menu for the main window
# input  : none
# output : none
# return : none
proc test:createMenu {} {
    global tmp app

    set menus {tools help}
    if {$tmp(recorder)} {
        set menus [linsert $menus 2 record]
        set m(record) [record:getmenu]
    }

    # Make sure we've got all the menus here
    set m(tools) {
        { "Log File"        4 {}          logfile:open}
        {separator}
        { "Save Settings"   0 {}          "init:SaveOptions 0"}
        { "Start stack"     3 {}          "test:Start $app(config,maxConnections) $app(config,memoryElementSize) $app(config,memoryElementNum) $app(config,maxRequests) $app(config,maxResponses) $app(config,maxHeaders) $app(config,dnsAddr) $app(config,maxSession) $app(config,maxDescReq) $app(config,transmitQSize) $app(config,maxUri) $app(config,dnsMaxResults) $app(config,descTimeout) $app(config,responseTimeout) $app(config,pingTimeout) "}
        { "Restart stack"   3 {}          "test:Restart $app(config,maxConnections) $app(config,memoryElementSize) $app(config,memoryElementNum) $app(config,maxRequests) $app(config,maxResponses) $app(config,maxHeaders) $app(config,dnsAddr) $app(config,maxSession) $app(config,maxDescReq) $app(config,transmitQSize) $app(config,maxUri) $app(config,dnsMaxResults) $app(config,descTimeout) $app(config,responseTimeout) $app(config,pingTimeout) "}
        { "Stop stack"      3 {}          "test:Stop"}
        { "Exit"            0 {}          "test.Quit"}
    }
    set m(help) {
        { "About"           0 {}          "Window open .about"}
    }

    # Create the main menu and all of the sub menus from the array variable m
    menu .test.main -tearoff 0
    foreach submenu $menus {
        .test.main add cascade -label [string toupper $submenu 0 0] \
            -menu .test.main.$submenu -underline 0
        menu .test.main.$submenu -tearoff 0

        foreach item $m($submenu) {
            set txt [lindex $item 0]

            if {$txt == "separator"} {
                # Put a separator
                .test.main.$submenu add separator
            } else {
                # Put a menu item
                set under [lindex $item 1]
                set key [lindex $item 2]
                set cmd [lindex $item 3]
                .test.main.$submenu add command -label $txt -accel $key -command $cmd -underline $under
            }
        }
    }
}

