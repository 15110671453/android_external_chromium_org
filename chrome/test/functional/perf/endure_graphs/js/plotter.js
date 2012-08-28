/*
  Copyright (c) 2012 The Chromium Authors. All rights reserved.
  Use of this source code is governed by a BSD-style license that can be
  found in the LICENSE file.
*/

/**
 * @fileoverview Collection of functions and classes used to plot data in a
 *     <canvas>.  Create a Plotter() to generate a plot.
 */

/**
 * Adds commas to a given number.
 *
 * Examples:
 *  1234.56 => "1,234.56"
 *  99999 => "99,999"
 *
 * @param {string|number} number The number to format.
 * @return {string} String representation of |number| with commas for every
 *     three digits to the left of a decimal point.
 */
function addCommas(number) {
  number += '';  // Convert number to string if not already a string.
  var numberParts = number.split('.');
  var integralPart = numberParts[0];
  var fractionalPart = numberParts.length > 1 ? '.' + numberParts[1] : '';
  var reThreeDigits = /(\d+)(\d{3})/;
  while (reThreeDigits.test(integralPart))
    integralPart = integralPart.replace(reThreeDigits, '$1' + ',' + '$2');
  return integralPart + fractionalPart;
}

/**
 * Vertical marker to highlight data points that are being hovered over by the
 * mouse.
 *
 * @param {string} color The color to make the marker, e.g., 'rgb(100,80,240)'.
 * @return {Element} A div Element object representing the vertical marker.
 */
function VerticalMarker(color) {
  var m = document.createElement('div');
  m.style.backgroundColor = color;
  m.style.opacity = '0.3';
  m.style.position = 'absolute';
  m.style.left = '-2px';
  m.style.top = '-2px';
  m.style.width = '0px';
  m.style.height = '0px';
  return m;
}

/**
 * Class representing a horizontal marker at the indicated mouse location.
 * @constructor
 *
 * @param {Object} canvasRect The canvas bounds (in client coords).
 * @param {Number} yPixelClicked The vertical mouse click location that spawned
 *     the marker, in the client coordinate space.
 * @param {Number} yValue The data value corresponding to the vertical click
 *     location.
 * @param {Number} yOtherValue If the plot is overlaying two coordinate systems,
 *     this is the data value corresponding to the vertical click location in
 *     the second coordinate system.  Can be null.
 */
function HorizontalMarker(canvasRect, yPixelClicked, yValue, yOtherValue) {
  var m = document.createElement('div');
  m.style.backgroundColor = HorizontalMarker.COLOR;
  m.style.opacity = '0.3';
  m.style.position = 'absolute';
  m.style.left = canvasRect.offsetLeft;
  var h = HorizontalMarker.HEIGHT;
  m.style.top = (yPixelClicked + document.body.scrollTop - (h / 2)).toFixed(0) +
                'px';
  m.style.width = canvasRect.offsetWidth + 'px';
  m.style.height = h + 'px';

  this.markerDiv = m;
  this.value = yValue;
  this.otherValue = yOtherValue;
}

HorizontalMarker.HEIGHT = 5;
HorizontalMarker.COLOR = 'rgb(0,100,100)';

/**
 * Removes the horizontal marker from the graph.
 */
HorizontalMarker.prototype.remove = function() {
  this.markerDiv.parentNode.removeChild(this.markerDiv);
};

