/**
 * OVBOX Web Mixer Server
 * 
 * This Node.js script creates a web-based interface for the OVBOX system. 
 * It serves an HTML client that acts as a mixer, recorder, panner, and instrument tuner.
 * It bridges communication between a web client (via Socket.IO) and the audio backend 
 * (via OSC - Open Sound Control).
 * 
 * Main Features:
 * - HTTP Server: Serves the web interface (HTML/CSS/JS).
 * - Socket.IO Server: Handles real-time bidirectional communication with the web client.
 * - OSC Server: Receives status updates (levels, positions, tuner data) from the audio backend.
 * - OSC Client: Sends control commands (fader moves, recording triggers) to the audio backend.
 */
// node-js file for the ovbox web mixer
// Import required modules
var http = require( 'http' );
var os = require( 'os' );
var fs = require( 'fs' );
var iolib = require( 'socket.io' );
var osc = require( 'node-osc' );
var path = require( 'path' );
const homedir = require( 'os' ).homedir();
// Global variables
var
vertexgain = {}; // Stores gain values and paths for audio vertices (sources/speakers)
var strobebuffer = Array( 10 ); // Buffer for tuner strobe data
var deviceid = ''; // Unique identifier for this device
// --- Device ID Initialization ---
{
  var devname = 'localhost';
  try {
    // Attempt to get the hostname from the OS
    devname = os.hostname();
  } catch ( ex ) {
    console.log( ex.message );
  }
  // Override with command line argument if provided (arg 3)
  if ( process.argv.length > 3 ) devname = process.argv[ 3 ];
  // Override with 'devicename' file if it exists in the local directory
  try {
    devname = fs.readFileSync( 'devicename' );
  } catch ( ee ) {}
  var devnames = devname.split( ' ' );
  deviceid = devnames[ 0 ];
}
// --- HTTP Server Setup ---
httpserver = http.createServer( function( req, res ) {
  // Check if the request is for a recorded audio file download
  if ( req.url.startsWith( '/rec' ) & ( req.url.endsWith( '.wav' ) || req
      .url.endsWith( '.aif' ) || req.url.endsWith( '.mat' ) || req.url
      .endsWith( '.flac' ) || req.url.endsWith( '.caf' ) ) ) {
    // Attempt to serve file from local directory
    if ( fs.existsSync( '.' + req.url ) ) {
      var data = fs.readFileSync( '.' + req.url );
      res.writeHead( 200 );
      res.end( data );
      return;
    }
    // Attempt to serve file from user home directory
    if ( fs.existsSync( homedir + req.url ) ) {
      var data = fs.readFileSync( homedir + req.url );
      res.writeHead( 200 );
      res.end( data );
      return;
    }
  }
  // Serve the main web interface
  var sdir = path.dirname( process.argv[ 1 ] );
  if ( sdir.length > 0 ) sdir = sdir + '/';
  // Read client-side assets
  var hosjs = fs.readFileSync( sdir + 'ovclient.js' );
  var hoscss = fs.readFileSync( sdir + 'ovclient.css' );
  var jackrec = fs.readFileSync( sdir + 'jackrec.html' );
  // Determine IP address and Device Name for display/connection
  var ipaddr = '127.0.0.1';
  try {
    ipaddr = os.hostname();
  } catch ( ex ) {
    console.log( ex.message );
  }
  if ( process.argv.length > 2 ) ipaddr = process.argv[ 2 ];
  var devname = 'localhost';
  try {
    devname = os.hostname();
  } catch ( ex ) {
    console.log( ex.message );
  }
  if ( process.argv.length > 3 ) devname = process.argv[ 3 ];
  try {
    devname = fs.readFileSync( 'devicename' );
  } catch ( ee ) {}
  var devnames = devname.split( ' ' );
  //deviceid = devnames[0];
  // Construct and send the HTML response
  res.writeHead( 200, {
    'Content-Type': 'text/html'
  } );
  res.write( '<!DOCTYPE HTML>\n' );
  res.write(
    '<html><head><meta name="viewport" content="width=device-width, initial-scale=1"><style>'
    );
  res.write( hoscss );
  res.write(
  '</style><title>ov-client web mixer</title>\n</head><body>\n' );
  res.write( '<h1>' + devname + '</h1>\n' );
  res.write( jackrec );
  res.write( '<script src="http://' + ipaddr +
    ':8080/socket.io/socket.io.js"></script>\n' );
  res.write( '<script>\n' );
  res.write( 'var socket = io("http://' + ipaddr + ':8080");\n' );
  res.write( hosjs );
  res.write( '</script>\n' );
  res.end( '</body></html>' );
} );
httpserver.listen( 8080 ); // Start listening on port 8080
// --- Socket.IO Setup ---
io = iolib( httpserver );
// --- OSC Setup ---
var oscServer, oscClient;
// Listen for OSC messages from the audio backend on port 9000
oscServer = new osc.Server( 9000, '0.0.0.0' );
// Send OSC messages to the audio backend (assumed to be on localhost:9871)
oscClient = new osc.Client( 'localhost', 9871 );
/**
 * Helper function to find overlapping strings.
 * Used to determine common prefixes in OSC paths.
 * 
 * @param {string} a - First string
 * @param {string} b - Second string
 * @returns {string} The overlapping substring
 */
