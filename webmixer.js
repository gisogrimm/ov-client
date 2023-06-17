// node-js file for the ovbox web mixer

var http = require('http');
var os = require('os');
var fs = require('fs');
var iolib = require('socket.io');
var osc = require('node-osc');
var path = require('path');

const homedir = require('os').homedir();

var deviceid = '';

{
    var devname = os.hostname();
    if( process.argv.length > 3 )
        devname = process.argv[3];
    try{
        devname = fs.readFileSync('devicename');
    }
    catch(ee){
    }
    var devnames = devname.split(' ');
    deviceid = devnames[0];
}

httpserver = http.createServer(function (req, res) {
    // check if file is in local directory:
    if( req.url.startsWith('/rec') & (
        req.url.endsWith('.wav')||
            req.url.endsWith('.aif')||
            req.url.endsWith('.mat')||
            req.url.endsWith('.flac')||
            req.url.endsWith('.caf')
    )){
	// download from local directory:
	if( fs.existsSync('.'+req.url) ){
	    var data = fs.readFileSync('.'+req.url);
	    res.writeHead(200);
	    res.end(data);
	    return;
	}
	// check in home directory:
	if( fs.existsSync(homedir+req.url) ){
	    var data = fs.readFileSync(homedir+req.url);
	    res.writeHead(200);
	    res.end(data);
	    return;
	}
    }
    var sdir = path.dirname(process.argv[1]);
    if( sdir.length > 0 )
        sdir = sdir + '/';
    var hosjs = fs.readFileSync(sdir+'ovclient.js');
    var hoscss = fs.readFileSync(sdir+'ovclient.css');
    var jackrec = fs.readFileSync(sdir+'jackrec.html');
    var ipaddr = os.hostname();
    if( process.argv.length > 2 )
	ipaddr = process.argv[2];
    var devname = os.hostname();
    if( process.argv.length > 3 )
	devname = process.argv[3];
    try{
	devname = fs.readFileSync('devicename');
    }
    catch(ee){
    }
    var devnames = devname.split(' ');
    //deviceid = devnames[0];
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write('<!DOCTYPE HTML>\n');
    res.write('<html><head><style>');
    res.write(hoscss);
    res.write('</style><title>ov-client web mixer</title>\n</head><body>\n');
    res.write('<h1>'+devname+'</h1>\n<div id="mixer">mixer</div><div id="plugpars"></div>\n');
    res.write('<script src="http://'+ipaddr+':8080/socket.io/socket.io.js"></script>\n');
    res.write('<script>\n');
    res.write('var socket = io("http://'+ipaddr+':8080");\n');
    res.write(hosjs);
    res.write('</script>\n');
    if( req.url.startsWith('/objmix') ){
        res.write('<div id="objmix" class="objmix">\n');
        res.write('<canvas id="objmixer">object mixer</canvas>\n');
        res.write('<br/>\n');
        res.write('<div class="objmixctl">\n');
        res.write('<input class="ctlbutton" type="button" value="store positions and gains" onclick="objmix_upload_posandgains();">\n');
        res.write('</div>\n');
        res.write('</div>\n');
        res.write('<div class="objmixctl">\n');
        res.write('<a href="/">mixer + audio recorder</a>\n');
        res.write('</div>\n');
    }else{
        res.write(jackrec);
        res.write('<div class="objmixctl">\n');
        res.write('<a href="/objmix">object base mixer</a>\n');
        res.write('</div>\n');
    }
    res.end('</body></html>');
});

httpserver.listen(8080);
io = iolib(httpserver);

var oscServer, oscClient;

oscServer = new osc.Server( 9000, '0.0.0.0' );
oscClient = new osc.Client( 'localhost', 9871 );

function findOverlap(a, b) {
    if (b.length === 0) {
        return "";
    }
    if (a.endsWith(b)) {
        return b;
    }
    if (a.indexOf(b) >= 0) {
        return b;
    }
    return findOverlap(a, b.substring(0, b.length - 1));
}

function onlyUnique(value, index, self) {
    return self.indexOf(value) === index;
}

