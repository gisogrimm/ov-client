// Global variables
var deviceid = ''; // Unique device identifier
var inchannelpos = {}; // Stores positions of audio channels
var objmix_sel = -1; // Selected object for dragging
var objmix_drag = false; // Dragging state flag
var objmix_micangle = 90; // opening angle for additional lines
var recpos = { // Receiver position and rotation
  'x': 0,
  'y': 0,
  'z': 0,
  'rz': 0,
  'ry': 0,
  'rx': 0
};
var strobebuffer = Array( 10 );
var tuner_notelabels = [ 'C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A',
  'Bb',
  'B'
];


/**
 * Converts HSV color space to RGB
 * @param {number} h - Hue value (0-360)
 * @param {number} s - Saturation value (0-1)
 * @param {number} v - Value/Brightness value (0-1)
 * @returns {Object} RGB values with properties r, g, b
 */
function HSVtoRGB( h, s, v ) {
  let r, g, b, i, f, p, q, t;
  // Handle both parameter formats: single object or individual values
  if ( arguments.length === 1 ) {
    s = h.s;
    v = h.v;
    h = h.h;
  }
  i = Math.floor( h * 6 );
  f = h * 6 - i;
  p = v * ( 1 - s );
  q = v * ( 1 - f * s );
  t = v * ( 1 - ( 1 - f ) * s );
  switch ( i % 6 ) {
    case 0:
      r = v, g = t, b = p;
      break;
    case 1:
      r = q, g = v, b = p;
      break;
    case 2:
      r = p, g = v, b = t;
      break;
    case 3:
      r = p, g = q, b = v;
      break;
    case 4:
      r = t, g = p, b = v;
      break;
    case 5:
      r = v, g = p, b = q;
      break;
  }
  return {
    r: Math.round( r * 255 ),
    g: Math.round( g * 255 ),
    b: Math.round( b * 255 )
  };
}

/**
 * Calculates scale factor for mixer display
 * @param {number} w - Width in pixels
 * @param {number} h - Height in pixels
 * @returns {number} Scale factor
 */
function objmix_getscale( w, h ) {
  return 0.11 * Math.min( w, h );
}

/**
 * Applies rotation around Z-axis to position
 * @param {Object} pos - Position object with x, y, z
 * @param {number} a - Rotation angle in radians
 * @returns {Object} Rotated position
 */
function euler_rotz( pos, a ) {
  return {
    x: Math.cos( a ) * pos.x - Math.sin( a ) * pos.y,
    y: Math.cos( a ) * pos.y + Math.sin( a ) * pos.x,
    z: pos.z
  };
}

/**
 * Applies rotation around Y-axis to position
 * @param {Object} pos - Position object with x, y, z
 * @param {number} a - Rotation angle in radians
 * @returns {Object} Rotated position
 */
function euler_roty( pos, a ) {
  return {
    x: Math.cos( a ) * pos.x + Math.sin( a ) * pos.z,
    y: pos.y,
    z: Math.cos( a ) * pos.z - Math.sin( a ) * pos.x
  };
}

/**
 * Applies rotation around X-axis to position
 * @param {Object} pos - Position object with x, y, z
 * @param {number} a - Rotation angle in radians
 * @returns {Object} Rotated position
 */
function euler_rotx( pos, a ) {
  return {
    x: pos.x,
    y: Math.cos( a ) * pos.y - Math.sin( a ) * pos.z,
    z: Math.cos( a ) * pos.z + Math.sin( a ) * pos.y
  };
}

/**
 * Applies sequential rotations (Z → Y → X) to position
 * @param {Object} pos - Position object with x, y, z
 * @param {number} rz - Rotation around Z-axis
 * @param {number} ry - Rotation around Y-axis
 * @param {number} rx - Rotation around X-axis
 * @returns {Object} Rotated position
 */
function euler( pos, rz, ry, rx ) {
  return euler_rotx( euler_roty( euler_rotz( pos, rz ), ry ), rx );
}

/**
 * Applies sequential local rotations (X → Y → Z) to position
 * @param {Object} pos - Position object with x, y, z
 * @param {number} rz - Rotation around Z-axis
 * @param {number} ry - Rotation around Y-axis
 * @param {number} rx - Rotation around X-axis
 * @returns {Object} Rotated position
 */
function euler_loc( pos, rz, ry, rx ) {
  return euler_rotz( euler_roty( euler_rotx( pos, rx ), ry ), rz );
}

/**
 * Converts 3D position to screen coordinates
 * @param {Object|Array} pos - Position object or array with x, y, z
 * @returns {Object} Screen coordinates with x, y
 */
function pos2scr( pos ) {
  const canvas = document.getElementById( "objmixer" );
  const scale = objmix_getscale( canvas.width, canvas.height );
  if ( Array.isArray( pos ) ) {
    return {
      x: 0.5 * canvas.width - scale * pos[ 1 ],
      y: 0.5 * canvas.height - scale * pos[ 0 ]
    };
  }
  return {
    x: 0.5 * canvas.width - scale * pos.y,
    y: 0.5 * canvas.height - scale * pos.x
  };
}

/**
 * Converts screen coordinates to 3D position
 * @param {Object} pos - Screen coordinates with x, y
 * @returns {Object} 3D position with x, y, z
 */
function scr2pos( pos ) {
  const canvas = document.getElementById( "objmixer" );
  const scale = objmix_getscale( canvas.width, canvas.height );
  return {
    y: -( pos.x - 0.5 * canvas.width ) / scale,
    x: ( -pos.y + 0.5 * canvas.height ) / scale
  };
}

/**
 * Handles canvas click events
 * @param {MouseEvent} e - Mouse event object
 */
