# STList.tcl --
#
#	This file implements Scrolled TList widgets
#
# Copyright (c) 1996, Expert Interface Technologies
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

tixWidgetClass tixScrolledTList {
    -classname TixScrolledTList
    -superclass tixScrolledWidget
    -method {
    }
    -flag {
    }
    -configspec {
    }
    -default {
	{.scrollbar			auto}
	{*borderWidth			1}
	{*tlist.background		#c3c3c3}
	{*tlist.highlightBackground	#d9d9d9}
	{*tlist.relief			sunken}
	{*tlist.takeFocus		1}
	{*Scrollbar.background		#d9d9d9}
	{*Scrollbar.troughColor		#c3c3c3}
	{*Scrollbar.takeFocus		0}
	{*Scrollbar.relief		sunken}
	{*Scrollbar.width		15}
    }
}

proc tixScrolledTList:ConstructWidget {w} {
    upvar #0 $w data

    tixChainMethod $w ConstructWidget

    set data(w:tlist) \
	[tixTList $w.tlist]
    set data(w:hsb) \
	[scrollbar $w.hsb -orient horizontal]
    set data(w:vsb) \
	[scrollbar $w.vsb -orient vertical ]

    set data(pw:client) $data(w:tlist)
}

proc tixScrolledTList:SetBindings {w} {
    upvar #0 $w data

    tixChainMethod $w SetBindings

    $data(w:tlist) config \
	-xscrollcommand "$data(w:hsb) set"\
	-yscrollcommand "$data(w:vsb) set"\
	-sizecmd "tixScrolledWidget:Configure $w"

    $data(w:hsb) config -command "$data(w:tlist) xview"
    $data(w:vsb) config -command "$data(w:tlist) yview"
}

#----------------------------------------------------------------------
#
#		option configs
#----------------------------------------------------------------------
proc tixScrolledTList:config-takefocus {w value} {
    upvar #0 $w data
  
    $data(w:tlist) config -takefocus $value
}	

#----------------------------------------------------------------------
#
#		Widget commands
#----------------------------------------------------------------------


#----------------------------------------------------------------------
#
#		Private Methods
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# virtual functions to query the client window's scroll requirement
#----------------------------------------------------------------------
proc tixScrolledTList:GeometryInfo {w mW mH} {
    upvar #0 $w data

    return [$data(w:tlist) geometryinfo $mW $mH]
}
