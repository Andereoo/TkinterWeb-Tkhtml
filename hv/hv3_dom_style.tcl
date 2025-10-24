namespace eval hv3 { set {version($Id: hv3_dom_style.tcl,v 1.15 2007/10/17 17:45:07 danielk1977 Exp $)} 1 }

#-------------------------------------------------------------------------
# DOM Level 2 Style.
#
# This file contains the Hv3 implementation of the DOM Level 2 Style
# specification.
#
#     ElementCSSInlineStyle        (mixed into Element)
#     CSSStyleDeclaration          (mixed into ElementCSSInlineStyle)
#     CSS2Properties               (mixed into CSSStyleDeclaration)
#
# http://www.w3.org/TR/2000/REC-DOM-Level-2-Style-20001113/css.html
#
#

# Set up an array of known "simple" properties. This is used during
# DOM compilation and at runtime.
#
set ::hv3::DOM::CSS2Properties_simple(azimuth)				 azimuth
set ::hv3::DOM::CSS2Properties_simple(background-attachment) backgroundAttachment
set ::hv3::DOM::CSS2Properties_simple(background-color)		 backgroundColor
set ::hv3::DOM::CSS2Properties_simple(background-image)		 backgroundImage
set ::hv3::DOM::CSS2Properties_simple(background-position)	 backgroundPosition
set ::hv3::DOM::CSS2Properties_simple(background-repeat)	 backgroundRepeat
set ::hv3::DOM::CSS2Properties_simple(background)			 background
set ::hv3::DOM::CSS2Properties_simple(border-collapse)		 borderCollapse
set ::hv3::DOM::CSS2Properties_simple(border-color)			 borderColor
set ::hv3::DOM::CSS2Properties_simple(border-spacing)		 borderSpacing
set ::hv3::DOM::CSS2Properties_simple(border-style)			 borderStyle
set ::hv3::DOM::CSS2Properties_simple(border-top)			 borderTop
set ::hv3::DOM::CSS2Properties_simple(border-right)			 borderRight
set ::hv3::DOM::CSS2Properties_simple(border-bottom)		 borderBottom
set ::hv3::DOM::CSS2Properties_simple(border-left)			 borderLeft
set ::hv3::DOM::CSS2Properties_simple(border-top-color)		 borderTopColor
set ::hv3::DOM::CSS2Properties_simple(border-right-color)	 borderRightColor
set ::hv3::DOM::CSS2Properties_simple(border-bottom-color)	 borderBottomColor
set ::hv3::DOM::CSS2Properties_simple(border-left-color)	 borderLeftColor
set ::hv3::DOM::CSS2Properties_simple(border-top-style)		 borderTopStyle
set ::hv3::DOM::CSS2Properties_simple(border-right-style)	 borderRightStyle
set ::hv3::DOM::CSS2Properties_simple(border-bottom-style)	 borderBottomStyle
set ::hv3::DOM::CSS2Properties_simple(border-left-style)	 borderLeftStyle
set ::hv3::DOM::CSS2Properties_simple(border-top-width)		 borderTopWidth
set ::hv3::DOM::CSS2Properties_simple(border-right-width)	 borderRightWidth
set ::hv3::DOM::CSS2Properties_simple(border-bottom-width)	 borderBottomWidth
set ::hv3::DOM::CSS2Properties_simple(border-left-width)	 borderLeftWidth
set ::hv3::DOM::CSS2Properties_simple(border-width)			 borderWidth
set ::hv3::DOM::CSS2Properties_simple(border)				 border
set ::hv3::DOM::CSS2Properties_simple(bottom)				 bottom
set ::hv3::DOM::CSS2Properties_simple(caption-side)			 captionSide
set ::hv3::DOM::CSS2Properties_simple(clear)				 clear
set ::hv3::DOM::CSS2Properties_simple(clip)					 clip
set ::hv3::DOM::CSS2Properties_simple(color)				 color
set ::hv3::DOM::CSS2Properties_simple(content)				 content
set ::hv3::DOM::CSS2Properties_simple(counter-increment)	 counterIncrement
set ::hv3::DOM::CSS2Properties_simple(counter-reset)		 counterReset
set ::hv3::DOM::CSS2Properties_simple(cue-after)			 cueAfter
set ::hv3::DOM::CSS2Properties_simple(cue-before)			 cueBefore
set ::hv3::DOM::CSS2Properties_simple(cue)					 cue
set ::hv3::DOM::CSS2Properties_simple(cursor)				 cursor
set ::hv3::DOM::CSS2Properties_simple(direction)			 direction
set ::hv3::DOM::CSS2Properties_simple(display)				 display
set ::hv3::DOM::CSS2Properties_simple(elevation)			 elevation
set ::hv3::DOM::CSS2Properties_simple(empty-cells)			 emptyCells
set ::hv3::DOM::CSS2Properties_simple(float)				 cssFloat
set ::hv3::DOM::CSS2Properties_simple(font-family)			 fontFamily
set ::hv3::DOM::CSS2Properties_simple(font-size)			 fontSize
set ::hv3::DOM::CSS2Properties_simple(font-style)			 fontStyle
set ::hv3::DOM::CSS2Properties_simple(font-variant)			 fontVariant
set ::hv3::DOM::CSS2Properties_simple(font-weight)			 fontWeight
set ::hv3::DOM::CSS2Properties_simple(font)					 font
set ::hv3::DOM::CSS2Properties_simple(height)				 height
set ::hv3::DOM::CSS2Properties_simple(left)					 left
set ::hv3::DOM::CSS2Properties_simple(letter-spacing)		 letterSpacing
set ::hv3::DOM::CSS2Properties_simple(line-height)			 lineHeight
set ::hv3::DOM::CSS2Properties_simple(list-style-image)		 listStyleImage
set ::hv3::DOM::CSS2Properties_simple(list-style-position)	 listStylePosition
set ::hv3::DOM::CSS2Properties_simple(list-style-type)		 listStyleType
set ::hv3::DOM::CSS2Properties_simple(list-style)			 listStyle
set ::hv3::DOM::CSS2Properties_simple(margin-right)			 marginRight
set ::hv3::DOM::CSS2Properties_simple(margin-left)			 marginLeft
set ::hv3::DOM::CSS2Properties_simple(margin-top)			 marginTop
set ::hv3::DOM::CSS2Properties_simple(margin-bottom)		 marginBottom
set ::hv3::DOM::CSS2Properties_simple(margin)				 margin
set ::hv3::DOM::CSS2Properties_simple(marker-offset)		 markerOffset
set ::hv3::DOM::CSS2Properties_simple(marks)				 marks
set ::hv3::DOM::CSS2Properties_simple(max-height)			 maxHeight
set ::hv3::DOM::CSS2Properties_simple(max-width)			 maxWidth
set ::hv3::DOM::CSS2Properties_simple(min-height)			 minHeight
set ::hv3::DOM::CSS2Properties_simple(min-width)			 minWidth
set ::hv3::DOM::CSS2Properties_simple(orphans)				 orphans
set ::hv3::DOM::CSS2Properties_simple(outline-color)		 outlineColor
set ::hv3::DOM::CSS2Properties_simple(outline-style)		 outlineStyle
set ::hv3::DOM::CSS2Properties_simple(outline-width)		 outlineWidth
set ::hv3::DOM::CSS2Properties_simple(outline)				 outline
set ::hv3::DOM::CSS2Properties_simple(overflow)				 overflow
set ::hv3::DOM::CSS2Properties_simple(padding-top)			 paddingTop
set ::hv3::DOM::CSS2Properties_simple(padding-right)		 paddingRight
set ::hv3::DOM::CSS2Properties_simple(padding-bottom)		 paddingBottom
set ::hv3::DOM::CSS2Properties_simple(padding-left)			 paddingLeft
set ::hv3::DOM::CSS2Properties_simple(padding)				 padding
set ::hv3::DOM::CSS2Properties_simple(page-break-after)		 pageBreakAfter
set ::hv3::DOM::CSS2Properties_simple(page-break-before)	 pageBreakBefore
set ::hv3::DOM::CSS2Properties_simple(page-break-inside)	 pageBreakInside
set ::hv3::DOM::CSS2Properties_simple(pause-after)			 pauseAfter
set ::hv3::DOM::CSS2Properties_simple(pause-before)			 pauseBefore
set ::hv3::DOM::CSS2Properties_simple(pause)				 pause
set ::hv3::DOM::CSS2Properties_simple(pitch-range)			 pitchRange
set ::hv3::DOM::CSS2Properties_simple(pitch)				 pitch
set ::hv3::DOM::CSS2Properties_simple(play-during)			 playDuring
set ::hv3::DOM::CSS2Properties_simple(position)				 position
set ::hv3::DOM::CSS2Properties_simple(quotes)				 quotes
set ::hv3::DOM::CSS2Properties_simple(richness)				 richness
set ::hv3::DOM::CSS2Properties_simple(right)				 right
set ::hv3::DOM::CSS2Properties_simple(size)					 size
set ::hv3::DOM::CSS2Properties_simple(speak-header)			 speakHeader
set ::hv3::DOM::CSS2Properties_simple(speak-numeral)		 speakNumeral
set ::hv3::DOM::CSS2Properties_simple(speak-punctuation)	 speakPunctuation
set ::hv3::DOM::CSS2Properties_simple(speak)				 speak
set ::hv3::DOM::CSS2Properties_simple(speech-rate)			 speechRate
set ::hv3::DOM::CSS2Properties_simple(stress)				 stress
set ::hv3::DOM::CSS2Properties_simple(table-layout)			 tableLayout
set ::hv3::DOM::CSS2Properties_simple(text-align)			 textAlign
set ::hv3::DOM::CSS2Properties_simple(text-decoration)		 textDecoration
set ::hv3::DOM::CSS2Properties_simple(text-indent)			 textIndent
set ::hv3::DOM::CSS2Properties_simple(text-transform)		 textTransform
set ::hv3::DOM::CSS2Properties_simple(top)					 top
set ::hv3::DOM::CSS2Properties_simple(unicode-bidi)			 unicodeBidi
set ::hv3::DOM::CSS2Properties_simple(vertical-align)		 verticalAlign
set ::hv3::DOM::CSS2Properties_simple(visibility)			 visibility
set ::hv3::DOM::CSS2Properties_simple(voice-family)			 voiceFamily
set ::hv3::DOM::CSS2Properties_simple(volume)				 volume
set ::hv3::DOM::CSS2Properties_simple(white-space)			 whiteSpace
set ::hv3::DOM::CSS2Properties_simple(widows)				 widows
set ::hv3::DOM::CSS2Properties_simple(width)				 width
set ::hv3::DOM::CSS2Properties_simple(word-spacing)			 wordSpacing
set ::hv3::DOM::CSS2Properties_simple(z-index)				 zIndex