function on_canvas_click( e ) {
  const canvas = document.getElementById( "objmixer" );
  var rect = canvas.getBoundingClientRect();
  var ksel = null;
  // Check all channel positions
  for ( const [ key, vertex ] of Object.entries( inchannelpos ) ) {
    const pos = pos2scr( [ vertex.x, vertex.y, vertex.z ] );
    const clickPos = {
      x: pos.x - e.clientX + rect.left,
      y: pos.y - e.clientY + rect.top
    };
    // Calculate distance from click to position
    const distance = Math.sqrt( clickPos.x * clickPos.x + clickPos.y * clickPos
      .y );
    if ( distance < 24 ) {
      ksel = key;
    }
  }
  objmix_sel = ksel;
  if ( ksel ) {
    objmix_drag = true;
  }
}

/**
 * Handles canvas mouse up events
 * @param {MouseEvent} e - Mouse event object
 */
function on_canvas_up( e ) {
  if ( objmix_drag ) {
    objmix_drag = false;
    socket.emit( 'objmixposcomplete' );
    e.preventDefault();
  }
}

/**
 * Handles canvas mouse move events
 * @param {MouseEvent} e - Mouse event object
 */
function on_canvas_move( e ) {
  if ( objmix_drag ) {
    const canvas = document.getElementById( "objmixer" );
    if ( !canvas ) return;
    const rect = canvas.getBoundingClientRect();
    const np = scr2pos( {
      x: e.clientX - rect.left,
      y: e.clientY - rect.top
    } );
    // Update selected channel position
    inchannelpos[ objmix_sel ].x = np.x;
    inchannelpos[ objmix_sel ].y = np.y;
    objmix_draw();
    // Emit position update
    const obj = {
      path: inchannelpos[ objmix_sel ].path,
      value: [
        inchannelpos[ objmix_sel ].x,
        inchannelpos[ objmix_sel ].y,
        inchannelpos[ objmix_sel ].z
      ]
    };
    socket.emit( "msg", obj );
    e.preventDefault();
  }
}

function transform( p, pos, rz, ry, rx ) {
  var ps = euler( p, rz, ry, rx );
  ps.x += pos.x;
  ps.y += pos.y;
  ps.z += pos.z;
  return ps;
}

/**
 * Draws receiver object on canvas
 * @param {CanvasRenderingContext2D} ctx - Canvas context
 * @param {Object} pos - Position object with x, y, z
 * @param {number} rz - Rotation around Z-axis
 * @param {number} ry - Rotation around Y-axis
 * @param {number} rx - Rotation around X-axis
 */
function draw_receiver( ctx, pos, rz, ry, rx ) {
  const scale = 0.1;
  const msize = 2;
  // Define receiver points
  const points = [ {
    x: 1.8 * msize * scale,
    y: -0.6 * msize * scale,
    z: 0
  }, {
    x: 2.9 * msize * scale,
    y: 0,
    z: 0
  }, {
    x: 1.8 * msize * scale,
    y: 0.6 * msize * scale,
    z: 0
  }, {
    x: -0.5 * msize * scale,
    y: 2.3 * msize * scale,
    z: 0
  }, {
    x: 0,
    y: 1.7 * msize * scale,
    z: 0
  }, {
    x: 0.5 * msize * scale,
    y: 2.3 * msize * scale,
    z: 0
  }, {
    x: -0.5 * msize * scale,
    y: -2.3 * msize * scale,
    z: 0
  }, {
    x: 0,
    y: -1.7 * msize * scale,
    z: 0
  }, {
    x: 0.5 * msize * scale,
    y: -2.3 * msize * scale,
    z: 0
  } ];
  const transPoints = points.map( p => transform( p, pos, rz, ry, rx ) );
  // Apply rotations and translate points
  //points.forEach( p => {
  //  p = euler( p, rz, ry, rx );
  //  p.x += pos.x;
  //  p.y += pos.y;
  //  p.z += pos.z;
  //} );

  const center = pos2scr( pos );
  //// Convert points to screen coordinates
  const screenPoints = transPoints.map( p => pos2scr( p ) );
  //} );
  // Calculate size scale
  const sizeRef = pos2scr( [ 0, 0 ] );
  const sizeScale = Math.sqrt( Math.pow( sizeRef.x - pos2scr( [ scale * msize,
    scale * msize
  ] ).x, 2 ) + Math.pow( sizeRef.y - pos2scr( [ scale * msize, scale *
    msize
  ] ).y, 2 ) );
  // Draw receiver
  ctx.save();
  ctx.lineWidth = 2 * msize;
  ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
  // Draw circle around center
  ctx.beginPath();
  ctx.arc( center.x, center.y, 1.4 * sizeScale, 0, 2 * Math.PI );
  ctx.stroke();
  // Draw receiver shape
  ctx.beginPath();
  ctx.moveTo( screenPoints[ 0 ].x, screenPoints[ 0 ].y );
  ctx.lineTo( screenPoints[ 1 ].x, screenPoints[ 1 ].y );
  ctx.lineTo( screenPoints[ 2 ].x, screenPoints[ 2 ].y );
  ctx.moveTo( screenPoints[ 3 ].x, screenPoints[ 3 ].y );
  ctx.lineTo( screenPoints[ 4 ].x, screenPoints[ 4 ].y );
  ctx.lineTo( screenPoints[ 5 ].x, screenPoints[ 5 ].y );
  ctx.moveTo( screenPoints[ 6 ].x, screenPoints[ 6 ].y );
  ctx.lineTo( screenPoints[ 7 ].x, screenPoints[ 7 ].y );
  ctx.lineTo( screenPoints[ 8 ].x, screenPoints[ 8 ].y );
  ctx.stroke();
  ctx.restore();
}

/**
 * Draws the complete mixer interface
 */
