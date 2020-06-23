const window   = require('svgdom');
const SVG      = require('svg.js')(window);
const document = window.document;
const polygon  = require('./polygon');


var draw = SVG(document.documentElement).size('25', '25').fill('#000');

var knob = draw.polygon(polygon.to_svg_string(polygon.calculate(0, 0, 18, 12)));
knob.move(0.5,1).fill('#988903').stroke({ color: '#e8b923', width: 0.5 });
var pointer = draw.rect(2, 6).fill('#381800');
pointer.move(11.5, 1);
var inner_knob = draw.circle(17).move(4, 4).fill('#583903').stroke({ color: '#e8b923', width: 0.5 });
console.log(draw.svg());
