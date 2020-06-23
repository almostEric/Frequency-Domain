function to_radians (degs) {
  return Math.PI * degs / 180;
}

function is_odd(n) {
  return (n % 2 === 1);
}

function calculate (center_x, center_y, sides, radius, offset = 0) {
  var center_angle = 2 * Math.PI / sides;
  var start_angle;
  if (is_odd(sides)) {
    start_angle = Math.PI / 2 + offset;
  } else {
    start_angle = Math.PI / 2 - center_angle / 2 + offset;
  }

  if (start_angle > 360) {
    start_angle -= 360;
  }

  var vertex = [ ];
  for(var i = 0; i < sides; i++) {
    var ang = start_angle + (i * center_angle);
		var vert_x = center_x + radius * Math.cos(ang);
		var vert_y = center_y - radius * Math.sin(ang);
		vertex.push({x: vert_x , y: vert_y });
	}

  return vertex;
}

function to_svg_string (vertices) {
  var parts = [ ];

  for (var i = 0; i < vertices.length; i++) {
    parts.push(vertices[i].x + ',' + vertices[i].y);
  }

  return parts.join(' ');
}

exports.calculate = calculate;
exports.to_svg_string = to_svg_string;
