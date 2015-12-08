#
# HtmlViewer
# ----------------------------------------------------------------------
# Implements a html display widget for on-line help
#
#   PUBLIC ATTRIBUTES:
#
#
#   METHODS:
#
#     loadFile  ....... loads the given Html file into the viewer
#     forward  ....... moves forward up the history stack
#     back      ....... moves backward on the history stack
#     textWinToObj .... finds the object name from the inter text widget (used by html_library callbacks)
#
#     PRIVATE METHODS
#
#     previous ..... recalls the previous command
#     next ......... recalls the next command
#     last ......... recalls the last command in the history list
#     first ........ recalls the first command in the history list
#
#   X11 OPTION DATABASE ATTRIBUTES
#
#
# ----------------------------------------------------------------------
#   AUTHOR:  Jim Ingham <jingham@cygnus.com>
#   Copyright (C) 1998 Cygnus Solutions
#


itcl_class HtmlViewer {
  # ------------------------------------------------------------------
  #  CONSTRUCTOR - create new html viewer window
  # ------------------------------------------------------------------

    constructor {} {
      global GDBTK_LIBRARY gdb_ImageDir

	set imageDir $gdb_ImageDir
        set htmlHead [file join $GDBTK_LIBRARY help]
	set homePage {}
	set firstPage {}
	set args {}

        if {[pref get gdb/mode]} {
          set htmlHead [file join $htmlHead trace]
	}
	set topDir $htmlHead
	
	if {![file exists $htmlHead] || ![file isdirectory $htmlHead]} {
	    error "Html \"$htmlHead\" head does not exist"
	}
	
	findHomePage $homePage

	set class [$this info class]

	@@rename $this $this-tmp-
	@@frame $this -class $class 
	@@rename $this $this-win-
	@@rename $this-tmp- $this
	
	defarray images
	defarray components
	defarray topics

	set topLevel [winfo toplevel $this]

	#
	# First, insert whatever menus we are going to insert.
	# These are the file like menu, and the Topics menu if there is one.
	#

	if {[$topLevel cget -menu] == ""} {
	    set component(menu) [menu $this.mbar -tearoff 0]
	    set ownsMenu 1
	    $topLevel configure -menu $component(menu)
	    set component(html_menu) [menu $component(menu).html -tearoff 0]
	    $component(menu) add cascade -label File -underline 0 \
		-menu $component(html_menu)
	    $component(html_menu) add command -label Back -underline 0\
		-command [list $this stopRenderAndDo "$this back"]
	    $component(html_menu) add command -label Forward  -underline 0\
		-command [list $this stopRenderAndDo "$this forward"]
	    $component(html_menu) add command -label Home  -underline 0\
		-command [list $this stopRenderAndDo [list $this loadFile **HOME**]]
	    $component(html_menu) add separator
	    $component(html_menu) add command -label Close \
		-command [list manage delete $this]

	    if {$homePageToc != ""} {
		set component(topic_menu) [menu $component(menu).topic -tearoff 0]
		$component(menu) add cascade -label Topics -underline 0 \
		    -menu $component(topic_menu)
		processTopics $component(topic_menu)
	    }
	} else {
	    # For now, we won't do anything significant if there is already
	    # a toplevel menu, since we would only add a close...
	    # set component(menu) [$topLevel cget -menu]
	    set ownsMenu 0
	}
	
	set component(toolbar) [frame $this.tool]
        set component(back) [button $component(toolbar).back \
				 -text Back \
				 -image [image create photo -file \
					     [file join $imageDir back.gif]] \
				 -command [list $this stopRenderAndDo \
					       [list $this back]]]
        set component(fore) [button $component(toolbar).fore \
				 -text Forward \
				 -image [image create photo -file \
					     [file join $imageDir fore.gif]] \
				 -command [list $this stopRenderAndDo \
					       [list $this forward]]]
	set component(home) [button $component(toolbar).home \
				 -text Home \
				 -image [image create photo -file \
					     [file join $imageDir home.gif]] \
				 -command [list $this stopRenderAndDo \
					       [list $this loadFile **HOME**]]]
	
	standard_toolbar $component(toolbar) $component(back) \
	    $component(fore) $component(home)
	
	pack $component(toolbar) -side top -fill x
	
	frame $this.statf -relief sunken -borderwidth 2
	set component(meter) [tixMeter $this.statf.meter -value 0 -text " " \
				  -width 100 -borderwidth 1]
	pack $component(meter) -side right -padx 4 -pady 4 
	pack $this.statf -side bottom -fill x
	    
	set component(stext) [tixScrolledText $this.stext -scrollbar "auto -x"]
	
	set component(viewer) [$this.stext subwidget text]
	$component(viewer) configure -width 80

	# Init the html_library for this widget...
	
	HMinit_win $component(viewer)

	# Reset the font map to values from our preferences...

	setFonts

	#
	# Now init the tag for glossary entries.
	#

      $component(viewer) tag configure glossary \
	-foreground darkgreen -underline 1
      if {![info exists Glossary(toplevel)]} {
	debug "Initializing Glossary"
	set Glossary(toplevel) [@@toplevel .__\#glossary \
				    -borderwidth 1 -bg black]
	wm withdraw $Glossary(toplevel)
	wm overrideredirect $Glossary(toplevel) 1
	set Glossary(label) [@@label $Glossary(toplevel).lbl \
			       -justify left]
	pack $Glossary(label)
	set Glossary(viewers) 0
      }
      incr Glossary(viewers)

	pack $component(stext) -side top -fill both -expand 1
	
        set textWinToObj($component(viewer)) $this

	eval configure $args

	#
	# Now parse the firstPage info.  It can be:
	#     "" == home
	#     {topic topicName}
	#     {file fileName}
	#     fileName
	#

	if {$firstPage == ""} {
	    loadFile **HOME**
	} elseif {[llength $firstPage] == 2} {
	    if {[string compare [lindex $firstPage 0] topic] == 0} {
		loadTopic [lindex $firstPage 1]
	    } else {
		loadFile [lindex $firstPage 1]
	    }
	} else {
	    loadFile $firstPage
	}
    }

  # ------------------------------------------------------------------
  #  DESTRUCTOR - clean up after the html viewer
  # ------------------------------------------------------------------
    destructor {
	if {!$ownsMenu} {
	    # If we had installed a menu in this case, we should now remove it.
	}
	unset textWinToObj($component(viewer))
      incr Glossary(viewers) -1
      if {$Glossary(viewers) == 0} {
	destroy $Glossary(toplevel)
	unset Glossary
      }
      destroy [winfo toplevel $this]
    }


  # ------------------------------------------------------------------
  #  CLASS METHODS
  # ------------------------------------------------------------------

  # ------------------------------------------------------------------
  #  configure - This does configuration for the whole widget
  # ------------------------------------------------------------------

    method configure {args} {
	debug "Not implemented yet"
    }

  # ------------------------------------------------------------------
  #  reconfig - This is the world changed procedure - just refresh the page.
  # ------------------------------------------------------------------

    method reconfig {args} {
	setFonts
	if {$histPtr != -1} {
	    recall 0
	}
    }

  # ------------------------------------------------------------------
  #  findHomePage - finds the home page & topic file
  # ------------------------------------------------------------------

    method findHomePage {home} {

	#
	# The home page can either be: 
	# 1) an Html page, in which case I will start there, and put a Home button
        #    in the button-bar that points back to it.  In this case, you have to
        #    specify the topics by giving the href of the file.
	# 2) A list of topics, which is a file of tcl code that sets up an array
	#    of Topic-name to href mappings.  In this case, I will autogen the 
        #    home page (later) and post a topics menu in the menubar.
	#

	if {$home == ""} {
	    set homePageHtml [fullPath "index.html"]
	    if {![file exists $homePageHtml]} {
		set homePageHtml ""
	    }
	    set homePageToc [fullPath "index.toc"]
	    if {![file exists $homePageToc]} {
		set homePageToc ""
	    } 
	} else {
	    if {0 < [llength $home] < 3} {
		foreach elem $home {
		    set homePage [fullPath $elem]
		    if {![file exists $homePage]} {
			error "Topics page $homePage does not exist"
		    } elseif {[string first htm [set ext [file extension $home]]] == 1} {
			set homePageHtml $homePage
		    } elseif {[string compare $ext toc] == 0} {
			set homePageToc $homePage
		    } else {
			error "Unrecognized index type: $ext"
		    }
		}
	    } else {
		error "Bad topics argument: \"$home\""
	    }
	}

    }

  # ------------------------------------------------------------------
  #  processTopics - generate the topics menu, and the dynamic home page
  # ------------------------------------------------------------------

    method processTopics {menu} {
	set fileH [open $homePageToc r]
	
	set homePageContents {
<html>
  <head>
    <title>Help - Table of Contents</title>
  </head>

  <body>
    <h1>Help - Table of Contents</h1>
    <P><\P>
    <ul>
	}
	while {[gets $fileH line] > -1} {
	    if {[llength $line] == 3} {
		foreach {topic link desc} $line " \#empty "
		append homePageContents "\n<li><a HREF=\"$link\">$topic</a> - $desc</li>"
		set topics($topic) $link
		$menu add command -label $topic -command [list $this loadFile $link]
	    } else {
		debug "Malformed Topic line: $line"
	    }
	}
	append homePageContents {
    </ul>
  </body>
</html>
	}
	
	close $fileH
    }

  # ------------------------------------------------------------------
  #  loadFile - load the given file in the viewer window
  # ------------------------------------------------------------------

    method loadFile {href {record 1}} {

	if {[isRelative $href]} {
	    parseMarks $href file mark

	    if {$file == ""} {
		set fileName $curFile
	    } elseif {[string compare $file **HOME**] == 0} {
		set fileName $file
	    } else {
		set fileName [fullPath $file]
		if {![file exists $fileName]} {
		    reportError "Bad fileName: \"$fileName\""
		    return
		}
	    }
	    
	    doLoad $fileName $mark
	    if {$record} {
		addToHist $fileName $mark
	    }
	}
    }

  # ------------------------------------------------------------------
  #  loadTopic - load the given topic in the viewer window
  # ------------------------------------------------------------------

    method loadTopic {topic} {
	if {[info exists topics($topic)]} {
	    loadFile $topics($topic)
	} else {
	    error "No such topic: $topic"
	}
    }

  # ------------------------------------------------------------------
  #  doLoad - loads a file - mark pair
  # ------------------------------------------------------------------

    method doLoad {fileName mark} {

	if {[string compare $fileName "**HOME**"] == 0} {
	    home	    
	} elseif {[string compare $fileName $curFile] != 0} {
	    if {[catch {open $fileName r} fileH]} {
		reportError "Could not open file $fileName: $fileH"
		return
	    }
	    
	    set page [read $fileH]
	    close $fileH
	    set curFile $fileName
	    
	    insertHtml $page
	}

	if {$mark != ""} {
	    goMark $mark
	} 
    }

  # ------------------------------------------------------------------
  #  goMark - go to a mark
  # ------------------------------------------------------------------

    method goMark {mark} {
	HMgoto $component(viewer) $mark
    }

    method stopRenderAndDo {command} {
	set abortRender 1
	after 10 "$this stopAbort \n$command"
    } 

    method stopAbort {} {
	set abortRender 0
    }

  # ------------------------------------------------------------------
  #  linkCallback - called by the html_libraries HMlink_callback
  # ------------------------------------------------------------------

    method linkCallback {href} {	
	
	if {$oldCursor != ""} {
	    $component(viewer) configure -cursor $oldCursor
	}

	loadFile $href

    }

  # ------------------------------------------------------------------
  #  setImage - called by the html_libraries HMset_image
  # ------------------------------------------------------------------

    method setImage {handle imgSrc} {
	
	set fullName [fullPath $imgSrc]
	
	if {[info exists images($fullName)]} {
	    HMgot_image $handle $images($fullName)
	    return
	}
	
	if {[catch {image create photo -file $fullName} image]} {
	    debug "Could not make image from file $fullName: $image"
	} else {
	    HMgot_image $handle $image
	    set images($fullName) $image
	}
	
    }

  # ------------------------------------------------------------------
  #  linkEnter - called by the html_libraries HMlink_enter_callback
  # ------------------------------------------------------------------

    method linkEnter {href} {

	set oldCursor [$component(viewer) cget -cursor]
	$component(viewer) configure -cursor $handCursor
    }


  # ------------------------------------------------------------------
  #  linkLeave - called by the html_libraries HMlink_leave_callback
  # ------------------------------------------------------------------

    method linkLeave {href} {

	$component(viewer) configure -cursor $oldCursor
    }

  # ------------------------------------------------------------------
  #  glossaryEnter - called by the html_libraries HMglossary_enter_callback
  # ------------------------------------------------------------------

    method glossaryEnter {term} {

	set oldCursor [$component(viewer) cget -cursor]
	$component(viewer) configure -cursor $handCursor
#	debug "Glossary enter with $term"
    }


  # ------------------------------------------------------------------
  #  glossaryLeave - called by the html_libraries HMglossary_leave_callback
  # ------------------------------------------------------------------

    method glossaryLeave {term} {

	$component(viewer) configure -cursor $oldCursor
#	debug "Glossary leave with $term"
    }
  # ------------------------------------------------------------------
  #  glossaryPost - called by the html_libraries HMglossary_post_callback
  #  x & y are the click positions.
  # ------------------------------------------------------------------

    method glossaryPost {W term x y} {

#	debug "Glossary post with $term $x $y"
	
	# lookup the term
	set definition [lookup $term]
	if {$definition != {}} {
	    $Glossary(label) configure -text $definition
	    @@update idletasks
	    # round ([winfo width $W] * .75)
	    set rh   [winfo reqheight $Glossary(label)]
	    set left [expr {[winfo rootx $W] + $x}]
	    set ypos [expr {[winfo rooty $W] + $y - $rh}]
	    set alt_ypos [winfo rooty $W]

	    set lw [winfo reqwidth $Glossary(label)]
	    set sw [winfo screenwidth $this]
	    set bw [$this-win- cget -borderwidth]
	    if {$left + $lw + 2 * $bw >= $sw} {
		set left [expr {$sw - 2 * $bw - $lw}]
	    }

	    set lh [winfo reqheight $Glossary(label)]
	    if {$ypos + $lh >= [winfo screenheight $this]} {
		set ypos [expr {$alt_ypos - $lh}]
	    }
 
	    wm positionfrom $Glossary(toplevel) user
	    wm geometry $Glossary(toplevel) +${left}+${ypos}
	    wm deiconify $Glossary(toplevel)
	    raise $Glossary(toplevel)
	}
    }


  # ------------------------------------------------------------------
  #  glossaryUnpost - called by the html_libraries HMglossary_unpost_callback
  #  x & y are the release positions.
  # ------------------------------------------------------------------

    method glossaryUnpost {term x y} {

#	debug "Glossary unpost with $term $x $y"
	wm withdraw $Glossary(toplevel)
    }

    method lookup {term} {
      if {[info exists Glossary(term,$term)]} {
	return $Glossary(term,$term)
      }

      return "This is some\ntext that I put\nin here."
    }

  # ------------------------------------------------------------------
  #  reportError - report errors in the status window...
  # ------------------------------------------------------------------

    method reportError {mssg} {
	puts $mssg
    }

  # ------------------------------------------------------------------
  #  isReady - report whether the window has completed rendering...
  # ------------------------------------------------------------------

    method is_ready {} {
	return $rendering
    }

  # ------------------------------------------------------------------
  #  insertHtml - call out to the html_library to render an html string
  # ------------------------------------------------------------------

    method insertHtml {page} {

	$component(viewer) configure -state normal
	HMreset_win $component(viewer)
	$component(meter) configure -value 0 -text ""
	catch {HMparse_html $page "HMrender $component(viewer)"}
	catch {$component(meter) configure -value 0 -text " "}
	catch {$component(viewer) configure -state disabled}

    }

  # ------------------------------------------------------------------
  #  renderUpdate - called while the HTML is being rendered
  # ------------------------------------------------------------------

    method renderUpdate {numTags} {
	
	if {$abortRender} { 
	    set rendering 0
	    error 
	}

	$component(meter) configure -value [expr {1.0 * $numTags / $totTags}]
	update
    }


  # ------------------------------------------------------------------
  #  renderStart - called when you start rendering the HTML
  # ------------------------------------------------------------------

    method renderStart {tags} {
	set totTags $tags
	set rendering 1
    }

  # ------------------------------------------------------------------
  #  renderEnd - called when you finish rendering the HTML
  # ------------------------------------------------------------------

    method renderEnd {} {
	set rendering 0
    }

  # 
  # These are the routines that deal with the history list.
  #

  # ------------------------------------------------------------------
  #  addToHist - add a file reference to the history list
  # ------------------------------------------------------------------

    method addToHist {file {mark {}}} {

	if {($histPtr == -1) || \
		($histPtr == [llength $history] - 1)} {
	    lappend history [list $file $mark]
	    incr histPtr
	} else {
	  set postHit [lindex $history [expr {$histPtr + 1}]]
	    if {([string compare [lindex $postHit 0] $file] != 0) \
			 || ([string compare [lindex $postHit 1] $mark] != 0)} {
		set history \
		    [linsert $history [incr histPtr] [list $file $mark]]
	    } else {
		incr histPtr
	    }
	}	
    }

  # ------------------------------------------------------------------
  #  recall - Used to move on the history list
  # ------------------------------------------------------------------

    method recall {incr} {
	if {$incr != 0} {
	    incr histPtr $incr
	} else {
	    #
	    # This will force the widget to reread the page...
	    #

	    set curFile ""
	}

	set record [lindex $history $histPtr]
	eval doLoad $record
    }

  # ------------------------------------------------------------------
  #  back - go back in the history list
  # ------------------------------------------------------------------

    method back {} {
	if {$histPtr > 0 } {
	    $this recall -1
	} else {
	    bell
	}
    }

  # ------------------------------------------------------------------
  #  forward - go forward in the history list
  # ------------------------------------------------------------------

    method forward {} {
	if {$histPtr < [llength $history] - 1 } {
	    $this recall 1
	} else {
	    bell
	}
    }

  # ------------------------------------------------------------------
  #  home - go home...
  # ------------------------------------------------------------------

    method home {{record 1}} {
	if {$homePageHtml != ""} {
	    loadFile $homePageHtml
	    set curFile $homePageHtml
	} elseif {$homePageContents != ""} {
	    insertHtml $homePageContents
	    set curFile **HOME**
	} else {
	    bell
	}
    }

  # ------------------------------------------------------------------
  #  fullPath - given a relative fileName, return the full path
  # ------------------------------------------------------------------

    method fullPath {file} {
      
      set path [file join $topDir $file]

      return $path
    }


  # ------------------------------------------------------------------
  #  CLASS PROCS
  # ------------------------------------------------------------------


  # ------------------------------------------------------------------
  #  setFonts - Sets the fonts from the pref's
  # ------------------------------------------------------------------

    proc setFonts {} {
	global HMtag_map

	HMreset_fonts global/fixed global/default

    }
    

  # ------------------------------------------------------------------
  #  isRelative - Is this a relative path or a full URL
  # ------------------------------------------------------------------

    proc isRelative {href} {
	return 1
    }

  # ------------------------------------------------------------------
  #  textWinToObj - look up the object name given the html display window.
  #                 This is used by the html_library code.
  # ------------------------------------------------------------------

    proc textWinToObj {win} {
	if {[info exists textWinToObj($win)]} {
	    return $textWinToObj($win)
	} else {
	    error "No HtmlViewer corresponding to window $win"
	}
    }

  # ------------------------------------------------------------------
  #  VARIABLES
  # ------------------------------------------------------------------

    common textWinToObj   ;# An array to do lookup from display window to $this
    common images         ;# An array for the images that we have already read in.
    common Glossary      ;# Array used to hold data for the glossary windows

    protected topics      ;# An array: key is topic name, value is its link
    protected homePageContents "" ;# The contents of the auto-gen'ed home page.
    protected homePageHtml "" ;# This is the Html home page.
    protected homePageToc "" ;# This is the file which stores the Table of Contents for help.
    protected history     ;# The history list for previously visited pages.  It is a list
                           # Of two element lists, the first element is a filename, the second
                           # a mark in that file.  The mark is == "" for no mark.
    protected histPtr -1  ;# Pointer to the current position in the history list.
    protected abortRender  0 ;# We need to abort the process of rendering.
    protected totTags        ;# The total number of tags in the current render.
    protected rendering 0    ;# Are we in the process of rendering?
    protected component  ;# These are the important components that make this widget
    protected ownsMenu      ;# Does this widget own the toplevel's menu?
    protected handCursor hand2 ;# This is the cursor when over a link
    protected oldCursor ""  ;# The cursor in teh text window (used when we switch it to a hand)
    protected topDir        ;# The top directory for HTML
    protected home          ;# The home file (by default $topDir/index.html
    protected curFile  ""   ;# The file we are currently displaying

    proc parseMarks {href filePtr markPtr} {
	upvar $filePtr file
	upvar $markPtr mark
	
	set file $href
	set mark ""
	
	return [regexp {^(.*)#([^/]*)$} $href dummy file mark]
    }

}

#
# Make sure the html library is loaded now...
#

auto_load HMinit_win

#
# Now override the callbacks
#

proc HMlink_callback {win href} {

    [HtmlViewer @@ textWinToObj $win] linkCallback $href

}

proc HMset_image {win handle imgSrc} {
  
  if [regexp ^% $imgSrc] {
    global gdb_ImageDir

    # Got a special identifier.
    switch $imgSrc {
      %run {set imgSrc [file join $gdb_ImageDir run.gif]}
      %stop {set imgSrc [file join $gdb_ImageDir stop.gif]}
      %step {set imgSrc [file join $gdb_ImageDir step.gif]}
      %next {set imgSrc [file join $gdb_ImageDir next.gif]}
      %continue {set imgSrc [file join $gdb_ImageDir continue.gif]}
      %finish {set imgSrc [file join $gdb_ImageDir finish.gif]}
      %stepi {set imgSrc [file join $gdb_ImageDir stepi.gif]}
      %nexti {set imgSrc [file join $gdb_ImageDir nexti.gif]}
      %stack {set imgSrc [file join $gdb_ImageDir stack.gif]}
      %register {set imgSrc [file join $gdb_ImageDir reg.gif]}
      %memory {set imgSrc [file join $gdb_ImageDir memory.gif]}
      %watch {set imgSrc [file join $gdb_ImageDir watch.gif]}
      %locals {set imgSrc [file join $gdb_ImageDir vars.gif]}
      %bp {set imgSrc [file join $gdb_ImageDir bp.gif]}
      %console {set imgSrc [file join $gdb_ImageDir console.gif]}
      %up {set imgSrc [file join $gdb_ImageDir up.gif]}
      %down {set imgSrc [file join $gdb_ImageDir down.gif]}
      %bottom {set imgSrc [file join $gdb_ImageDir bottom.gif]}
      default { debug "HMset_image: unknown special image \"$imgSrc\"" }
    }
  } else {
    # perhaps windows vs unix pictures?
  }

  [HtmlViewer @@ textWinToObj $win] setImage $handle $imgSrc

}

proc HMlink_enter_callback {win href} {
    
    [HtmlViewer @@ textWinToObj $win] linkEnter $href
    
}

proc HMlink_leave_callback {win href} {
    
    [HtmlViewer @@ textWinToObj $win] linkLeave $href

    
}

proc HMdo_update {win} {
    upvar #0 HM$win var
    
   [HtmlViewer @@ textWinToObj $win] renderUpdate $var(tags)

}

proc HMrender_start {cmd tags} {

    # 
    # I am relying on the fact that the command I pass to HMparse_html is
    # HMrender $component(viewer), since HMparse_html doesn't a priori
    # know anything about the window it is rendering into.
    #

    set win [lindex $cmd 1]
    [HtmlViewer @@ textWinToObj $win] renderStart $tags

}

proc HMrender_end {cmd} {

    # 
    # I am relying on the fact that the command I pass to HMparse_html is
    # HMrender $component(viewer), since HMparse_html doesn't a priori
    # know anything about the window it is rendering into.
    #

    set win [lindex $cmd 1]
    [HtmlViewer @@ textWinToObj $win] renderEnd $tags

}


#
# The next section implements the balloon based term glossary items
#

proc HMtag_glossary {win param text} {
    upvar #0 HM$win var

    if {[HMextract_param $param term]} {
	set var(Tdefn) [list G:$term]
	HMstack $win "" "Tglossary glossary"
	HMglossary_setup $win $term
    }

}

proc HMtag_/glossary {win param text} {
    upvar #0 HM$win var

    if {[info exists var(Tdefn)]} {
	unset var(Tdefn)
	HMstack $win / "Tglossary glossary"
    }

}

proc HMglossary_setup {win term} {
    global HMevents HMhandCursor
    regsub -all {%} $term {%%} term2

    $win tag bind  G:$term <Enter> [list HMglossary_enter_callback $win $term2]
    $win tag bind  G:$term <Leave> [list HMglossary_leave_callback $win $term2]
    $win tag bind  G:$term <ButtonPress> [list HMglossary_post_callback $win $term2 %x %y]
    $win tag bind  G:$term <ButtonRelease> [list HMglossary_unpost_callback $win $term2 %x %y]

 
}

proc HMglossary_enter_callback {win term} {
    [HtmlViewer @@ textWinToObj $win] glossaryEnter $term
}

proc HMglossary_leave_callback {win term} {
    [HtmlViewer @@ textWinToObj $win] glossaryLeave $term
}

proc HMglossary_post_callback {win term x y} {
    [HtmlViewer @@ textWinToObj $win] glossaryPost $win $term $x $y
}

proc HMglossary_unpost_callback {win term x y} {
    [HtmlViewer @@ textWinToObj $win] glossaryUnpost $term $x $y
}
