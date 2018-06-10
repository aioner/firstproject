#############################################################################################c#
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
#                                 TRTSP_stub.tcl
#
# This file is only sourced when running in TCL-only mode. it contains some functions to
# replace C implemented functions, also the remote operated stack mini-protocol.
#
#
#
##############################################################################################



##############################################################################################
#
#   MAIN WINDOW operations
#
##############################################################################################


# test:Quit
# This procedure destructs the main window of the test application
# input  : none
# output : none
# return : none
proc test.Quit {} {
    global tmp

    init:SaveOptions 0

    if {$tmp(remote,sock) != ""} {
        sendData "callCmd {test.Quit}"
        catch {fileevent $tmp(remote,sock) readable ""}

        after 100

        catch { close $tmp(remote,sock) }
        set tmp(remote,sock) ""
    }
    exit
}



##############################################################################################
#
# tools
#
##############################################################################################


proc Status.Display {} {
    status:Clean
    for {set i 0} {$i < 30} {incr i} {status:InsertLine TEST TEST$i [expr $i*2] [expr $i*3] [expr $i*4]}
    status:InsertLine {}   TIMER {} [clock seconds] {}
    status:DrawGraphs
}



##############################################################################################
#
# VTCL functions
#
##############################################################################################


proc vTclWindow. {base} {}
proc vTclWindow.test {base} {test:create}

# update the GUI - this is done by the C code when it is used
test:updateGui 1
after 1000 "update;wm withdraw .;wm geometry .test $app(test,size);wm deiconify .test;update;notebook:refresh test"



##############################################################################################
#
# Connection to remote stack
#
##############################################################################################


set tmp(remote,firstCommand) 1
set tmp(remote,sock) ""


# traceVariables
# This procedure adds a trace to variables of interest to the remote stack
# input  : none
# output : none
# return : none
proc traceVariables {} {
    global app

    # Set tracing on log-filter variables
    foreach name [array names app "logFilter,*"] {
        trace variable app($name) w transmitVariable
    }

    # Set tracing on specific variables
    foreach name { "config,maxListeningPorts" "config,maxConnections" "config,maxSessionsPerConn" "config,maxRequests" "config,maxResponses" "config,memoryElementSize" "config,memoryElementNum" "config,autoAnswer" "config,brokenTcp"\
         } {
        trace variable app($name) w transmitVariable
    }
}

# sendAllVariables
# This procedure sends the initial values of the traced variables to the remote stack
# input  : none
# output : none
# return : none
proc sendAllVariables {} {
    global app 

    # Set tracing on specific variables
    foreach name { "config,autoAnswer" "config,maxSession" "config,maxDescReq" "config,transmitQSize" "config,maxUri" "config,dnsMaxResults" "config,descTimeout" "config,responseTimeout" "config,pingTimeout"
        } {
        transmitVariable app $name w
        after 1
    }
}


# transmitVariable
# This procedure sends a variable to the remote stack
# input  : arrName - Name of the array (app)
#          varName - Name of the array index
#          op      - Tranced operation (w)
# output : none
# return : none
proc transmitVariable { arrName varName op } {
    global app tmp

    set newValue [lindex [array get $arrName $varName] 1]
    set varUpdateStr "varUpd {$varName} {$newValue}"
    sendData $varUpdateStr
}


# test.Connect
# This procedure connects to the remote stack
# input  : stackIp - the IP address of the stack application
# output : none
# return : none
proc test.Connect {stackIp} {
    global tmp app

    if {$stackIp == ""} {
       tk_dialog .err "Error" "No IP address provided" "" 0 "Ok"
       return
	}

    test:Log {Connecting to port 3046};
    update

    set sock [socket $stackIp 3049]
    fconfigure $sock -buffering none
    fconfigure $sock -blocking 0

    # Set up a callback for when we receive data
    fileevent $sock readable "recvData"
    set tmp(remote,sock) $sock

    transmitVariable app "config,stackIp" w

    test:Start $app(config,maxConnections) $app(config,memoryElementSize) $app(config,memoryElementNum) $app(config,maxRequests) $app(config,maxResponses) $app(config,maxHeaders) $app(config,dnsAddr) $app(config,maxSession) $app(config,maxDescReq) $app(config,transmitQSize) $app(config,maxUri) $app(config,dnsMaxResults) $app(config,descTimeout) $app(config,responseTimeout) $app(config,pingTimeout)
}


# sendData
# This procedure sends raw data on the connection to the remote stack
# input  : data - raw data to send
# output : none
# return : none
proc sendData {data} {
    global tmp

    if {$tmp(remote,sock) == ""} {return "error"}

    if { [catch {puts $tmp(remote,sock) $data} err] } {
        return "error"
    }
    return ""
}


