const window = require('svgdom');
const SVG = require('svg.js')(window);
const document = window.document;

function getCoordinatesForPercent(percent) {
  const x = Math.cos(2 * Math.PI * percent);
  const y = Math.sin(2 * Math.PI * percent);

  return [x, y];
}

function getCoordinatesForAngle(angle) {
  const x = Math.cos(angle * Math.PI / 180);
  const y = Math.sin(angle * Math.PI / 180);

  return [x, y];
}

function drawRadii(x, y, radius, draw) {
  // 11 major segments
  // 4 between each segment
  // 51 total
  // span of 240 degrees
  let add = 240 / 50;
  let angle = -210;

  for (let i = 0; i < 51; i++) {
    let r;
    if (i % 5 == 0) {
      r = radius;
      // long
    } else {
      r = radius - 1;
      // short
    }

    let line = draw.line();
    let coords = getCoordinatesForAngle(angle);
    line.plot([[x, y], [(coords[0] * r) + x, (coords[1] * r) + y]]);
    line.stroke({
      color: '#e8b923',
      width: 0.75
    }).fill();

    angle += add;
  }

  var circle = draw.circle((radius - 3) * 2).move(x - (radius - 3), y - (radius - 3)).fill('#988903').stroke({
    color: '#583903',
    width: 0.75
  });
}

function drawRadiiCount(x, y, radius, count, draw) {
  let add = 240 / (count - 1);
  let angle = -210;

  for (let i = 0; i < count; i++) {
    let r = radius;
    let line = draw.line();
    let coords = getCoordinatesForAngle(angle);
    line.plot([[x, y], [(coords[0] * r) + x, (coords[1] * r) + y]]);
    line.stroke({
      color: '#e8b923',
      width: 0.75
    }).fill();

    angle += add;
  }

  var circle = draw.circle((radius - 3) * 2).move(x - (radius - 3), y - (radius - 3)).fill('#988903').stroke({
    color: '#583903',
    width: 0.75
  });
}


var draw = SVG(document.documentElement).size('165', '380');
draw.fill('#ffffff');
var rect = draw.rect(165, 380).fill('#ffffff');

// first row
drawRadiiCount(62.5, 70.5, 21, 36, draw);
drawRadiiCount(152.5, 70.5, 21, 7, draw);
drawRadii(242.5, 70.5, 21, draw);
drawRadiiCount(332.5, 70.5, 21, 8, draw);
drawRadiiCount(422.5, 70.5, 21, 37, draw);

// second row
drawRadii(62.5, 157.5, 21, draw);
drawRadii(152.5, 157.5, 21, draw);
drawRadii(242.5, 157.5, 21, draw);
drawRadii(422.5, 157.5, 21, draw);

// third row
drawRadii(62.5, 244.5, 21, draw);
drawRadii(152.5, 244.5, 21, draw);
drawRadii(242.5, 244.5, 21, draw);

// fourth row
drawRadii(62.5, 331.5, 21, draw);
drawRadiiCount(242.5, 331.5, 21, 7, draw);

console.log(draw.svg());