/**
 * Main class that does the actual plotting.
 *
 * Draws a chart using a canvas element. Takes an array of lines to draw.
 * @constructor
 *
 * @param {Array} plotData list of arrays that represent individual lines. The
 *     line itself is an Array of points.
 * @param {Array} dataDescriptions list of data descriptions for each line in
 *     |plotData|.
 * @param {string} eventName The string name of an event to overlay on the
 *     graph.  Should be 'null' if there are no events to overlay.
 * @param {Object} eventInfo If |eventName| is specified, an array of event
 *     points to overlay on the graph.  Each event point in the array is itself
 *     a 2-element array, where the first element is the x-axis value at which
 *     the event occurred during the test, and the second element is a
 *     dictionary of kay/value pairs representing metadata associated with the
 *     event.
 * @param {string} unitsX The x-axis units of the data being plotted.
 * @param {string} unitsY The y-axis units of the data being plotted.
 * @param {string} unitsYOther If another graph (with different y-axis units) is
 *    being overlayed over the first graph, this represents the units of the
 *    other graph.  Otherwise, this should be 'null'.
 * @param {string} resultNode A DOM Element object representing the DOM node to
 *     which the plot should be attached.
 * @param {boolean} Whether or not the graph should be drawn in 'lookout' mode,
 *     which is a summarized view that is made for overview pages when the graph
 *     is drawn in a more confined space.
 *
 * Example of the |plotData|:
 *  [
 *    [line 1 data],
 *    [line 2 data]
 *  ].
 *  Line data looks like  [[point one], [point two]].
 *  And individual points are [x value, y value]
 */
function Plotter(plotData, dataDescriptions, eventName, eventInfo, unitsX,
                 unitsY, unitsYOther, resultNode, is_lookout) {
  this.plotData_ = plotData;
  this.dataDescriptions_ = dataDescriptions;
  this.eventName_ = eventName;
  this.eventInfo_ = eventInfo;
  this.unitsX_ = unitsX;
  this.unitsY_ = unitsY;
  this.unitsYOther_ = unitsYOther;
  this.resultNode_ = resultNode;
  this.is_lookout_ = is_lookout;

  this.dataColors_ = [];

  this.coordinates = null;
  this.coordinatesOther = null;
  if (this.unitsYOther_) {
    // Need two different coordinate systems to overlay on the same graph.
    this.coordinates = new Coordinates([plotData[0]]);
    this.coordinatesOther = new Coordinates([plotData[1]]);
  } else {
    this.coordinates = new Coordinates(plotData);
  }

  // A color palette that's unambigous for normal and color-deficient viewers.
  // Values are (red, green, blue) on a scale of 255.
  // Taken from http://jfly.iam.u-tokyo.ac.jp/html/manuals/pdf/color_blind.pdf.
  this.colors = [[0, 114, 178],   // Blue.
                 [230, 159, 0],   // Orange.
                 [0, 158, 115],   // Green.
                 [204, 121, 167], // Purplish pink.
                 [86, 180, 233],  // Sky blue.
                 [213, 94, 0],    // Dark orange.
                 [0, 0, 0],       // Black.
                 [240, 228, 66]   // Yellow.
                ];

  for (var i = 0, colorIndex = 0; i < this.dataDescriptions_.length; ++i)
    this.dataColors_[i] = this.makeColor(colorIndex++);
}

/**
 * Generates a string representing a color corresponding to the given index
 * in a color array.  Handles wrapping around the color array if necessary.
 *
 * @param {number} i An index into the |this.colors| array.
 * @return {string} A string representing a color in 'rgb(X,Y,Z)' format.
 */
Plotter.prototype.makeColor = function(i) {
  var index = i % this.colors.length;
  return 'rgb(' + this.colors[index][0] + ',' +
                  this.colors[index][1] + ',' +
                  this.colors[index][2] + ')';
};

/**
 * Same as function makeColor above, but also takes a transparency value
 * indicating how transparent to make the color appear.
 *
 * @param {number} i An index into the |this.colors| array.
 * @param {number} transparencyPercent Percentage transparency to make the
 *     color, e.g., 0.75.
 * @return {string} A string representing a color in 'rgb(X,Y,Z,A)' format,
 *     where A is the percentage transparency.
 */
Plotter.prototype.makeColorTransparent = function(i, transparencyPercent) {
  var index = i % this.colors.length;
  return 'rgba(' + this.colors[index][0] + ',' +
                   this.colors[index][1] + ',' +
                   this.colors[index][2] + ',' + transparencyPercent + ')';
};

