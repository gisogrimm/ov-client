// node-js file for the ovbox web mixer
var http = require( 'http' );
var os = require( 'os' );
var fs = require( 'fs' );
var iolib = require( 'socket.io' );
var osc = require( 'node-osc' );
var path = require( 'path' );
const homedir = require( 'os' ).homedir();
var vertexgain = {};
var deviceid = ''; {
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
  deviceid = devnames[ 0 ];
}
httpserver = http.createServer( function( req, res ) {
  // check if file is in local directory:
  if ( req.url.startsWith( '/rec' ) & ( req.url.endsWith( '.wav' ) || req
      .url.endsWith( '.aif' ) || req.url.endsWith( '.mat' ) || req.url
      .endsWith( '.flac' ) || req.url.endsWith( '.caf' ) ) ) {
    // download from local directory:
    if ( fs.existsSync( '.' + req.url ) ) {
      var data = fs.readFileSync( '.' + req.url );
      res.writeHead( 200 );
      res.end( data );
      return;
    }
    // check in home directory:
    if ( fs.existsSync( homedir + req.url ) ) {
      var data = fs.readFileSync( homedir + req.url );
      res.writeHead( 200 );
      res.end( data );
      return;
    }
  }
  var sdir = path.dirname( process.argv[ 1 ] );
  if ( sdir.length > 0 ) sdir = sdir + '/';
  var hosjs = fs.readFileSync( sdir + 'ovclient.js' );
  var hoscss = fs.readFileSync( sdir + 'ovclient.css' );
  var jackrec = fs.readFileSync( sdir + 'jackrec.html' );
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
httpserver.listen( 8080 );
io = iolib( httpserver );
var oscServer, oscClient;
oscServer = new osc.Server( 9000, '0.0.0.0' );
oscClient = new osc.Client( 'localhost', 9871 );

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

function onlyUnique( value, index, self ) {
  return self.indexOf( value ) === index;
}
io.on( 'connection', function( socket ) {
  socket.emit( 'deviceid', deviceid );
  socket.on( 'objmixposcomplete', async function( obj ) {
    socket.emit( 'objmixredraw' );
  } );
  socket.on( 'config', function( obj ) {
    var varlist = {};
    oscClient.send( '/status', socket.id + ' connected' );
    oscServer.on( 'message', async function( msg, rinfo ) {
      // instrument tuner:
      if ( msg[ 0 ] == '/tuner' ) {
        // update tuner GUI (frequency, note, octave, delta, confidence):
        socket.emit( 'tuner', msg[ 1 ], msg[ 2 ], msg[ 3 ], msg[
          4 ], msg[ 5 ] );
      }
      if ( msg[ 0 ] == '/tuner_getvar' ) {
        // update tuner GUI (frequency, note, octave, delta, confidence):
          socket.emit( 'tuner_getvar', msg[ 1 ], msg[ 2 ] );
      }
      // OSC gain control and level meter:
      if ( msg[ 0 ] == '/touchosc/scene' ) {
        socket.emit( 'scene', 'scene' );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/label' ) && ( !msg[ 0 ]
          .endsWith( '/color' ) ) && ( msg[ 1 ].length > 1 ) ) {
        socket.emit( 'newfader', msg[ 0 ].substr( 15 ), msg[
        1 ] );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/mute' ) && ( !msg[ 0 ]
          .endsWith( '/color' ) ) ) {
        socket.emit( 'updatemute', msg[ 0 ], msg[ 1 ] );
      }
      if ( msg[ 0 ].startsWith( '/touchosc/fader' ) && ( !msg[ 0 ]
          .endsWith( '/color' ) ) ) {
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
        socket.emit( 'updatefader', msg[ 0 ], msg[ 1 ] );
      }
      if ( msg[ 0 ] == '/vertexpos' ) {
        var vpvars = msg[ 1 ].split( '/' );
        var vpname = vpvars[ 2 ] + '.' + vpvars[ 3 ];
        if ( vpvars[ 2 ] == 'ego' ) vpname = vpvars[ 3 ];
        vpvars[ 2 ] = vpvars[ 2 ] + '.' + vpvars[ 3 ];
        vpvars.splice( 3 );
        const vertexid = vpvars.join( "/" );
        socket.emit( 'vertexpos', vertexid, vpname, msg[ 2 ], msg[
          3 ], msg[ 4 ], msg[ 1 ] );
        var gainpath = msg[ 1 ].substring( 0, msg[ 1 ].length -
          10 ) + '/gain/get';
        oscClient.send( gainpath, 'osc.udp://localhost:9000/',
          '/soundgain' );
      }
      if ( msg[ 0 ] == '/tascarpos' ) {
        var vpvars = msg[ 1 ].split( '/' );
        var vpname = vpvars[ 2 ];
        if ( vpvars.length > 3 ) vpname = vpvars[ 2 ] + '.' +
          vpvars[ 3 ];
        if ( vpvars[ 2 ] == 'ego' ) vpname = vpvars[ 3 ];
        vpvars.splice( 4 );
        const vertexid = vpvars.join( "/" );
        if ( ( vpvars[ 2 ] != 'reverb' ) && ( vpvars[ 2 ] !=
            'room' ) ) {
          // forward data
          socket.emit( 'vertexposrot', vertexid, vpname, msg[ 2 ],
            msg[ 3 ], msg[ 4 ], msg[ 5 ] * Math.PI / 180, msg[
              6 ] * Math.PI / 180, msg[ 7 ] * Math.PI / 180,
            msg[ 1 ] );
        }
      }
      if ( msg[ 0 ] == '/soundgain' ) {
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
      if ( msg[ 0 ] == '/varlist/getval' ) {
        if ( varlist[ msg[ 1 ] ] !== null ) {
          socket.emit( 'updatevar', msg[ 1 ].replace(
            /[^a-zA-Z0-9]/g, '' ), msg[ 2 ], varlist[ msg[
            1 ] ].type );
        }
      }
      if ( msg[ 0 ] == '/varlist/begin' ) varlist = {};
      if ( msg[ 0 ] == '/varlist' ) {
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
        parents.sort( ( a, b ) => {
          if ( a.level != b.level ) return a.level - b.level;
          if ( b.label < a.label ) return 1;
          if ( b.label > a.label ) return -1;
          return 0;
        } );
        socket.emit( 'oscvarlist', parents, varlist );
        for ( const key in varlist ) {
          const v = varlist[ key ];
          oscClient.send( v.path + '/get',
            'osc.udp://localhost:9000/', '/varlist/getval' );
        }
      }
    } );
    oscClient.send( '/touchosc/connect', 16 );
    oscClient.send( '/jackrec/listports' );
    oscClient.send( '/jackrec/listfiles' );
    oscClient.send( '/sendvarsto', 'osc.udp://localhost:9000/',
      '/varlist', '/bus.' );
    //oscClient.send('/*/ego/*/pos/get', 'osc.udp://localhost:9000/', '/vertexpos');
    oscClient.send( '/*/globalpos/get', 'osc.udp://localhost:9000/',
                    '/vertexpos' );
      oscClient.send('/tuner/isactive/get','osc.udp://localhost:9000/', '/tuner_getvar');
      oscClient.send('/tuner/f0/get','osc.udp://localhost:9000/', '/tuner_getvar');
      oscClient.send('/tuner/tuning/get','osc.udp://localhost:9000/', '/tuner_getvar');
  } );
  socket.on( 'message', function( obj ) {
    oscClient.send( obj );
  } );
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