io.on('connection', function (socket) {
    socket.emit('deviceid',deviceid);
    socket.on('objmixposcomplete', async function (obj) {
        socket.emit('objmixredraw');
    });
    socket.on('config', function (obj) {
        var varlist = {};
	oscClient.send('/status', socket.id + ' connected');
	oscServer.on('message', async function(msg, rinfo) {
	    if( msg[0] == '/touchosc/scene' ){
		socket.emit('scene', 'scene');
	    }
	    if( msg[0].startsWith('/touchosc/label') && (!msg[0].endsWith('/color')) && (msg[1].length>1)){
		socket.emit('newfader', msg[0].substr(15), msg[1] );
	    }
	    if( msg[0].startsWith('/touchosc/fader') && (!msg[0].endsWith('/color')) ){
		socket.emit('updatefader', msg[0], msg[1] );
	    }
	    if( msg[0].startsWith('/touchosc/level') ){
		socket.emit('updatefader', msg[0], msg[1] );
	    }
            if( msg[0] == '/vertexpos' ){
                const vpvars = msg[1].split('/');
                //console.log(vpvars);
                socket.emit('vertexpos', vpvars[3], msg[2], msg[3], msg[4] );
            }
	    if( msg[0] == '/jackrec/start' )
		socket.emit('jackrecstart', '');
	    if( msg[0] == '/jackrec/stop' )
		socket.emit('jackrecstop', '');
	    if( msg[0] == '/jackrec/portlist' )
		socket.emit('jackrecportlist', '');
	    if( msg[0] == '/jackrec/port' )
		socket.emit('jackrecaddport', msg[1] );
	    if( msg[0] == '/jackrec/filelist' )
		socket.emit('jackrecfilelist', '');
	    if( msg[0] == '/jackrec/file' )
		socket.emit('jackrecaddfile', msg[1] );
	    if( msg[0] == '/jackrec/rectime' )
		socket.emit('jackrectime', msg[1] );
	    if( msg[0] == '/jackrec/error' )
		socket.emit('jackrecerr', msg[1] );
	    if( msg[0] == '/varlist/getval' ){
                if( varlist[msg[1]] !== null ){
                    socket.emit('updatevar',msg[1].replace(/[^a-zA-Z0-9]/g,''),msg[2]);
                }
            }
	    if( msg[0] == '/varlist/begin' )
                varlist = {};
	    if( msg[0] == '/varlist' ){
                if( (msg[2] == 'f') && (msg[3]>0) ){
                    var grps = msg[1].split('/');
                    if( grps.length > 3 )
                        varlist[msg[1]] = {'path':msg[1],'range':msg[4],'comment':msg[5],'label':msg[1]};
                }
            }
	    if( msg[0] == '/varlist/end' ){
                var parents = [];
                var sparents = [];
                for(const key in varlist){
                    var grps = varlist[key].path.split('/');
                    varlist[key].id = varlist[key].path.replace(/[^a-zA-Z0-9]/g,'');
                    varlist[key].label = grps.pop();
                    varlist[key].parent = grps.join('');
                    while( grps.length > 0 ){
                        const level = grps.length;
                        var parent = grps.join('');
                        var grapa = null;
                        var grlab = grps.pop();
                        if( grlab ){
                            grapa = grps.join('');
                            grlab = grlab.replace('bus.','');
                        }
                        if( grlab && (grlab.length > 0) ){
                            if( sparents.indexOf(parent) < 0 ){
                                parents.unshift({'id':parent,'parent':grapa,'label':grlab,'level':level});
                                sparents.push(parent);
                            }
                        }
                    }
                }
                parents.sort((a, b) => {
                    if( a.level != b.level ) return a.level-b.level;
                    if( b.label < a.label ) return 1;
                    if( b.label > a.label ) return -1;
                    return 0;
                } );
                socket.emit('oscvarlist',parents,varlist);
                for(const key in varlist){
                    const v = varlist[key];
                    oscClient.send(v.path+'/get','osc.udp://localhost:9000/','/varlist/getval');
                }
            }
	});
	oscClient.send('/touchosc/connect',16);
	oscClient.send('/jackrec/listports');
	oscClient.send('/jackrec/listfiles');
        oscClient.send('/sendvarsto','osc.udp://localhost:9000/','/varlist','/bus.');
        oscClient.send('/*/ego/*/pos/get', 'osc.udp://localhost:9000/', '/vertexpos');
    });
    socket.on('message', function (obj) {
	oscClient.send(obj);
    });
    socket.on('msg', function (obj) {
	if( obj.hasOwnProperty('value') && (obj.value != null) ){
	    oscClient.send( obj.path, obj.value );
	}else{
	    oscClient.send( obj.path );
	}
    });
});