/**
 * Gets the data color value associated with a specified color index.
 *
 * @param {number} i An index into the |this.colors| array.
 * @return {string} A string representing a color in 'rgb(X,Y,Z,A)' format,
 *     where A is the percentage transparency.
 */
Plotter.prototype.getDataColor = function(i) {
  if (this.dataColors_[i])
    return this.dataColors_[i];
  else
    return this.makeColor(i);
};

/**
 * Does the actual plotting.
 */
Plotter.prototype.plot = function() {
  this.canvasElement_ = this.canvas_();
  this.rulerDiv_ = this.ruler_();

  // Markers for the result point(s)/events that the mouse is currently
  // hovering over.
  this.cursorDiv_ = new VerticalMarker('rgb(100,80,240)');
  this.cursorDivOther_ = new VerticalMarker('rgb(50,50,50)');
  this.eventDiv_ = new VerticalMarker('rgb(255, 0, 0)');

  this.resultNode_.appendChild(this.canvasElement_);
  this.resultNode_.appendChild(this.coordinates_());
  this.resultNode_.appendChild(this.rulerDiv_);
  this.resultNode_.appendChild(this.cursorDiv_);
  this.resultNode_.appendChild(this.cursorDivOther_);
  this.resultNode_.appendChild(this.eventDiv_);
  this.attachEventListeners_();

  // Now draw the canvas.
  var ctx = this.canvasElement_.getContext('2d');

  // Clear it with white: otherwise canvas will draw on top of existing data.
  ctx.clearRect(0, 0, this.canvasElement_.width, this.canvasElement_.height);

  // Draw all data lines.
  for (var i = 0; i < this.plotData_.length; ++i) {
    var coordinateSystem = this.coordinates;
    if (i > 0 && this.unitsYOther_)
      coordinateSystem = this.coordinatesOther;
    this.plotLine_(ctx, this.getDataColor(i), this.plotData_[i],
                   coordinateSystem);
  }

  // Draw events overlayed on graph if needed.
  if (this.eventName_ && this.eventInfo_)
    this.plotEvents_(ctx, 'rgb(255, 150, 150)', this.coordinates);

  this.graduation_divs_ = this.graduations_(this.coordinates, 0, false);
  if (this.unitsYOther_) {
    this.graduation_divs_ = this.graduation_divs_.concat(
        this.graduations_(this.coordinatesOther, 1, true));
  }
  for (var i = 0; i < this.graduation_divs_.length; ++i)
    this.resultNode_.appendChild(this.graduation_divs_[i]);
};

/**
 * Draws events overlayed on top of an existing graph.
 *
 * @param {Object} ctx A canvas element object for drawing.
 * @param {string} strokeStyles A string representing the drawing style.
 * @param {Object} coordinateSystem A Coordinates object representing the
 *     coordinate system of the graph.
 */
Plotter.prototype.plotEvents_ = function(ctx, strokeStyles, coordinateSystem) {
  ctx.strokeStyle = strokeStyles;
  ctx.fillStyle = strokeStyles;
  ctx.lineWidth = 1.0;

  ctx.beginPath();
  var data = this.eventInfo_;
  for (var index = 0; index < data.length; ++index) {
    var event_time = data[index][0];
    var x = coordinateSystem.xPixel(event_time);
    ctx.moveTo(x, 0);
    ctx.lineTo(x, this.canvasElement_.offsetHeight);
  }
  ctx.closePath();
  ctx.stroke();
};

/**
 * Draws a line on the graph.
 *
 * @param {Object} ctx A canvas element object for drawing.
 * @param {string} strokeStyles A string representing the drawing style.
 * @param {Array} data A list of [x, y] values representing the line to plot.
 * @param {Object} coordinateSystem A Coordinates object representing the
 *     coordinate system of the graph.
 */