set ::hv3::dom::code::ELEMENTCSSINLINESTYLE {
  -- A reference to the [Ref CSSStyleDeclaration] object used to access 
  -- the HTML \"style\" attribute of this document element.
  --
  dom_get style {
    list object [list ::hv3::DOM::CSSStyleDeclaration $myDom $myNode]
  }
}

set ::hv3::dom::code::CSS2PROPERTIES {

  dom_parameter myNode

  foreach {k v} [array get ::hv3::DOM::CSS2Properties_simple] {
    if {$v eq ""} { set v $k }
    dom_get $v "
      CSSStyleDeclaration.getStyleProperty \$myNode $k
    "
    dom_put -string $v value "
      CSSStyleDeclaration.setStyleProperty \$myNode $k \$value
    "
  }
  unset -nocomplain k
  unset -nocomplain v

  dom_put -string border value {
    set style [$myNode attribute -default {} style]
    if {$style ne ""} {append style ";"}
    append style "border: $value"
    $myNode attribute style $style
  }

  dom_put -string background value {
    array set current [$myNode prop -inline]
    unset -nocomplain current(background-color)
    unset -nocomplain current(background-image)
    unset -nocomplain current(background-repeat)
    unset -nocomplain current(background-attachment)
    unset -nocomplain current(background-position)
    unset -nocomplain current(background-position-y)
    unset -nocomplain current(background-position-x)

    set style "background:$value;"
    foreach prop [array names current] {
      append style "$prop:$current($prop);"
    }
    $myNode attribute style $style
  }
}

