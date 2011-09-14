#!/usr/bin/env python
#
# Generator script for proof_html.cpp
#
# STIR -- Sifteo Tiled Image Reducer
# M. Elizabeth Scott <beth@sifteo.com>
#
# Copyright <c> 2011 Sifteo, Inc. All rights reserved.
#

header = """
<!DOCTYPE html>
<html>
<head> 
  <script> 
 
     /*
      * Define a tile pool (per-group)
      */

     function defineTiles(prefix, tiles) {
       for (var i = 0; i < tiles.length; i++) {

         // Load it using a Data URI and anonymous img element
         var img = new Image();
         img.src = prefix + tiles[i];

         // Keep a list of closures that can be used to redraw
         // every occurrance of this tile in the TileGrids.
         img.tgRedraw = []
 
        tiles[i] = img;
       }
       return tiles;
     }

     /*
      * Object for a single TileGrid, with some interactive features.
      * Renders onto an HTML5 Canvas
      */

     allTileGrids = {};
     highlightTile = null;
     nextHighlightTile = null;

     function TileGrid(pool, canvasId, tileSize) {
       var obj = this;

       allTileGrids[canvasId] = this;

       this.pool = pool;
       this.canvas = document.getElementById("i" + canvasId);
       this.ctx = this.canvas.getContext("2d");

       this.size = tileSize;
       this.width = this.canvas.width / this.size;
       this.height = this.canvas.height / this.size;
       this.mouse = null

       this.canvas.onmousemove = function(evt) {
         obj.updateMouse([Math.floor(mouseX(evt) / obj.size),
                          Math.floor(mouseY(evt) / obj.size)]);
       }

       this.canvas.onmouseout = function(evt) {
         obj.updateMouse(null);
       }
     }

     TileGrid.prototype.updateMouse = function(mouse) {
       if (mouse)
         setTileHighlight(this.tileAt(mouse[0], mouse[1]));
       else
         setTileHighlight(null);
     }

     TileGrid.prototype.range = function(begin, end) {
       var a = [];
       for (var i = begin; i < end; i++)
         a[i - begin] = i;
       this.array(a);
     }

     TileGrid.prototype.array = function(a) {
       this.tiles = a;
     }

     TileGrid.prototype.draw = function() {
       /*
        * Draw the whole TileGrid unconditionally
        */

       for (var y = 0; y < this.height; y++)
         for (var x = 0; x < this.width; x++)
           this.drawTile(x, y);
     }

     TileGrid.prototype.installDrawHandlers = function() {
       /*
        * Install tile redraw handlers for every tile in the TileGrid
        */

       for (var y = 0; y < this.height; y++)
         for (var x = 0; x < this.width; x++)
           var f = function (obj, x, y) {
             var t = obj.tileAt(x, y);

             if (t) {
               t.tgRedraw.push(function() { obj.drawTile(x, y); })
             }

           } (this, x, y);
     }

     TileGrid.prototype.tileAt = function(x, y) {
       /*
        * Returns the tile at location (x,y), if any.
        */

       var i = x + y * this.width;
       if (i < this.tiles.length)
         return this.pool[this.tiles[i]];
     }

     TileGrid.prototype.drawTile = function(x, y) {
       /*
        * (Re)draw a single tile at a specified location on this grid.
        */

       var tile = this.tileAt(x, y);
       if (tile) {

         this.ctx.globalAlpha = 1.0;
         this.ctx.drawImage(tile, 0, 0, tile.width, tile.height, 
                            x*this.size, y*this.size, this.size, this.size);

         if (highlightTile != null && tile != highlightTile) {
           // Dim this tile
           this.ctx.globalAlpha = 0.5;
           this.ctx.fillStyle = "#000";
           this.ctx.fillRect(x*this.size, y*this.size, this.size, this.size);
         }

       } else {
 
         // Background
         this.ctx.globalAlpha = 1.0;
         this.ctx.fillStyle = "#222";
         this.ctx.fillRect(x*this.size, y*this.size, this.size, this.size);
       }
     }

     function onload() {
       /*
        * Draw all TileGrids after our images have loaded
        */

       for (var i in allTileGrids) {
         allTileGrids[i].installDrawHandlers();
         allTileGrids[i].draw();
       }
     }

     function drawTile(t) {
       /*
        * Redraw a single tile, on every grid.
        */

       if (t) {
         for (var i in t.tgRedraw)
           t.tgRedraw[i]();
       }
     }

     function setTileHighlight(t) {
       /*
        * Change the tile that's currently highlighted.
        *
        * We do this lazily via a timer callback, to optimize
        * out changes that are occurring faster than we can draw them,
        * or spurious mouse-out events that occur before moving into a
        * different grid.
        */

       if (nextHighlightTile != t) {
         nextHighlightTile = t;

         setTimeout(function() {

           if (nextHighlightTile != highlightTile) {
             var prev = highlightTile;
             highlightTile = nextHighlightTile;

             if (prev == null || highlightTile == null) {
               /*
                * Transitions from highlighted to not highlighted or vice versa
                * require a full draw, since we're dimming all non-hilighted tiles.
                */
          
               for (var i in allTileGrids)
                 allTileGrids[i].draw();

             } else {
               /*
                * Only draw the affected tiles
                */

               drawTile(prev);
               drawTile(highlightTile);
             }
           }
         }, 0);
       }
     }

     function mouseX(evt) {
       var x;

       if (evt.pageX)
         x = evt.pageX;
       else
         x = evt.clientX + document.body.scrollLeft + document.documentElement.scrollLeft;

       return x - evt.target.offsetLeft;
     }

     function mouseY(evt) {
       var y;

       if (evt.pageY)
         y = evt.pageY;
       else
         y = evt.clientY + document.body.scrollTop + document.documentElement.scrollTop;

       return y - evt.target.offsetTop;
     }

     // Keep track of a global 'mode' for each multi-frame asset to display in.
     displayMode = {}
     animFrame = {}
     animTimers = {}

     function toggleDisplayMode(firstId, idCount, mode) {
       if (displayMode[firstId] == mode)
         mode = null;

       displayMode[firstId] = mode;

       if (mode == "anim" || mode == "all") {
         // Animation sequence only resets on "play". When paused, we intentionally
         // keep the last previous frame visible. (Play/Pause behaviour)
         animFrame[firstId] = 0;
       }

       document.getElementById("buttonAll" + firstId).className = (mode == "all") ? "button buttonOn" : "button";
       document.getElementById("buttonAnim" + firstId).className = (mode == "anim") ? "button buttonOn" : "button";

       updateVisibility(firstId, idCount);

       if (animTimers[firstId]) {
         clearInterval(animTimers[firstId]);
         animTimers[firstId] = null;
       }
       if (mode == "anim") {
         animTimers[firstId] = setInterval(function() { updateVisibility(firstId, idCount); }, 100);
       }
     }

     function updateVisibility(firstId, idCount) {
       var mode = displayMode[firstId];
       var frame = animFrame[firstId];

       if (mode == "anim") {
         frame = (frame + 1) % idCount;
         animFrame[firstId] = frame;
       }

       for (var i = 0; i < idCount; i++) {
         var visible = mode == "all" ? true : frame == i;
         document.getElementById("i" + (firstId + i)).style.display = visible ? "inline" : "none";
       }
     }

  </script> 
  <style> 
 
    body { 
      color: #eee; 
      background: #222; 
      font-family: verdana, tahoma, helvetica, arial, sans-serif; 
      font-size: 12px; 
      margin: 10px 5px 50px 5px; 
    } 
  
    h1 { 
      background: #fff; 
      color: #222; 
      font-size: 22px; 
      font-weight: normal; 
      padding: 10px; 
      margin: 0; 
    } 
 
    h2 { 
      color: #fff; 
      font-size: 16px; 
      font-weight: normal; 
      padding: 10px; 
      margin: 0; 
    } 
 
    p { 
      padding: 0 10px; 
    } 

    canvas {
      padding: 0;
      margin: 0;
    }

    span.button {
      -webkit-user-select: none;
      -khtml-user-select: none;
      -moz-user-select: none;
      -o-user-select: none;
      user-select: none;
      cursor: pointer;

      vertical-align: middle;
      font-size: 12px; 
      margin: 0 0 0 2px;
      padding: 1px 5px;
      background: #555;
      color: #000;
    }

    span.buttonOn {
      background: #fb7;
    }
 
    span.button:hover {
      background: #888;
    }

    span.buttonOn:hover {
      background: #fda;
    }

    span.button:first-child {
      margin: 0 0 0 12px;
    }

  </style> 
</head> 
<body onload="onload()">
"""

cppTemplate = """
/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * AUTOMATICALLY GENERATED by proof_html.py
 * Do not edit by hand!
 */

#include "proof.h"

const char *Stir::ProofWriter::header =
%s;
"""

###########################################################

import sys

def cString(s):
    s = s.replace('\\', '\\\\')
    s = s.replace('"', '\\"')
    lines = s.split('\n')
    return '\n'.join(['    "%s\\n"' % l for l in lines])

f = open(sys.argv[1], 'w')
f.write(cppTemplate.lstrip() % (
        cString(header.lstrip()),
        ))