Plotter.prototype.plotLine_ = function(ctx, strokeStyles, data,
                                       coordinateSystem) {
  ctx.strokeStyle = strokeStyles;
  ctx.fillStyle = strokeStyles;
  ctx.lineWidth = 2.0;

  ctx.beginPath();
  var initial = true;
  var allPoints = [];
  for (var i = 0; i < data.length; ++i) {
    var pointX = parseFloat(data[i][0]);
    var pointY = parseFloat(data[i][1]);
    var x = coordinateSystem.xPixel(pointX);
    var y = 0.0;
    if (isNaN(pointY)) {
      // Re-set 'initial' if we're at a gap in the data.
      initial = true;
    } else {
      y = coordinateSystem.yPixel(pointY);
      if (initial)
        initial = false;
      else
        ctx.lineTo(x, y);
    }

    ctx.moveTo(x, y);
    allPoints.push([x, y]);
  }
  ctx.closePath();
  ctx.stroke();

  if (!this.is_lookout_) {
    // Draw a small dot at each point.
    for (var i = 0; i < allPoints.length; ++i) {
      ctx.beginPath();
      ctx.arc(allPoints[i][0], allPoints[i][1], 3, 0, Math.PI*2, true);
      ctx.fill();
    }
  }
};

/**
 * Attaches event listeners to DOM nodes.
 */
Plotter.prototype.attachEventListeners_ = function() {
  var self = this;
  this.canvasElement_.parentNode.addEventListener(
    'mousemove', function(evt) { self.onMouseMove_(evt); }, false);
  this.cursorDiv_.addEventListener(
    'click', function(evt) { self.onMouseClick_(evt); }, false);
  this.cursorDivOther_.addEventListener(
    'click', function(evt) { self.onMouseClick_(evt); }, false);
  this.eventDiv_.addEventListener(
    'click', function(evt) { self.onMouseClick_(evt); }, false);
};

/**
 * Update the horizontal line that is following where the mouse is hovering.
 *
 * @param {Object} evt A mouse event object representing a mouse move event.
 */
Plotter.prototype.updateRuler_ = function(evt) {
  var r = this.rulerDiv_;
  r.style.left = this.canvasElement_.offsetLeft + 'px';
  r.style.top = this.canvasElement_.offsetTop + 'px';
  r.style.width = this.canvasElement_.offsetWidth + 'px';
  var h = evt.clientY + document.body.scrollTop - this.canvasElement_.offsetTop;
  if (h > this.canvasElement_.offsetHeight)
    h = this.canvasElement_.offsetHeight;
  r.style.height = h + 'px';
};

/**
 * Update the highlighted data point at the x value that the mouse is hovering
 * over.
 *
 * @param {Object} coordinateSystem A Coordinates object representing the
 *     coordinate system of the graph.
 * @param {number} currentIndex The index into the |this.plotData| array of the
 *     data point being hovered over, for a given line.
 * @param {Object} cursorDiv A DOM element div object representing the highlight
 *     itself.
 * @param {number} dataIndex The index into the |this.plotData| array of the
 *     line being hovered over.
 */