function objmix_draw() {
  const canvas = document.getElementById( "objmixer" );
  if ( !canvas ) return;
  // Set canvas dimensions
  canvas.width = canvas.parentElement.clientWidth - 2;
  canvas.height = canvas.width;
  const ctx = canvas.getContext( "2d" );
  ctx.fillStyle = '#153d17';
  ctx.fillRect( 0, 0, canvas.width, canvas.height );
  // Draw background elements
  ctx.save();
  ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
  // Draw crosshair
  const center = pos2scr( [ 0, 0, 0 ] );
  const left = pos2scr( [ 0, 4, 0 ] );
  const right = pos2scr( [ 0, -4, 0 ] );
  const front = pos2scr( [ 4, 0, 0 ] );
  const back = pos2scr( [ -4, 0, 0 ] );
  ctx.beginPath();
  ctx.moveTo( left.x, left.y );
  ctx.lineTo( right.x, right.y );
  ctx.moveTo( back.x, back.y );
  ctx.lineTo( front.x, front.y );
  // now draw opening angle lines:
  const micleft = pos2scr( euler_rotz( {
    x: 4,
    y: 0,
    z: 0
  }, 0.0087266 * objmix_micangle ) );
  const micright = pos2scr( euler_rotz( {
    x: 4,
    y: 0,
    z: 0
  }, -0.0087266 * objmix_micangle ) );
  ctx.moveTo( micleft.x, micleft.y );
  ctx.lineTo( center.x, center.y );
  ctx.lineTo( micright.x, micright.y );
  ctx.stroke();
  // Draw circle
  const radius = Math.abs( left.x - center.x );
  ctx.beginPath();
  ctx.arc( center.x, center.y, radius, 0, 2 * Math.PI );
  ctx.stroke();
  // Draw smaller circle
  ctx.beginPath();
  ctx.arc( center.x, center.y, 0.25 * radius, 0, 2 * Math.PI );
  ctx.stroke();
  ctx.restore();
  // Draw channels
  ctx.save();
  ctx.font = "24px sans";
  Object.entries( inchannelpos ).forEach( ( [ key, vertex ], index ) => {
    var gain = 0;
    if ( "gain" in vertex )
      gain = vertex.gain;
    // Calculate position on screen
    const pos = pos2scr( vertex );
    // Calculate color based on position
    const color = HSVtoRGB( index / Object.entries( inchannelpos ).length,
      0.65, 0.8 );
    if ( gain > -100 ) {
      // Draw connection line
      ctx.beginPath();
      ctx.strokeStyle = `rgb(${color.r},${color.g},${color.b})`;
      ctx.lineWidth = 3;
      const rotatedPos = euler_loc( {
        x: 0.5,
        y: 0,
        z: 0
      }, vertex.rz, vertex.ry, vertex.rx );
      rotatedPos.x += vertex.x;
      rotatedPos.y += vertex.y;
      rotatedPos.z += vertex.z;
      const rotatedScreen = pos2scr( rotatedPos );
      ctx.moveTo( pos.x, pos.y );
      ctx.lineTo( rotatedScreen.x, rotatedScreen.y );
      ctx.stroke();
      // Draw channel marker
      ctx.beginPath();
      ctx.arc( pos.x, pos.y, 20 * Math.sqrt( canvas.width / 1000 ), 0, Math
        .PI * 2 );
      ctx.fillStyle = `rgb(${color.r},${color.g},${color.b})`;
      ctx.fill();
      // Draw channel label
      //ctx.fillStyle = `rgb(${color.r},${color.g},${color.b})`;
      ctx.fillText( vertex.name, pos.x + 24, pos.y - 5 );
    }
  } );
  // Draw receiver
  ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
  draw_receiver( ctx, recpos, recpos.rz, recpos.ry, recpos.rx );
  ctx.restore();
}

/**
 * Initializes and updates the mixer display
 */
function update_objmix_sounds() {
  const canvas = document.getElementById( "objmixer" );
  if ( !canvas ) return;
  // Set canvas dimensions
  canvas.width = canvas.parentElement.clientWidth;
  canvas.height = 0.55 * canvas.width;
  // Add event listeners
  canvas.addEventListener( 'pointerdown', on_canvas_click );
  canvas.addEventListener( 'pointerup', on_canvas_up );
  canvas.addEventListener( 'pointermove', on_canvas_move );
  // Redraw mixer
  objmix_draw();
}

function update_objmix_sounds() {
  const canvas = document.getElementById( "objmixer" );
  if ( !canvas ) return;
  canvas.width = canvas.parentElement.clientWidth;
  canvas.height = 0.55 * canvas.width;
  canvas.addEventListener( 'pointerdown', on_canvas_click );
  canvas.addEventListener( 'pointerup', on_canvas_up );
  canvas.addEventListener( 'pointermove', on_canvas_move );
  objmix_draw();
}

function tuner_draw() {
  const canvas = document.getElementById( "tunercanvas" );
  if ( !canvas ) return;
  // Set canvas dimensions
  canvas.width = canvas.parentElement.clientWidth - 2;
  canvas.height = 0.1 * canvas.width;
  const ctx = canvas.getContext( "2d" );
  var gradient = ctx.createLinearGradient( 0, 0, canvas.width, 0 );
  var prev_color = 'rgb(0,0,0)';
  gradient.addColorStop( 0, prev_color );
  r_back = 20;
  g_back = 44;
  b_back = 27;
  r_front = 241;
  g_front = 236;
  b_front = 208;
  for ( let x = 0; x < strobebuffer.length; x++ ) {
    let w = 0.5 * strobebuffer[ strobebuffer.length - x - 1 ] + 0.5;
    if ( !isNaN( w ) ) {
      let r = Math.min( 255, Math.max( 0, w * r_front + ( 1 - w ) * r_back ) );
      let g = Math.min( 255, Math.max( 0, w * g_front + ( 1 - w ) * g_back ) );
      let b = Math.min( 255, Math.max( 0, w * b_front + ( 1 - w ) * b_back ) );
      var next_color = `rgb(${r} ${g} ${b})`;
      gradient.addColorStop( x / strobebuffer.length, next_color );
    }
    prev_color = next_color;
  }
  ctx.fillStyle = gradient;
  ctx.fillRect( 0, 0, canvas.width, canvas.height );
  //ctx.save();
  // Draw background elements
  //ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
  // Draw delta scale
  //ctx.restore();
}

function tuner_set_isactive() {
  tuner_active = document.getElementById( 'tuner_active' );
  if ( tuner_active.checked ) socket.emit( "msg", {
    "path": '/tuner/isactive',
    "value": 1
  } )
  else socket.emit( "msg", {
    "path": '/tuner/isactive',
    "value": 0
  } );
}