# sendData
# This procedure received commands from the remote stack and executes them
# input  : none
# output : none
# return : none
proc recvData {} {
    global tmp app

    if {[eof $tmp(remote,sock)]} {
        # TCP socket got closed...
        test:Log {!!! Connection lost !!!};
        test:Log {Exiting in 5 seconds...};
        after 4900 test.Quit
    } else {
        # We've got some incoming data to process
        if {$tmp(remote,firstCommand)} {
            sendAllVariables
            traceVariables
            set address [lindex [fconfigure $tmp(remote,sock) -peername] 1]
            wm title .test "Radvision Client RTSP Protocol toolkit v2.0.0.2 Test Application (Debug)"
            set tmp(remote,firstCommand) 0
        }
        set buf [gets $tmp(remote,sock)]
        eval $buf
    }
}


proc test.Start {conns elemSize elemNum requests responses headers dnsaddr session descreq trQSize uri dnsresult desctimeout resptimeout pingtimeout} {
    sendData "callCmd {test.Start} {$conns} {$elemSize} {$elemNum} {$requests} {$responses} {$headers} {$dnsaddr} {$session} {$descreq} {$trQSize} {$uri} {$dnsresult} {$desctimeout} {$resptimeout} {$pingtimeout}"
}

proc test.Restart  {conns elemSize elemNum requests responses headers dnsaddr session descreq trQSize uri dnsresult desctimeout resptimeout pingtimeout} {

    sendData "callCmd {test.Restart} {$conns} {$elemSize} {$elemNum} {$requests} {$responses} {$headers} {$dnsaddr} {$session} {$descreq} {$trQSize} {$uri} {$dnsresult} {$desctimeout} {$resptimeout} {$pingtimeout}" 
}

proc test.SendResponse {method hAppConn hAppSess Stat Phrase Cseq Public ContType ContLen respAddl uri } {
    sendAllVariables
    sendData "callCmd {test.SendResponse} {$method} {$hAppConn} {hAppSess} {$Stat} {$Phrase} {$Cseq} {$Public} {$ContType} {$ContLen} {$respAddl} {$uri}"
}

proc test.SendDescRequest {hAppConn check1 uri1 check2 uri2 check3 uri3 check4 uri4 reqAddl} {
    sendAllVariables
    sendData "callCmd {test.SendDescRequest} {$hAppConn} {$check1} {$uri1} {$check2} {$uri2} {$check3} {$uri3} {$check4} {$uri4} {$reqAddl}"
}

proc test.ConstructAndSendReq {check1 uri1 check2 uri2 check3 uri3 check4 uri4 reqAddl} {
    sendData "callCmd {test.ConstructAndSendReq} {} {$check1} {$uri1} {$check2} {$uri2} {$check3} {$uri3} {$check4} {$uri4} {$reqAddl}"
}

proc test.DestructConnection {hAppConn} {
    sendData "callCmd {test.DestructConnection} {$hAppConn}"
}

proc test.SendRequest {method hApp uri cseq trackId accept require sessId sessTimeOut TrClientPortAEnt TrClientPortBEnt TrServerPortAEnt TrServerPortBEnt isUnicast TrDest reqAddl startHTime startMTime startSTime endHTime endMTime endSTime } {
    sendAllVariables
    sendData "callCmd {test.SendRequest} {$method} {$hApp} {$uri} {$cseq} {$trackId} {$accept} {$require} {$sessId} {$sessTimeOut} {$TrClientPortAEnt} {$TrClientPortBEnt} {$TrServerPortAEnt} {$TrServerPortBEnt} {$isUnicast} {$TrDest} {$reqAddl} {$startHTime} {$startMTime} {$startSTime} {$endHTime} {$endMTime} {$endSTime}" 
}

proc test.TestConstructConn {uri} {
    sendData "callCmd {test.TestConstructConn} {$uri}"
}

proc test.SetUpSession {uri hApp hAppSess reqAddl} {
    sendAllVariables
    sendData "callCmd {test.SetUpSession} {$uri} {$hApp} {$hAppSess} {$reqAddl}" 
}

proc test.PlaySession {hAppSess reqAddl} {
    sendData "callCmd {test.PlaySession} {$hAppSess} {$reqAddl}"
}

proc test.PauseSession {hAppSess reqAddl} {
    sendData "callCmd {test.PauseSession} {$hAppSess} {$reqAddl}"
}

proc test.TeardownSession {hAppSess reqAddl} {
    sendData "callCmd {test.TeardownSession} {$hAppSess} {$reqAddl}"
}

proc test.Stop {} {
    sendData "callCmd {test.Stop}"
}

proc test.updateSessionList {hAppConn} {
    sendData "callCmd {test.updateSessionList} {$hAppConn}"
}

proc test.ResetLog {} {
    sendData "callCmd {test.ResetLog}"
}