Plotter.prototype.updateCursor_ = function(coordinateSystem, currentIndex,
                                           cursorDiv, dataIndex) {
  var c = cursorDiv;
  c.style.top = this.canvasElement_.offsetTop + 'px';
  c.style.height = this.canvasElement_.offsetHeight + 'px';

  // Left point is half-way to the previous x value, unless it's the first
  // point, in which case it's the x value of the current point.
  var leftPoint = null;
  if (currentIndex == 0) {
    leftPoint = this.canvasElement_.offsetLeft +
        coordinateSystem.xPixel(this.plotData_[dataIndex][0][0]);
  }
  else {
    var left_x = this.canvasElement_.offsetLeft +
        coordinateSystem.xPixel(this.plotData_[dataIndex][currentIndex - 1][0]);
    var curr_x = this.canvasElement_.offsetLeft +
        coordinateSystem.xPixel(this.plotData_[dataIndex][currentIndex][0]);
    leftPoint = (left_x + curr_x) / 2;
  }
  c.style.left = leftPoint;

  // Width is half-way to the next x value minus the left point, unless it's
  // the last point, in which case it's the x value of the current point minus
  // the left point.
  if (currentIndex == this.plotData_[dataIndex].length - 1) {
    var curr_x = this.canvasElement_.offsetLeft +
        coordinateSystem.xPixel(this.plotData_[dataIndex][currentIndex][0]);
    c.style.width = curr_x - left_point;
  }
  else {
    var next_x = this.canvasElement_.offsetLeft +
        coordinateSystem.xPixel(this.plotData_[dataIndex][currentIndex + 1][0]);
    var curr_x = this.canvasElement_.offsetLeft +
        coordinateSystem.xPixel(this.plotData_[dataIndex][currentIndex][0]);
    c.style.width = ((next_x + curr_x) / 2) - leftPoint;
  }
};

/**
 * Update the highlighted event at the x value that the mouse is hovering over.
 *
 * @param {number} x The x-value (pixel) at which to draw the event highlight
 *     div.
 * @param {boolean} show Whether or not to show the highlight div.
 */
Plotter.prototype.updateEventDiv_ = function(x, show) {
  var c = this.eventDiv_;
  c.style.top = this.canvasElement_.offsetTop + 'px';
  c.style.height = this.canvasElement_.offsetHeight + 'px';

  if (show) {
    c.style.left = this.canvasElement_.offsetLeft + (x - 2);
    c.style.width = 8;
  } else {
    c.style.width = 0;
  }
};

/**
 * Handle a mouse move event.
 *
 * @param {Object} evt A mouse event object representing a mouse move event.
 */