function upload_session_gains() {
  socket.emit( "msg", {
    "path": "/uploadsessiongains",
    "value": null
  } );
}

function str_pad_left( string, pad, length ) {
  return ( new Array( length + 1 ).join( pad ) + string ).slice( -length );
}

function sec2minsec( t ) {
  var minutes = Math.floor( t / 60 );
  var seconds = Math.floor( t - 60 * minutes );
  var sec10 = Math.floor( 10 * ( t - Math.floor( t ) ) );
  return str_pad_left( minutes, '0', 2 ) + ':' + str_pad_left( seconds, '0',
    2 ) + '.' + sec10;
}

function recerror( e ) {
  let el = document.getElementById( "recerr" );
  if ( el.childNodes.length > 0 ) el.replaceChild( document.createTextNode( e ),
    el.childNodes[ 0 ] );
  else el.appendChild( document.createTextNode( e ) );
}

function objmix_upload_posandgains() {
  socket.emit( "msg", {
    "path": "/uploadobjmix",
    "value": null
  } );
}
socket.on( "jackrecerr", function( e ) {
  recerror( e )
} );
socket.on( "jackrectime", function( t ) {
  let el = document.getElementById( "rectime" );
  el.setAttribute( "value", sec2minsec( t ) );
  //el.set_attribute("value",t);
} );
socket.on( "jackrecportlist", function( t ) {
  let el = document.getElementById( "portlist" );
  if ( el )
    while ( el.firstChild ) {
      el.removeChild( el.firstChild );
    }
} );
socket.on( 'jackrecaddport', function( p ) {
  var labs = p;
  var classes = 'mixerstrip jackrecsrcport';
  var helps = '';
  if ( p.startsWith( 'n2j_' ) ) return;
  if ( p.startsWith( deviceid + '.levelanalysis:out' ) ) return;
  if ( p.startsWith( 'system:capture' ) ) {
    classes += ' mixerego';
    helps = 'Hardware input';
  }
  if ( p.startsWith( 'bus.' ) ) {
    classes += ' mixerego';
    helps = 'My input (with effects)';
    labs = labs.replace( 'bus.', '' ).replace( ':out.0', '' );
  }
  if ( p == deviceid + '.metronome:out.0' ) {
    return;
  }
  if ( p == deviceid + '.metronome:out.1' ) {
    classes += ' mixerother';
    helps = 'Metronome';
    labs = 'metronome';
  }
  if ( p.startsWith( deviceid + '.main:' ) ) {
    classes += ' mixerother';
    helps = 'My headphone output';
    labs = labs.replace( deviceid + '.main:', '' );
  }
  labs = labs.replace( '.' + deviceid + ':out', '' );
  let el = document.getElementById( "portlist" );
  if ( el ) {
    let div = el.appendChild( document.createElement( 'div' ) );
    div.setAttribute( 'class', classes );
    let inp = div.appendChild( document.createElement( 'input' ) );
    inp.setAttribute( 'title', helps );
    inp.setAttribute( 'type', 'checkbox' );
    inp.setAttribute( 'value', p );
    inp.setAttribute( 'id', p );
    inp.setAttribute( 'class', 'jackport checkbox' );
    let lab = div.appendChild( document.createElement( 'label' ) );
    lab.setAttribute( 'for', p );
    lab.setAttribute( 'title', helps );
    lab.appendChild( document.createTextNode( labs ) );
  }
} );
socket.on( "jackrecfilelist", function( t ) {
  let el = document.getElementById( "filelist" );
  if ( el )
    while ( el.firstChild ) {
      el.removeChild( el.firstChild );
    }
} );
socket.on( 'jackrecaddfile', function( p ) {
  let el = document.getElementById( "filelist" );
  if ( !el ) return;
  let div = el.appendChild( document.createElement( 'div' ) );
  let inp = div.appendChild( document.createElement( 'input' ) );
  inp.setAttribute( 'type', 'checkbox' );
  inp.setAttribute( 'value', p );
  inp.setAttribute( 'class', 'filename' );
  let lab = div.appendChild( document.createElement( 'a' ) );
  lab.setAttribute( 'href', p );
  lab.appendChild( document.createTextNode( p ) );
} );
socket.on( 'jackrecstart', function( p ) {
  let el = document.getElementById( "recindicator" );
  el.style = 'display: inline-block;';
} );
socket.on( 'jackrecstop', function( p ) {
  let el = document.getElementById( "recindicator" );
  el.style = 'display: none;';
} );
socket.on( 'objmixredraw', function( p ) {
  objmix_draw();
} );
let form = document.getElementById( "mixer" );
if ( form ) form.oninput = handleChange;

function gain_to_gui( dbGain ) {
  guiGain = Math.pow( ( ( dbGain + 200.0 ) / 210.0 ), 6.0 );
  return guiGain;
}

function gui_to_gain( guiGain ) {
  dbGain = 210.0 * ( Math.pow( guiGain, 1 / 6 ) ) - 200.0;
  return dbGain;
}

/**
 * Upload gain values from mixer
 */
function handleChange( e ) {
  if ( e.target.id.substr( 0, 3 ) == "txt" ) {
    // text field contains dB values, no conversion:
    socket.emit( "msg", {
      path: e.target.id.substr( 3 ),
      value: e.target.valueAsNumber
    } );
    let fad = document.getElementById( e.target.id.substr( 3 ) );
    if ( fad != null ) {
      fad.value = gain_to_gui( e.target.valueAsNumber );
    }
  } else {
    if ( e.target.type == "checkbox" ) {
      if ( e.target.checked ) socket.emit( "msg", {
        path: e.target.id,
        value: 1
      } );
      else socket.emit( "msg", {
        path: e.target.id,
        value: 0
      } );
    } else {
      // range fader, conversion needed:
      const new_gain = gui_to_gain( e.target.valueAsNumber );
      socket.emit( "msg", {
        path: e.target.id,
        value: new_gain
      } );
      //let fadt = document.getElementById( e.target.id );
      //  if ( fadt != null ) {
      //  //fadt.value = new_gain.toFixed( 1 );
      //}
    }
  }
}