# In a complete implementation of the DOM Level 2 style for an HTML 
# browser, the CSSStyleDeclaration interface is used for two purposes:
#
#     * As the ElementCSSInlineStyle.style property object. This 
#       represents the contents of an HTML "style" attribute.
#
#     * As part of the DOM representation of a parsed stylesheet 
#       document. Hv3 does not implement this function.
#
::hv3::dom2::stateless CSSStyleDeclaration {
  %CSS2PROPERTIES%

  # cssText attribute - access the text of the style declaration. 
  # TODO: Setting this to a value that does not parse is supposed to
  # throw a SYNTAX_ERROR exception.
  #
  dom_get cssText { list [$myNode attribute -default "" style] }
  dom_put -string cssText val { 
    $myNode attribute style $val
  }

  dom_call_todo getPropertyValue
  dom_call_todo getPropertyCSSValue
  dom_call_todo removeProperty
  dom_call_todo getPropertyPriority

  dom_call -string setProperty {THIS propertyName value priority} {
    if {[info exists ::hv3::DOM::CSS2Properties_simple($propertyName)]} {
      CSSStyleDeclaration_setStyleProperty $myNode $propertyName $value
      return
    }
    error "DOMException SYNTAX_ERROR {unknown property $propertyName}"
  }
  
  # Interface to iterate through property names:
  #
  #     readonly unsigned long length;
  #     DOMString              item(in unsigned long index);
  #
  dom_get length {
    list [expr {[llength [$myNode prop -inline]]/2}]
  }
  dom_call item {THIS index} {
    set idx [expr {2*int([lindex $index 1])}]
    list [lindex [$myNode prop -inline] $idx]
  }

  # Read-only parentRule property. Always null in hv3.
  #
  dom_get parentRule { list null }
}

namespace eval ::hv3::DOM {
  proc CSSStyleDeclaration.getStyleProperty {node css_property} {
    set val [$node property -inline $css_property]
    list $val
  }

  proc CSSStyleDeclaration.setStyleProperty {node css_property value} {
    array set current [$node prop -inline]

    if {$value ne ""} {
      set current($css_property) $value
    } else {
      unset -nocomplain current($css_property)
    }

    set style ""
    foreach prop [array names current] {
      append style "$prop:$current($prop);"
    }

    $node attribute style $style
  }
}