Plotter.prototype.onMouseMove_ = function(evt) {
  var canvas = evt.currentTarget.firstChild;
  var positionX = evt.clientX + document.body.scrollLeft -
      this.canvasElement_.offsetLeft;
  var positionY = evt.clientY + document.body.scrollTop -
      this.canvasElement_.offsetTop;

  // Identify the index of the x value that is closest to the mouse x value.
  var xValue = this.coordinates.xValue(positionX);
  var min_diff = Math.abs(this.plotData_[0][0][0] - xValue);
  indexValueX = 0;
  for (var i = 1; i < this.plotData_[0].length; ++i) {
    var diff = Math.abs(this.plotData_[0][i][0] - xValue);
    if (diff < min_diff) {
      min_diff = diff;
      indexValueX = i;
    }
  }

  // Identify the index of the x value closest to the mouse x value for the
  // other graph being overlayed on top of the original graph, if one exists.
  if (this.unitsYOther_) {
    var xValue = this.coordinatesOther.xValue(positionX);
    var min_diff = Math.abs(this.plotData_[1][0][0] - xValue);
    var indexValueXOther = 0;
    for (var i = 1; i < this.plotData_[1].length; ++i) {
      var diff = Math.abs(this.plotData_[1][i][0] - xValue);
      if (diff < min_diff) {
        min_diff = diff;
        indexValueXOther = i;
      }
    }
  }

  // Update coordinate information displayed directly underneath the graph.
  var yValue = this.coordinates.yValue(positionY);

  this.coordinatesTd_.innerHTML =
      '<font style="color:' + this.dataColors_[0] + '">' +
      this.plotData_[0][indexValueX][0] + ' ' + this.unitsX_ + ': ' +
      addCommas(this.plotData_[0][indexValueX][1].toFixed(2)) + ' ' +
      this.unitsY_  + '</font> [hovering at ' + addCommas(yValue.toFixed(2)) +
      ' ' + this.unitsY_ + ']';

  if (this.unitsYOther_) {
    var yValue2 = this.coordinatesOther.yValue(positionY);
    this.coordinatesTdOther_.innerHTML =
      '<font style="color:' + this.dataColors_[1] + '">' +
      this.plotData_[1][indexValueXOther][0] + ' ' + this.unitsX_ + ': ' +
      addCommas(this.plotData_[1][indexValueXOther][1].toFixed(2)) + ' ' +
      (this.unitsYOther_ ? this.unitsYOther_ : this.unitsY_)  +
      '</font> [hovering at ' + addCommas(yValue2.toFixed(2)) + ' ' +
      this.unitsYOther_ + ']';
  }
  else if (this.dataDescriptions_.length > 1) {
    this.coordinatesTdOther_.innerHTML =
        '<font style="color:' + this.dataColors_[1] + '">' +
        this.plotData_[1][indexValueX][0] + ' ' + this.unitsX_ + ': ' +
        addCommas(this.plotData_[1][indexValueX][1].toFixed(2)) + ' ' +
        (this.unitsYOther_ ? this.unitsYOther_ : this.unitsY_)  + '</font>';
  }

  // If there is a horizontal marker, also display deltas relative to it.
  if (this.horizontal_marker_) {
    var baseline = this.horizontal_marker_.value;
    var delta = yValue - baseline;
    var fraction = delta / baseline;  // Allow division by 0.

    var deltaStr = (delta >= 0 ? '+' : '') + delta.toFixed(0) + ' ' +
        this.unitsY_;
    var percentStr = (fraction >= 0 ? '+' : '') + (fraction * 100).toFixed(3) +
        '%';

    this.baselineDeltasTd_.innerHTML = deltaStr + ': ' + percentStr;

    if (this.unitsYOther_) {
      var baseline = this.horizontal_marker_.otherValue;
      var yValue2 = this.coordinatesOther.yValue(positionY);
      var delta = yValue2 - baseline;
      var fraction = delta / baseline;  // Allow division by 0.

      var deltaStr = (delta >= 0 ? '+' : '') + delta.toFixed(0) + ' ' +
          this.unitsYOther_;
      var percentStr = (fraction >= 0 ? '+' : '') +
          (fraction * 100).toFixed(3) + '%';
      this.baselineDeltasTd_.innerHTML += '<br>' + deltaStr + ': ' + percentStr;
    }
  }

  this.updateRuler_(evt);
  this.updateCursor_(this.coordinates, indexValueX, this.cursorDiv_, 0);
  if (this.unitsYOther_) {
    this.updateCursor_(this.coordinatesOther, indexValueXOther,
                       this.cursorDivOther_, 1);
  }

  // If there are events displayed, see if we're hovering close to an existing
  // event on the graph, and if so, display the metadata associated with it.
  if (this.eventName_ != null && this.eventInfo_ != null) {
    var data = this.eventInfo_;
    var showed_event = false;
    var x = 0;
    for (var index = 0; index < data.length; ++index) {
      var event_time = data[index][0];
      x = this.coordinates.xPixel(event_time);
      if (positionX >= x - 10 && positionX <= x + 10) {
        var metadata = data[index][1];
        var metadata_str = "";
        for (var meta_key in metadata)
          metadata_str += meta_key + ': ' + metadata[meta_key] + ', ';
        metadata_str = metadata_str.substring(0, metadata_str.length - 2);
        this.coordinatesTdOther_.innerHTML = event_time + ' ' + this.unitsX_ +
            ': {' + metadata_str + '}';
        showed_event = true;
        this.updateEventDiv_(x, true);
        break;
      }
    }
    if (!showed_event) {
      this.coordinatesTdOther_.innerHTML =
          'move mouse close to vertical event marker';
      this.updateEventDiv_(x, false);
    }
  }
};

/**
 * Handle a mouse click event.
 *
 * @param {Object} evt A mouse event object representing a mouse click event.
 */