function findOverlap( a, b ) {
  if ( b.length === 0 ) {
    return "";
  }
  if ( a.endsWith( b ) ) {
    return b;
  }
  if ( a.indexOf( b ) >= 0 ) {
    return b;
  }
  return findOverlap( a, b.substring( 0, b.length - 1 ) );
}
/**
 * Helper function for Array filtering to keep only unique values.
 * 
 * @param {*} value - The current element
 * @param {number} index - The index of the current element
 * @param {Array} self - The array itself
 * @returns {boolean} True if unique
 */
function onlyUnique( value, index, self ) {
  return self.indexOf( value ) === index;
}
// --- Socket.IO Connection Handler ---
io.on( 'connection', function( socket ) {
  // Send the device ID to the newly connected client
  socket.emit( 'deviceid', deviceid );
  /**
   * Handles 'objmixposcomplete' event from client.
   * Triggers a redraw of the mixer interface.
   */
  socket.on( 'objmixposcomplete', async function( obj ) {
    socket.emit( 'objmixredraw' );
  } );
  /**
   * Handles 'config' event from client.
   * Initializes the OSC listeners and requests initial state from the backend.
   */
  socket.on( 'config', function( obj ) {
    var varlist = {};
    // Notify backend that a client has connected
    oscClient.send( '/status', socket.id + ' connected' );
    // --- OSC Message Listener ---
    // This listener handles incoming messages from the audio backend and forwards them to the web client
    oscServer.on( 'message', async function( msg, rinfo ) {
      // --- Instrument Tuner Messages ---
      if ( msg[ 0 ] == '/tuner' ) {
        // Update tuner GUI (frequency, note, octave, delta, confidence)
        socket.emit( 'tuner', msg[ 1 ], msg[ 2 ], msg[ 3 ], msg[
          4 ], msg[ 5 ] );
      }
      if ( msg[ 0 ] == '/tuner/strobe' ) {
        // Handle strobe tuner visualization data
        if ( strobebuffer.length != msg.length - 1 )
          strobebuffer = Array( msg.length - 1 );
        for ( let k = 0; k < Math.min( strobebuffer.length, msg
            .length - 1 ); k++ ) strobebuffer[ k ] = msg[ k + 1 ];
        socket.emit( 'tuner_strobe', strobebuffer );
      }
      if ( msg[ 0 ] == '/micangle' ) {
        // Update microphone angle display
        socket.emit( 'micangle', msg[ 2 ] );
      }
      if ( msg[ 0 ] == '/tuner_getvar' ) {
        // Update tuner variable GUI
        socket.emit( 'tuner_getvar', msg[ 1 ], msg[ 2 ] );
      }
      // --- OSC Gain Control and Level Meter ---
      if ( msg[ 0 ] == '/touchosc/scene' ) {
        socket.emit( 'scene', 'scene' );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/label' ) && ( !msg[ 0 ]
          .endsWith( '/color' ) ) && ( msg[ 1 ].length > 1 ) ) {
        // Create a new fader based on label data
        socket.emit( 'newfader', msg[ 0 ].substr( 15 ), msg[
        1 ] );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/mute' ) && ( !msg[ 0 ]
          .endsWith( '/color' ) ) ) {
        // Update mute status
        socket.emit( 'updatemute', msg[ 0 ], msg[ 1 ] );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/fader' ) && ( !msg[ 0 ]
          .endsWith( '/color' ) ) ) {
        // Handle fader movement updates
        Object.entries( vertexgain ).forEach( ( [ vertexid,
          vgain
        ] ) => {
          oscClient.send( vgain.path + '/get',
            'osc.udp://localhost:9000/', '/soundgain' );
        } );
        if ( msg[ 1 ] != -Infinity ) socket.emit( 'updatefader',
          msg[ 0 ], msg[ 1 ] );
        else socket.emit( 'updatefader', msg[ 0 ], -80 );
        Object.entries( vertexgain ).forEach( ( [ vertexid,
          vgain
        ] ) => {
          socket.emit( 'vertexgain', vertexid, vgain.gain );
        } );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/level' ) ) {
        // Update level meter
        socket.emit( 'updatefader', msg[ 0 ], msg[ 1 ] );
      }
      // --- Vertex Positioning (Panner) ---
      if ( msg[ 0 ] == '/vertexpos' ) {
        // Parse vertex position data
        var vpvars = msg[ 1 ].split( '/' );
        var vpname = vpvars[ 2 ] + '.' + vpvars[ 3 ];
        if ( vpvars[ 2 ] == 'ego' ) vpname = vpvars[ 3 ];
        vpvars[ 2 ] = vpvars[ 2 ] + '.' + vpvars[ 3 ];
        vpvars.splice( 3 );
        const vertexid = vpvars.join( "/" );
        socket.emit( 'vertexpos', vertexid, vpname, msg[ 2 ], msg[
          3 ], msg[ 4 ], msg[ 1 ] );
        // Request gain update for this vertex
        var gainpath = msg[ 1 ].substring( 0, msg[ 1 ].length -
          10 ) + '/gain/get';
        oscClient.send( gainpath, 'osc.udp://localhost:9000/',
          '/soundgain' );
      }
      if ( msg[ 0 ] == '/tascarpos' ) {
        // Parse TASCAR position/rotation data
        var vpvars = msg[ 1 ].split( '/' );
        var vpname = vpvars[ 2 ];
        if ( vpvars.length > 3 ) vpname = vpvars[ 2 ] + '.' +
          vpvars[ 3 ];
        if ( vpvars[ 2 ] == 'ego' ) vpname = vpvars[ 3 ];
        vpvars.splice( 4 );
        const vertexid = vpvars.join( "/" );
        if ( ( vpvars[ 2 ] != 'reverb' ) && ( vpvars[ 2 ] !=
            'room' ) ) {
          // Forward position/rotation data to client (excluding reverb/room)
          socket.emit( 'vertexposrot', vertexid, vpname, msg[ 2 ],
            msg[ 3 ], msg[ 4 ], msg[ 5 ] * Math.PI / 180, msg[
              6 ] * Math.PI / 180, msg[ 7 ] * Math.PI / 180,
            msg[ 1 ] );
        }
      }
      if ( msg[ 0 ] == '/soundgain' ) {
        // Store gain values in the global vertexgain object
        var vpvars = msg[ 1 ].split( '/' );
        var vpname = vpvars[ 2 ] + '.' + vpvars[ 3 ];
        if ( vpvars[ 2 ] == 'ego' ) vpname = vpvars[ 3 ];
        vpvars[ 2 ] = vpvars[ 2 ] + '.' + vpvars[ 3 ];
        vpvars.splice( 3 );
        const vertexid = vpvars.join( "/" );
        if ( vertexid in vertexgain ) {
          vertexgain[ vertexid ].gain = msg[ 2 ];
          vertexgain[ vertexid ].path = msg[ 1 ];
        } else {
          vertexgain[ vertexid ] = {
            'gain': msg[ 2 ],
            'path': msg[ 1 ]
          };
        }
      }
      // --- Recorder Interface (Jackrec) ---
      if ( msg[ 0 ] == '/jackrec/start' ) socket.emit(
        'jackrecstart', '' );
      if ( msg[ 0 ] == '/jackrec/stop' ) socket.emit(
        'jackrecstop', '' );
      if ( msg[ 0 ] == '/jackrec/portlist' ) socket.emit(
        'jackrecportlist', '' );
      if ( msg[ 0 ] == '/jackrec/port' ) socket.emit(
        'jackrecaddport', msg[ 1 ] );
      if ( msg[ 0 ] == '/jackrec/filelist' ) socket.emit(
        'jackrecfilelist', '' );
      if ( msg[ 0 ] == '/jackrec/file' ) socket.emit(
        'jackrecaddfile', msg[ 1 ] );
      if ( msg[ 0 ] == '/jackrec/rectime' ) socket.emit(
        'jackrectime', msg[ 1 ] );
      if ( msg[ 0 ] == '/jackrec/error' ) socket.emit(
        'jackrecerr', msg[ 1 ] );
      if ( msg[ 0 ] == '/jackrec/enabledports/start' ) socket
        .emit( 'jackrecenabledports', 'start' );
      if ( msg[ 0 ] == '/jackrec/enabledport' ) socket.emit(
        'jackrecenabledport', msg[ 1 ] );
      // --- Variable List Handling (Dynamic OSC Variables) ---
      if ( msg[ 0 ] == '/varlist/getval' ) {
        // Update specific variable value on client
        if ( varlist[ msg[ 1 ] ] !== null ) {
          socket.emit( 'updatevar', msg[ 1 ].replace(
            /[^a-zA-Z0-9]/g, '' ), msg[ 2 ], varlist[ msg[
            1 ] ].type );
        }
      }
      if ( msg[ 0 ] == '/varlist/begin' )
      varlist = {}; // Clear list
      if ( msg[ 0 ] == '/varlist' ) {
        // Parse variable definitions (float or bool)
        if ( ( msg[ 2 ] == 'f' ) && ( msg[ 3 ] > 0 ) ) {
          var grps = msg[ 1 ].split( '/' );
          if ( grps.length > 3 ) varlist[ msg[ 1 ] ] = {
            'path': msg[ 1 ],
            'range': msg[ 4 ],
            'comment': msg[ 5 ],
            'label': msg[ 1 ],
            'type': 'float'
          };
        } else {
          if ( ( msg[ 2 ] == 'i' ) && ( msg[ 4 ] == 'bool' ) ) {
            var grps = msg[ 1 ].split( '/' );
            if ( grps.length > 3 ) varlist[ msg[ 1 ] ] = {
              'path': msg[ 1 ],
              'range': msg[ 4 ],
              'comment': msg[ 5 ],
              'label': msg[ 1 ],
              'type': 'bool'
            };
          }
        }
      }
      if ( msg[ 0 ] == '/varlist/end' ) {
        // Process the collected variable list to build a hierarchy
        var parents = [];
        var sparents = [];
        for ( const key in varlist ) {
          var grps = varlist[ key ].path.split( '/' );
          varlist[ key ].id = varlist[ key ].path.replace(
            /[^a-zA-Z0-9]/g, '' );
          varlist[ key ].label = grps.pop();
          varlist[ key ].parent = grps.join( '' );
          while ( grps.length > 0 ) {
            const level = grps.length;
            var parent = grps.join( '' );
            var grapa = null;
            var grlab = grps.pop();
            if ( grlab ) {
              grapa = grps.join( '' );
              grlab = grlab.replace( 'bus.', '' );
            }
            if ( grlab && ( grlab.length > 0 ) ) {
              if ( sparents.indexOf( parent ) < 0 ) {
                parents.unshift( {
                  'id': parent,
                  'parent': grapa,
                  'label': grlab,
                  'level': level
                } );
                sparents.push( parent );
              }
            }
          }
        }
        // Sort parents by level and label
        parents.sort( ( a, b ) => {
          if ( a.level != b.level ) return a.level - b.level;
          if ( b.label < a.label ) return 1;
          if ( b.label > a.label ) return -1;
          return 0;
        } );
        // Send the structured variable list to the client
        socket.emit( 'oscvarlist', parents, varlist );
        // Request current values for all variables
        for ( const key in varlist ) {
          const v = varlist[ key ];
          oscClient.send( v.path + '/get',
            'osc.udp://localhost:9000/', '/varlist/getval' );
        }
      }
    } );
    // --- Initial OSC Requests ---
    // Trigger backend to send current state
    oscClient.send( '/touchosc/connect', 16 );
    oscClient.send( '/jackrec/listports' );
    oscClient.send( '/jackrec/listfiles' );
    oscClient.send( '/sendvarsto', 'osc.udp://localhost:9000/',
      '/varlist', '/bus.' );
    // Request positions and tuner status
    oscClient.send( '/*/globalpos/get', 'osc.udp://localhost:9000/',
      '/vertexpos' );
    oscClient.send( '/tuner/isactive/get', 'osc.udp://localhost:9000/',
      '/tuner_getvar' );
    oscClient.send( '/tuner/f0/get', 'osc.udp://localhost:9000/',
      '/tuner_getvar' );
    oscClient.send( '/tuner/tuning/get', 'osc.udp://localhost:9000/',
      '/tuner_getvar' );
    oscClient.send( '/*/main/ortf/angle/get',
      'osc.udp://localhost:9000/', '/micangle' );
  } );
  /**
   * Handles generic 'message' events from client.
   * Forwards raw OSC messages to the backend.
   */
  socket.on( 'message', function( obj ) {
    oscClient.send( obj );
  } );
  /**
   * Handles structured 'msg' events from client.
   * Expects an object with 'path' and optional 'value'.
   * Forwards to the backend.
   */
  socket.on( 'msg', function( obj ) {
    if ( obj.hasOwnProperty( 'value' ) && ( obj.value != null ) ) {
      oscClient.send( obj.path, obj.value );
    } else {
      oscClient.send( obj.path );
    }
  } );
} );
/*
 * Local Variables:
 * c-basic-offset: 2
 * compile-command: "js-beautify -d -P -s 2 -w 80 -r webmixer.js"
 * End:
 */