function jackrec_start() {
  socket.emit( "msg", {
    path: '/jackrec/clear',
    value: null
  } );
  let el = document.getElementById( "portlist" );
  if ( !el ) return;
  let ports = el.getElementsByClassName( 'jackport' );
  for ( var k = 0; k < ports.length; k++ ) {
    if ( ports[ k ].checked ) {
      socket.emit( "msg", {
        path: '/jackrec/addport',
        value: ports[ k ].getAttribute( 'value' )
      } );
    }
  }
  recerror( '' );
  socket.emit( "msg", {
    path: '/jackrec/start',
    value: null
  } );
}

function jackrec_delete() {
  let el = document.getElementById( "filelist" );
  if ( !el ) return;
  let ports = el.getElementsByClassName( 'filename' );
  for ( var k = 0; k < ports.length; k++ ) {
    if ( ports[ k ].checked ) {
      socket.emit( "msg", {
        path: '/jackrec/rmfile',
        value: ports[ k ].getAttribute( 'value' )
      } );
    }
  }
  socket.emit( 'msg', {
    path: '/jackrec/listfiles',
    value: null
  } );
  document.getElementById( "selectallfiles" ).checked = false;
}

function jackrec_stop() {
  recerror( '' );
  socket.emit( "msg", {
    path: '/jackrec/stop',
    value: null
  } );
  socket.emit( 'msg', {
    path: '/jackrec/listfiles',
    value: null
  } );
}

function jackrec_selectallfiles() {
  let ischecked = document.getElementById( "selectallfiles" ).checked;
  let el = document.getElementById( "filelist" );
  if ( !el ) return;
  let ports = el.getElementsByClassName( 'filename' );
  for ( var k = 0; k < ports.length; k++ ) {
    ports[ k ].checked = ischecked;
  }
}

function jackrec_selectallports() {
  let ischecked = document.getElementById( "selectallports" ).checked;
  let el = document.getElementById( "portlist" );
  if ( !el ) return;
  let ports = el.getElementsByClassName( 'jackport' );
  for ( var k = 0; k < ports.length; k++ ) {
    ports[ k ].checked = ischecked;
  }
}

function showtab( name ) {
  names = [ 'mixer', 'plugpars', 'jackrec', 'objmix', 'tuner' ];
  for ( var k = 0; k < names.length; k++ ) {
    var el = document.getElementById( names[ k ] );
    var elinp = document.getElementById( 'tabact' + names[ k ] );
    if ( names[ k ] == name ) {
      el.classList.add( "tabshow" );
      el.classList.remove( "tabhide" );
      elinp.classList.add( "tabact" );
    } else {
      el.classList.remove( "tabshow" );
      el.classList.add( "tabhide" );
      elinp.classList.remove( "tabact" );
    }
  }
  objmix_draw();
  tuner_draw();
}


/**
 * register web socket handlers:
 */