Plotter.prototype.onMouseClick_ = function(evt) {
  // Shift-click controls the horizontal reference line.
  if (evt.shiftKey) {
    if (this.horizontal_marker_)
      this.horizontal_marker_.remove();

    var canvasY = evt.clientY - this.canvasElement_.offsetTop;
    this.horizontal_marker_ = new HorizontalMarker(
        this.canvasElement_, evt.clientY, this.coordinates.yValue(canvasY),
        (this.coordinatesOther ? this.coordinatesOther.yValue(canvasY) : null));
    // Insert before cursor node, otherwise it catches clicks.
    this.cursorDiv_.parentNode.insertBefore(
        this.horizontal_marker_.markerDiv, this.cursorDiv_);
  }
};

/**
 * Generates and returns a list of div objects representing horizontal lines in
 * the graph that indicate y-axis values at a computed interval.
 *
 * @param {Object} coordinateSystem a Coordinates object representing the
 *     coordinate system for which the graduations should be created.
 * @param {number} colorIndex An index into the |this.colors| array representing
 *     the color to make the graduations in the event that two graphs with
 *     different coordinate systems are being overlayed on the same plot.
 * @param {boolean} isRightSide Whether or not the graduations should have
 *     right-aligned text (used when the graduations are for a second graph
 *     that is being overlayed on top of another graph).
 * @return {Array} An array of DOM Element objects representing the divs.
 */
Plotter.prototype.graduations_ = function(coordinateSystem, colorIndex,
                                          isRightSide) {
  // Don't allow a graduation in the bottom 5% of the chart or the number label
  // would overflow the chart bounds.
  var yMin = coordinateSystem.yMinValue + .05 * coordinateSystem.yValueRange();
  var yRange = coordinateSystem.yMaxValue - yMin;

  // Use the largest scale that fits 3 or more graduations.
  // We allow scales of [...,500, 250, 100, 50, 25, 10,...].
  var scale = 5000000000;
  while (scale) {
    if (Math.floor(yRange / scale) > 2) break;  // 5s.
    scale /= 2;
    if (Math.floor(yRange / scale) > 2) break;  // 2.5s.
    scale /= 2.5;
    if (Math.floor(yRange / scale) > 2) break;  // 1s.
    scale /= 2;
  }

  var graduationPosition = yMin + (scale - yMin % scale);
  var graduationDivs = [];
  while (graduationPosition < coordinateSystem.yMaxValue || yRange == 0) {
    var graduation = document.createElement('div');
    var canvasPosition;
    if (yRange == 0) {
      // Center the graduation vertically.
      canvasPosition = this.canvasElement_.offsetHeight / 2;
    } else {
      canvasPosition = coordinateSystem.yPixel(graduationPosition);
    }
    if (this.unitsYOther_) {
      graduation.style.borderTop = '1px dashed ' +
          this.makeColorTransparent(colorIndex, 0.4)
    } else {
      graduation.style.borderTop = '1px dashed rgba(0,0,0,.08)';
    }
    graduation.style.position = 'absolute';
    graduation.style.left = this.canvasElement_.offsetLeft + 'px';
    graduation.style.top = canvasPosition + this.canvasElement_.offsetTop +
        'px';
    graduation.style.width = this.canvasElement_.offsetWidth -
        this.canvasElement_.offsetLeft + 'px';
    graduation.style.paddingLeft = '4px';
    if (this.unitsYOther_)
      graduation.style.color = this.makeColorTransparent(colorIndex, 0.9)
    else
      graduation.style.color = 'rgba(0,0,0,.4)';
    graduation.style.fontSize = '9px';
    graduation.style.paddingTop = '0';
    graduation.style.zIndex = '-1';
    if (isRightSide)
      graduation.style.textAlign = 'right';
    if (yRange == 0)
      graduation.innerHTML = addCommas(yMin);
    else
      graduation.innerHTML = addCommas(graduationPosition);
    graduationDivs.push(graduation);
    if (yRange == 0)
      break;
    graduationPosition += scale;
  }
  return graduationDivs;
};

