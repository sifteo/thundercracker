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
         var img = new Image(8, 8);
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

     allTileGrids = [];
     highlightTile = null;

     function TileGrid(pool, canvasId) {
       var obj = this;

       allTileGrids.push(this);

       this.pool = pool;
       this.canvas = document.getElementById(canvasId);
       this.ctx = this.canvas.getContext("2d");
       this.width = this.canvas.width / 8;
       this.height = this.canvas.height / 8;

       this.canvas.onmousemove = function(evt) {
         setTileHighlight(obj.tileAt(Math.floor(mouseX(evt) / 8),
                                     Math.floor(mouseY(evt) / 8)));
       }

       this.canvas.onmouseout = function(evt) {
         setTileHighlight(null);
       }
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
         this.ctx.drawImage(tile, x*8, y*8);

         if (highlightTile != null && tile != highlightTile) {
           // Dim this tile
           this.ctx.globalAlpha = 0.5;
           this.ctx.fillStyle = "#000";
           this.ctx.fillRect(x*8, y*8, 8, 8);
         }

       } else {
 
         // Background
         this.ctx.globalAlpha = 1.0;
         this.ctx.fillStyle = "#000";
         this.ctx.fillRect(x*8, y*8, 8, 8);
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
        */

       if (t != highlightTile) {
         var prev = highlightTile;
         highlightTile = t;

         if (prev == null || t == null) {
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
           drawTile(t);
         }
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