socket.on( "connect", function() {
  inchannelpos = {};
  objmix_sel = null;
  objmix_drag = false;
  recpos = {
    'x': 0,
    'y': 0,
    'z': 0,
    'rz': 0,
    'ry': 0,
    'rx': 0
  };
  socket.emit( "config", {} );
} );
socket.on( 'deviceid', function( id ) {
  deviceid = id;
} );
socket.on( "scene", function( scene ) {
  let el = document.getElementById( "mixer" );
  if ( el ) {
    while ( el.firstChild ) {
      el.removeChild( el.firstChild );
    }
    let elheader = document.createElement( "h2" );
    elheader.setAttribute( "class", "scene" );
    el.append( scene, elheader );
    let elgainstore = document.createElement( "p" );
    elgainstore.setAttribute( "class", "gainstore" );
    el.appendChild( elgainstore );
  }
} );
socket.on( "vertexgain", function( vertexid, gain ) {
  if ( vertexid in inchannelpos ) {
    if ( 'gain' in inchannelpos[ vertexid ] ) {
      if ( gain != inchannelpos[ vertexid ].gain ) {
        inchannelpos[ vertexid ].gain = gain;
        objmix_draw();
      }
    } else {
      inchannelpos[ vertexid ].gain = gain;
    }
  }
} );
socket.on( "vertexpos", function( vertexid, name, x, y, z, path ) {
  if ( Reflect.has( inchannelpos, vertexid ) ) {
    inchannelpos[ vertexid ].name = name;
    inchannelpos[ vertexid ].x = x;
    inchannelpos[ vertexid ].y = y;
    inchannelpos[ vertexid ].z = z;
    inchannelpos[ vertexid ].path = path;
  } else {
    inchannelpos[ vertexid ] = {
      'name': name,
      'x': x,
      'y': y,
      'z': z,
      'rz': 0,
      'ry': 0,
      'rx': 0,
      'path': path,
      'gain': 0
    };
  }
  update_objmix_sounds();
} );
socket.on( "vertexposrot", function( vertexid, name, x, y, z, rz, ry, rx,
  path ) {
  if ( Reflect.has( inchannelpos, vertexid ) ) {
    var need_update = false;
    if ( ( x != inchannelpos[ vertexid ].x ) || ( y != inchannelpos[
        vertexid ].y ) || ( z != inchannelpos[ vertexid ].z ) || ( rx !=
        inchannelpos[ vertexid ].rx ) || ( ry != inchannelpos[ vertexid ]
        .ry ) || ( rz != inchannelpos[ vertexid ].rz ) ) need_update = true;
    inchannelpos[ vertexid ].x = x;
    inchannelpos[ vertexid ].y = y;
    inchannelpos[ vertexid ].z = z;
    inchannelpos[ vertexid ].rx = rx;
    inchannelpos[ vertexid ].ry = ry;
    inchannelpos[ vertexid ].rz = rz;
    if ( need_update ) objmix_draw();
  } else {
    if ( name === 'main' ) {
      var need_update = false;
      if ( ( x != recpos.x ) || ( y != recpos.y ) || ( z != recpos.z ) || (
          rx != recpos.rx ) || ( ry != recpos.ry ) || ( rz != recpos.rz ) )
        need_update = true;
      recpos.x = x;
      recpos.y = y;
      recpos.z = z;
      recpos.rx = rx;
      recpos.ry = ry;
      recpos.rz = rz;
      if ( need_update ) objmix_draw();
    }
  }
  //inchannelpos[vertexid] = {'name':name,'x':x, 'y':y, 'z': z, 'path' : path};
  //update_objmix_sounds();
} );
socket.on( "newfader", function( faderno, val ) {
  // this function adds a new mixer strip to the mixer panel.

  // remove effect bus from mixer:
  if ( val.startsWith( 'bus.' ) )
    return;
  if ( val == deviceid + '.levelanalysis' )
    return;
  fader = "/touchosc/fader" + faderno;
  levelid = "/touchosc/level" + faderno;
  muteid = "/touchosc/mute" + faderno;
  recordtoggleid = "/recordtoggle" + faderno;
  let el_mixer = document.getElementById( "mixer" );
  if ( el_mixer ) {
    orig_val = val;
    // set class based on name:
    let classname = "mixerstrip";
    val = val.replace( '.' + deviceid, '' );
    val = val.replace( deviceid + '.', '' );
    val = val.replace( 'bus.', '' );
    if ( val.startsWith( "ego." ) || ( val == "monitor" ) ) {
      classname = classname + " mixerego";
      val = val.replace( 'ego.', '' );
    }
    if ( ( val == "main" ) || ( val == "reverb" ) ) classname = classname +
      " mixerother";
    // create main mixer strip element:
    let el_mixerstrip = el_mixer.appendChild( document.createElement(
      "div" ) );
    el_mixerstrip.setAttribute( "class", classname );
    el_mixerstrip.setAttribute( "id", "mixerstrip_" + orig_val );
    // now insert two columns, one for mute/rec buttons, the other for fader/level:
    let el_col1 = el_mixerstrip.appendChild( document.createElement(
      "div" ) );
    el_col1.setAttribute( "class", "mixercolbuttons" );
    let el_col2 = el_mixerstrip.appendChild( document.createElement(
      "div" ) );
    el_col2.setAttribute( "class", "mixercolfader" );
    let el_row1 = el_col2.appendChild( document.createElement(
      "div" ) );
    el_row1.setAttribute( "class", "mixerrow" );
    let el_row2 = el_col2.appendChild( document.createElement(
      "div" ) );
    el_row2.setAttribute( "class", "mixerrow" );
    let el_row3 = el_col2.appendChild( document.createElement(
      "div" ) );
    el_row3.setAttribute( "class", "mixerrow" );
    let datalist = document.createElement( "tickbox" );
    datalist.setAttribute( "class", "fadertickbox" );
    [ -40, -30, -20, -10, 0, 5, 10 ].forEach( function( item, index ) {
      let opt = datalist.appendChild( document.createElement(
        "div" ) );
      opt.setAttribute( "style", "left: calc(" + 100 * gain_to_gui(
          item ) +
        "% - 15px);" );
      opt.setAttribute( "class", "fadertick" );
      opt.appendChild( document.createTextNode( item ) );
      opt.appendChild( document.createElement( 'br' ) );
      opt.appendChild( document.createTextNode( '|' ) );
    } );
    el_row1.appendChild( datalist );
    let el_lab = el_row1.appendChild( document.createElement( "label" ) );
    el_lab.setAttribute( "for", fader );
    el_lab.setAttribute( "class", "mixerlabel" );
    el_lab.append( val );
    // mute button:
    let el_mutebuttondiv = el_col1.appendChild( document.createElement(
      "label" ) );
    el_mutebuttondiv.setAttribute( "class", "togglebutton" );
    let el_mutebutton = el_mutebuttondiv.appendChild( document
      .createElement( "input" ) );
    el_mutebutton.setAttribute( "type", "checkbox" );
    el_mutebutton.setAttribute( "id", muteid );
    let mb = el_mutebuttondiv.appendChild( document
      .createElement( "span" ) );
    mb.setAttribute( "class", "mutebutton" );
    mb.appendChild( document.createTextNode( 'M' ) );
    // // record enable:
    // let el_recordtogglediv = el_col1.appendChild( document.createElement(
    //     "label" ) );
    // el_recordtogglediv.setAttribute( "class", "togglebutton" );
    // let el_recordtoggle = el_recordtogglediv.appendChild( document
    //                                                       .createElement( "input" ) );
    // el_recordtoggle.setAttribute( "type", "checkbox" );
    // el_recordtoggle.setAttribute( "id", recordtoggleid );
    // let rb = el_recordtogglediv.appendChild( document
    //                                          .createElement( "span" ) );
    // rb.setAttribute( "class", "recordtoggle" );
    // rb.appendChild(document.createTextNode('R'));
    // fader:
    let el_fader = el_row2.appendChild( document.createElement( "input" ) );
    el_fader.setAttribute( "class", "fader" );
    el_fader.setAttribute( "type", "range" );
    el_fader.setAttribute( "min", "0" );
    el_fader.setAttribute( "max", "1" );
    el_fader.setAttribute( "value", val );
    el_fader.setAttribute( "step", 1 / 127 );
    el_fader.setAttribute( "id", fader );
    //el_fader.setAttribute("list","gainfadervals");
    el_fader.onchange = upload_session_gains;
    let el_gaintext = el_row2.appendChild( document.createElement(
      "input" ) );
    el_gaintext.setAttribute( "type", "number" );
    el_gaintext.setAttribute( "class", "gaintxtfader" );
    el_gaintext.setAttribute( "min", "-200" );
    el_gaintext.setAttribute( "max", "10" );
    el_gaintext.setAttribute( "step", "0.1" );
    el_gaintext.setAttribute( "id", "txt" + fader );
    let el_meter = el_row3.appendChild( document.createElement( "meter" ) );
    el_meter.setAttribute( "class", "level" );
    el_meter.setAttribute( "min", "0" );
    el_meter.setAttribute( "max", "94" );
    el_meter.setAttribute( "low", "71" );
    el_meter.setAttribute( "high", "84" );
    el_meter.setAttribute( "optimum", "54" );
    el_meter.setAttribute( "id", levelid );
    let el_metertext = el_row3.appendChild( document.createElement(
      "input" ) );
    el_metertext.setAttribute( "type", "text" );
    el_metertext.setAttribute( "readonly", "true" );
    el_metertext.setAttribute( "class", "gaintxtfader" );
    el_metertext.setAttribute( "id", "txt" + levelid );
  }
} );
socket.on( "updatefader", function( fader, val ) {
  let fad = document.getElementById( fader );
  if ( ( fad != null ) && ( val != null ) )
    if ( fader.startsWith( '/touchosc/level' ) ) {
      fad.value = val;
    } else {
      fad.value = gain_to_gui( val );
    }
  let fadt = document.getElementById( "txt" + fader );
  if ( ( fadt != null ) && ( val != null ) ) {
    fadt.value = val.toFixed( 1 );
  }
} );
socket.on( "updatemute", function( fader, val ) {
  let fad = document.getElementById( fader );
  if ( ( fad != null ) && ( val != null ) ) {
    fad.checked = ( val == 1 );
  }
} );
socket.on( 'updatevar', function( id, val, vartype ) {
  if ( ( val != null ) && ( vartype == 'float' ) ) {
    let el = document.getElementById( id );
    if ( el ) el.setAttribute( 'value', val );
    let el2 = document.getElementById( id + '.disp' );
    if ( el2 ) el2.setAttribute( 'value', parseFloat( val.toPrecision(
      4 ) ) );
  }
  if ( ( val != null ) && ( vartype == 'bool' ) ) {
    let el = document.getElementById( id );
    if ( el ) el.checked = ( val != 0 );
    //let el2 = document.getElementById( id + '.disp' );
    //if ( el2 ) el2.setAttribute( 'value', parseFloat( val.toPrecision(
    //    4 ) ) );
  }
} );
socket.on( 'oscvarlist', function( parents, varlist ) {
  let el = document.getElementById( 'plugpars' );
  while ( el.firstChild ) {
    el.removeChild( el.firstChild );
  }
  for ( var k = 0; k < parents.length; ++k ) {
    const p = parents[ k ];
    var d;
    var dp = null;
    if ( p.parent && ( p.parent.length > 0 ) ) dp = document.getElementById(
      p.parent );
    if ( dp ) d = dp.appendChild( document.createElement( 'div' ) );
    else d = el.appendChild( document.createElement( 'div' ) );
    d.setAttribute( 'id', p.id );
    d.setAttribute( 'class', 'parblock' );
    d.appendChild( document.createTextNode( p.label ) );
  }
  for ( const key in varlist ) {
    const v = varlist[ key ];
    let p = document.getElementById( v.parent );
    if ( p ) {
      var dplug = p.appendChild( document.createElement( 'div' ) );
      dplug.setAttribute( 'class', 'parstrip' );
      var dpluglab = dplug.appendChild( document.createElement( 'label' ) );
      //dpluglab.setAttribute('type','label');
      dpluglab.setAttribute( 'class', 'parstriplabel' );
      dpluglab.appendChild( document.createTextNode( v.label ) );
      //dpluglab.setAttribute('value',v.label);
      dpluglab.setAttribute( 'title', v.comment );
      var inp = dplug.appendChild( document.createElement( 'input' ) );
      var inp2 = dplug.appendChild( document.createElement( 'input' ) );
      inp.setAttribute( 'class', 'parstripctl' );
      inp.setAttribute( 'id', v.id );
      inp.setAttribute( 'title', v.comment );
      inp2.setAttribute( 'class', 'parstripdisp' );
      inp2.setAttribute( 'id', v.id + '.disp' );
      if ( v.type == 'float' ) {
        inp2.setAttribute( 'type', 'number' );
        inp2.setAttribute( 'step', 'any' );
      }
      if ( v.type == 'bool' ) {
        inp2.setAttribute( 'style', 'display:none;' );
      }
      inp2.setAttribute( 'title', v.comment );
      if ( v.type == 'float' ) {
        var rg = v.range.split( ',' );
        if ( rg.length == 2 ) {
          var vmin = parseFloat( rg[ 0 ].replace( '[', '' ).replace( ']',
            '' ) );
          var vmax = parseFloat( rg[ 1 ].replace( '[', '' ).replace( ']',
            '' ) );
          const step = ( vmax - vmin ) / 256;
          if ( rg[ 0 ].startsWith( ']' ) ) vmin += step;
          if ( rg[ 1 ].endsWith( '[' ) ) vmax -= step;
          inp.setAttribute( 'type', 'range' );
          inp.setAttribute( 'min', vmin );
          inp.setAttribute( 'max', vmax );
          inp.setAttribute( 'step', step );
        } else {
          inp.setAttribute( 'type', 'number' );
          inp.setAttribute( 'step', 'any' );
        }
      }
      if ( v.type == 'bool' ) {
        inp.setAttribute( 'type', 'checkbox' );
      }
      inp.onchange = function( e ) {
        if ( v.type == 'float' ) {
          socket.emit( "msg", {
            "path": v.path,
            "value": e.target.valueAsNumber
          } );
        } else if ( v.type == 'bool' ) {
          if ( e.target.checked ) socket.emit( "msg", {
            "path": v.path,
            "value": 1
          } );
          else socket.emit( "msg", {
            "path": v.path,
            "value": 0
          } );
        }
        socket.emit( "msg", {
          "path": "/uploadpluginsettings",
          "value": null
        } );
        let inp = document.getElementById( v.id + '.disp' );
        if ( inp && ( v.type == 'float' ) ) inp.value = parseFloat( e
          .target.valueAsNumber.toPrecision( 4 ) );
      };
      inp2.onchange = function( e ) {
        socket.emit( "msg", {
          "path": v.path,
          "value": e.target.valueAsNumber
        } );
        socket.emit( "msg", {
          "path": "/uploadpluginsettings",
          "value": null
        } );
        let inp = document.getElementById( v.id );
        if ( inp ) inp.value = e.target.valueAsNumber;
      };
    }
  }
} );
socket.on( 'tuner', function( v_freq, v_note, v_octave, v_delta,
  v_confidence ) {
  tuner_draw();
  tuner_delta_neg = document.getElementById( 'tuner_delta_neg' );
  tuner_delta_pos = document.getElementById( 'tuner_delta_pos' );
  tuner_note = document.getElementById( 'tuner_notedisplay' );
  tuner_freq = document.getElementById( 'tuner_freqdisplay' );
  tuner_delta = document.getElementById( 'tuner_deltadisplay' );
  tuner_delta_neg.value = -v_delta;
  tuner_delta_pos.value = v_delta;
  tuner_delta_neg.style.opacity = v_confidence;
  tuner_delta_pos.style.opacity = v_confidence;
  tuner_note.style.opacity = Math.sqrt( v_confidence );
  while ( tuner_note.firstChild )
    tuner_note.removeChild( tuner_note.firstChild );
  tuner_freq.style.opacity = Math.sqrt( v_confidence );
  while ( tuner_freq.firstChild )
    tuner_freq.removeChild( tuner_freq.firstChild );
  tuner_delta.style.opacity = Math.sqrt( v_confidence );
  while ( tuner_delta.firstChild )
    tuner_delta.removeChild( tuner_delta.firstChild );
  tuner_note.appendChild( document.createTextNode( tuner_notelabels[
    v_note ] ) );
  dsign = '+';
  if ( v_delta < 0 )
    dsign = '';
  tuner_delta.appendChild( document.createTextNode( dsign + v_delta.toFixed(
    0 ) + " Cent " ) );
  tuner_freq.appendChild( document.createTextNode( " " + v_freq.toFixed(
      1 ) +
    " Hz " + "(oct. " + v_octave.toFixed(
      0 ) + ")" ) );
} );
socket.on( 'tuner_strobe', function( strobe ) {
  if ( strobebuffer.length != strobe.length ) {
    strobebuffer = Array( strobe.length );
  }
  for ( let k = 0; k < Math.min( strobe.length, strobebuffer.length ); k++ )
    strobebuffer[ k ] = strobe[ k ];
  tuner_draw();
} );
socket.on( 'micangle', function( val ) {
  objmix_micangle = 57.296 * val;
} );
socket.on( 'tuner_getvar', function( path, val ) {
  if ( path == '/tuner/isactive' ) {
    tuner_active = document.getElementById( 'tuner_active' );
    tuner_active.checked = ( val > 0 );
    if ( !tuner_active.checked ) {
      tuner_note = document.getElementById( 'tuner_notedisplay' );
      tuner_freq = document.getElementById( 'tuner_freqdisplay' );
      tuner_delta = document.getElementById( 'tuner_deltadisplay' );
      tuner_note.style.opacity = 1.0;
      while ( tuner_note.firstChild )
        tuner_note.removeChild( tuner_note.firstChild );
      tuner_freq.style.opacity = 1.0;
      while ( tuner_freq.firstChild )
        tuner_freq.removeChild( tuner_freq.firstChild );
      tuner_delta.style.opacity = 1.0;
      while ( tuner_delta.firstChild )
        tuner_delta.removeChild( tuner_delta.firstChild );
      tuner_note.appendChild( document.createTextNode( "--" ) );
      tuner_freq.appendChild( document.createTextNode( "--" ) );
    }
  }
  if ( path == '/tuner/f0' ) {
    tuner_pitcha = document.getElementById( 'tuner_pitcha' );
    while ( tuner_pitcha.firstChild )
      tuner_pitcha.removeChild( tuner_pitcha.firstChild );
    tuner_pitcha.appendChild( document.createTextNode( 'a = ' + val.toFixed(
      1 ) + ' Hz' ) );
  }
  if ( path == '/tuner/tuning' ) {
    tuner_tuning = document.getElementById( 'tuner_tuning' );
    while ( tuner_tuning.firstChild )
      tuner_tuning.removeChild( tuner_tuning.firstChild );
    switch ( val ) {
      case "equal":
        val = "equal temperament";
        break;
      case "werkmeister3":
        val = "Werckmeister 3";
        tuner_notelabels = [ 'C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G',
          'G#', 'A', 'Bb',
          'B'
        ];
        break;
      case "werckmeister3":
        val = "Werckmeister 3";
        tuner_notelabels = [ 'C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G',
          'G#', 'A', 'Bb',
          'B'
        ];
        break;
      case "meantone4":
        val = "1/4 comma meantone";
        tuner_notelabels = [ 'C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G',
          'G#', 'A', 'Bb',
          'B'
        ];
        break;
      case "meantone4#":
        val = "1/4 comma meantone, sharps";
        tuner_notelabels = [ 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G',
          'G#', 'A', 'A#',
          'B'
        ];
        break;
      case "meantone4b":
        val = "1/4 comma meantone, flats";
        tuner_notelabels = [ 'C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G',
          'Ab', 'A', 'Bb',
          'B'
        ];
        break;
      case "meantone6":
        val = "1/6 comma meantone";
        break;
      case "valotti":
        val = "Vallotti";
        break;
      case "vallotti":
        val = "Vallotti";
        break;
      case "Bach-Kellner1977":
        val = "Bach-Kellner (1977)";
        break;
      case "Neidhardt-1724-GrosseStadt":
        val = "Neidhardt (1724) Grosse Stadt";
        break;
    }
    tuner_tuning.appendChild( document.createTextNode( val ) );
  }
} );
window.addEventListener( 'resize', function( event ) {
  objmix_draw();
}, true );



/*
 * Local Variables:
 * c-basic-offset: 2
 * compile-command: "js-beautify -P -s 2 -w 80 -r ovclient.js"
 * End:
 */