/**
 * Generates and returns a div object representing the horizontal line that
 * follows the mouse pointer around the plot.
 *
 * @return {Object} A DOM Element object representing the div.
 */
Plotter.prototype.ruler_ = function() {
  var ruler = document.createElement('div');
  ruler.setAttribute('class', 'plot-ruler');
  ruler.style.borderBottom = '1px dotted black';
  ruler.style.position = 'absolute';
  ruler.style.left = '-2px';
  ruler.style.top = '-2px';
  ruler.style.width = '0px';
  ruler.style.height = '0px';
  return ruler;
};

/**
 * Generates and returns a canvas object representing the plot itself.
 *
 * @return {Object} A DOM Element object representing the canvas.
 */
Plotter.prototype.canvas_ = function() {
  var canvas = document.createElement('canvas');
  canvas.setAttribute('id', '_canvas');
  canvas.setAttribute('class', 'plot');
  canvas.setAttribute('width', this.coordinates.widthMax);
  canvas.setAttribute('height', this.coordinates.heightMax);
  canvas.plotter = this;
  return canvas;
};

/**
 * Generates and returns a div object representing the coordinate information
 * displayed directly underneath a graph.
 *
 * @return {Object} A DOM Element object representing the div.
 */
Plotter.prototype.coordinates_ = function() {
  var coordinatesDiv = document.createElement('div');
  var table_html = '<table border=0 width="100%"';
  if (this.is_lookout_) {
    table_html += ' style="font-size:0.8em"';
  }
  table_html += '><tbody><tr>'

  table_html += '<td><span class="legend_item" style="color:' +
      this.getDataColor(0) + '">' + this.dataDescriptions_[0] +
      '</span>:&nbsp;&nbsp;<span class="plot-coordinates">' +
      '<i>move mouse over graph</i></span></td>';

  table_html += '<td align="right">x-axis is ' + this.unitsX_ + '</td>';

  table_html += '</tr><tr>'

  table_html += '<td>';
  if (this.dataDescriptions_.length > 1) {
    // A second line will be drawn, so add information about it.
    table_html += '<span class="legend_item" style="color:' +
        this.getDataColor(1) + '">' + this.dataDescriptions_[1] +
        '</span>:&nbsp;&nbsp;<span class="plot-coordinates">' +
        '<i>move mouse over graph</i></span>';
  } else if (this.eventName_ != null) {
    // Event information will be overlayed on the graph, so add info about it.
    table_html += '<span class="legend_item" style="color:' +
        this.getDataColor(1) + '">Event "' + this.eventName_ + '":</span>' +
        '&nbsp;&nbsp;<span class="plot-coordinates">' +
        '<i>move mouse over graph</i></span>';
  }
  table_html += '</td>';

  if (!this.is_lookout_) {
    table_html += '<td align="right" style="color: ' + HorizontalMarker.COLOR +
        '"><i>Shift-click to place baseline.</i></td>';
  }
  table_html += '</tr></tbody></table>';
  coordinatesDiv.innerHTML = table_html;

  var tr = coordinatesDiv.firstChild.firstChild.childNodes[0];
  this.coordinatesTd_ = tr.childNodes[0].childNodes[2];
  tr = coordinatesDiv.firstChild.firstChild.childNodes[1];
  if (this.dataDescriptions_.length > 1) {
    // For second graph line.
    this.coordinatesTdOther_ = tr.childNodes[0].childNodes[2];
  } else if (this.eventName_ != null) {
    // For event metadata.
    this.coordinatesTdOther_ = tr.childNodes[0].childNodes[2];
  }
  this.baselineDeltasTd_ = tr.childNodes[1];

  return coordinatesDiv;
};
