var deviceid = '';
var inchannelpos = {};
var objmix_sel = -1;
var objmix_drag = false;
var recpos = {'x':0,'y':0,'z':0,'rz':0,'ry':0,'rx':0};

/* accepts parameters
 * h  Object = {h:x, s:y, v:z}
 * OR
 * h, s, v
 */
function HSVtoRGB(h, s, v) {
    var r, g, b, i, f, p, q, t;
    if (arguments.length === 1) {
        s = h.s, v = h.v, h = h.h;
    }
    i = Math.floor(h * 6);
    f = h * 6 - i;
    p = v * (1 - s);
    q = v * (1 - f * s);
    t = v * (1 - (1 - f) * s);
    switch (i % 6) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    }
    return {
        r: Math.round(r * 255),
        g: Math.round(g * 255),
        b: Math.round(b * 255)
    };
}

function objmix_getscale( w, h )
{
    return 0.11*Math.min(w,h);
}

function euler_rotz( pos, a )
{
    return {x:Math.cos(a) * pos.x - Math.sin(a) * pos.y,
	    y:Math.cos(a) * pos.y + Math.sin(a) * pos.x,
	    z:pos.z};
}

function euler_roty( pos, a )
{
    return {x:Math.cos(a) * pos.x + Math.sin(a) * pos.z,
	    y:pos.y,
	    z:Math.cos(a) * pos.z - Math.sin(a) * pos.x};
}

function euler_rotx( pos, a )
{
    return {x:pos.x,
	    y:Math.cos(a) * pos.y - Math.sin(a) * pos.z,
	    z:Math.cos(a) * pos.z + Math.sin(a) * pos.y};
}

function euler( pos, rz, ry, rx )
{
    return euler_rotx(euler_roty(euler_rotz(pos,rz),ry),rx);
}

function euler_loc( pos, rz, ry, rx )
{
    return euler_rotz(euler_roty(euler_rotx(pos,rx),ry),rz);
}

function pos2scr( pos )
{
    const canvas = document.getElementById("objmixer");
    const scale = objmix_getscale(canvas.width, canvas.height);
    if( Array.isArray(pos) )
	return {x:(0.5*canvas.width-scale*pos[1]),y:0.5*canvas.height-scale*pos[0]};
    return {x:(0.5*canvas.width-scale*pos.y),y:0.5*canvas.height-scale*pos.x};
}

function scr2pos( pos )
{
    const canvas = document.getElementById("objmixer");
    const scale = objmix_getscale(canvas.width, canvas.height);
    return {y:-(pos.x-0.5*canvas.width)/scale,
            x:(-pos.y+0.5*canvas.height)/scale};
}

function on_canvas_click( e )
{
    const canvas = document.getElementById("objmixer");
    var rect = canvas.getBoundingClientRect();
    var ksel = null;
    for( [key,vertex] of Object.entries(inchannelpos) ){
        var pos = pos2scr([vertex.x,vertex.y,vertex.z]);
        pos.x = pos.x - e.clientX + rect.left;
        pos.y = pos.y - e.clientY + rect.top;
        const d = Math.sqrt(pos.x*pos.x + pos.y*pos.y);
        // click is not more than 24 pixels away:
        if( d < 24 )
            ksel = key;
    }
    //for( var k=0; k<inchannelpos.length;k++){
    //    const vertex = inchannelpos[k];
    //    var pos = pos2scr([vertex.x,vertex.y,vertex.z]);
    //    pos.x = pos.x - e.clientX + rect.left;
    //    pos.y = pos.y - e.clientY + rect.top;
    //    const d = Math.sqrt(pos.x*pos.x + pos.y*pos.y);
    //    if( d < 24 )
    //        ksel = k;
    //}
    objmix_sel = ksel;
    if( ksel ){
        objmix_drag = true;
    }
}

function on_canvas_up( e )
{
    if( objmix_drag ){
        objmix_drag = false;
        socket.emit('objmixposcomplete');
        e.preventDefault();
    }
}

function on_canvas_move( e )
{
    if( objmix_drag ){
        const canvas = document.getElementById("objmixer");
        if( !canvas )
            return;
        var rect = canvas.getBoundingClientRect();
        const np = scr2pos({x: e.clientX-rect.left, y: e.clientY-rect.top});
        inchannelpos[objmix_sel].x = np.x;
        inchannelpos[objmix_sel].y = np.y;
        objmix_draw();
        var obj = {path:inchannelpos[objmix_sel].path,
               value:[inchannelpos[objmix_sel].x,inchannelpos[objmix_sel].y,inchannelpos[objmix_sel].z]}
        socket.emit("msg",obj);
        e.preventDefault();
    }
}

function draw_receiver( ctx, pos, rz, ry, rx )
{
    const scale = 0.1;
    const msize = 2;
    // calculate positions:
    var p1 = {x:1.8 * msize * scale, y:-0.6 * msize * scale, z:0};
    var p2 = {x:2.9 * msize * scale, y:0, z:0};
    var p3 = {x:1.8 * msize * scale, y:0.6 * msize * scale, z:0};
    var p4 = {x:-0.5 * msize * scale, y:2.3 * msize * scale, z:0};
    var p5 = {x:0, y:1.7 * msize * scale, z:0};
    var p6 = {x:0.5 * msize * scale, y:2.3 * msize * scale, z:0};
    var p7 = {x:-0.5 * msize * scale, y:-2.3 * msize * scale, z:0};
    var p8 = {x:0, y:-1.7 * msize * scale, z:0};
    var p9 = {x:0.5 * msize * scale, y:-2.3 * msize * scale, z:0};
    p1 = euler( p1, rz, ry, rx );
    p2 = euler( p2, rz, ry, rx );
    p3 = euler( p3, rz, ry, rx );
    p4 = euler( p4, rz, ry, rx );
    p5 = euler( p5, rz, ry, rx );
    p6 = euler( p6, rz, ry, rx );
    p7 = euler( p7, rz, ry, rx );
    p8 = euler( p8, rz, ry, rx );
    p9 = euler( p9, rz, ry, rx );
    p1.x += pos.x; p1.y += pos.y; p1.z += pos.z;
    p2.x += pos.x; p2.y += pos.y; p2.z += pos.z;
    p3.x += pos.x; p3.y += pos.y; p3.z += pos.z;
    p4.x += pos.x; p4.y += pos.y; p4.z += pos.z;
    p5.x += pos.x; p5.y += pos.y; p5.z += pos.z;
    p6.x += pos.x; p6.y += pos.y; p6.z += pos.z;
    p7.x += pos.x; p7.y += pos.y; p7.z += pos.z;
    p8.x += pos.x; p8.y += pos.y; p8.z += pos.z;
    p9.x += pos.x; p9.y += pos.y; p9.z += pos.z;
    pos = pos2scr(pos);
    p1 = pos2scr(p1);
    p2 = pos2scr(p2);
    p3 = pos2scr(p3);
    p4 = pos2scr(p4);
    p5 = pos2scr(p5);
    p6 = pos2scr(p6);
    p7 = pos2scr(p7);
    p8 = pos2scr(p8);
    p9 = pos2scr(p9);
    const msizescr1 = pos2scr([0,0]);
    const msizescr2 = pos2scr([scale*msize,scale*msize]);
    const msizescr = Math.sqrt(Math.pow(msizescr2.x-msizescr1.x,2) + Math.pow(msizescr2.y-msizescr1.y,2));
    // start drawing:
    ctx.save();
    const ls = ctx.lineWidth;
    ctx.lineWidth = 2*msize;
    ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
    ctx.moveTo(pos.x + 1.4*msizescr, pos.y);
    ctx.arc(pos.x, pos.y, 1.4*msizescr, 0, 2.0*Math.PI);
    ctx.moveTo(p1.x, p1.y);
    ctx.lineTo(p2.x, p2.y);
    ctx.lineTo(p3.x, p3.y);
    ctx.moveTo(p4.x, p4.y);
    ctx.lineTo(p5.x, p5.y);
    ctx.lineTo(p6.x, p6.y);
    ctx.moveTo(p7.x, p7.y);
    ctx.lineTo(p8.x, p8.y);
    ctx.lineTo(p9.x, p9.y);
    ctx.stroke();
    ctx.lineWidth = ls;
    ctx.restore();
}

function objmix_draw()
{
    const canvas = document.getElementById("objmixer");
    if( !canvas )
        return;
    canvas.width = canvas.parentElement.clientWidth-2;
    canvas.height = canvas.width;
    const ctx = canvas.getContext("2d");
    //ctx.globalCompositeOperation = "destination-over";
    ctx.fillStyle = '#153d17';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.save();
    ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
    ctx.beginPath();
    p0 = pos2scr( [0,0] );
    p1 = pos2scr( [0,4] );
    p2 = pos2scr( [0,-4] );
    p3 = pos2scr( [4,0] );
    ctx.moveTo(p1.x,p1.y);
    ctx.lineTo(p2.x,p2.y);
    ctx.moveTo(p0.x,p0.y);
    ctx.lineTo(p3.x,p3.y);
    ctx.moveTo(p0.x,p0.y);
    ctx.stroke();
    ctx.beginPath();
    ctx.arc(p0.x,p0.y,Math.abs(p1.x-p0.x),0,2.0*Math.PI,true);
    ctx.stroke();
    ctx.beginPath();
    ctx.arc(p0.x,p0.y,0.25*Math.abs(p1.x-p0.x),0,2.0*Math.PI,true);
    ctx.stroke();
    ctx.restore();
    ctx.save();
    ctx.font = "24px sans";
    //canvas.width = canvas.width;
    var k = 0;
    var N = Object.entries(inchannelpos).length;
    for( [key,vertex] of Object.entries(inchannelpos) ){
	var px = euler({x:0.5,y:0,z:0}, vertex.rz, vertex.ry, vertex.rx );
	px.x += vertex.x;
	px.y += vertex.y;
	px.z += vertex.z;
        const pos = pos2scr(vertex);
	const pos_x = pos2scr(px);
        const colrgb = HSVtoRGB(k/N, 0.65, 0.8 );
	ctx.lineWidth = 3;
	ctx.strokeStyle = `rgb(${colrgb.r},${colrgb.g},${colrgb.b})`;
        ctx.fillStyle = `rgb(${colrgb.r},${colrgb.g},${colrgb.b})`;
	ctx.beginPath();
	ctx.moveTo(pos.x,pos.y);
	ctx.lineTo(pos_x.x,pos_x.y);
	ctx.stroke();
        ctx.beginPath();
        ctx.arc(pos.x, pos.y, 20*Math.sqrt(canvas.width/1000), 0, Math.PI * 2);
        ctx.fill();
        ctx.fillText(vertex.name, pos.x+24, pos.y-5);
        k = k+1;
    }
    ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
    draw_receiver(ctx,recpos, recpos.rz, recpos.ry, recpos.rx );
    ctx.strokeStyle = "rgba(200, 200, 200, 0.5)";
    ctx.restore();
}

function update_objmix_sounds()
{
    const canvas = document.getElementById("objmixer");
    if( !canvas )
        return;
    canvas.width = canvas.parentElement.clientWidth;
    canvas.height = 0.55*canvas.width;
    canvas.addEventListener( 'pointerdown', on_canvas_click );
    canvas.addEventListener( 'pointerup', on_canvas_up );
    canvas.addEventListener( 'pointermove', on_canvas_move );
    objmix_draw();
}

socket.on("connect", function() {
    inchannelpos = {};
    objmix_sel = null;
    objmix_drag = false;
    recpos = {'x':0,'y':0,'z':0,'rz':0,'ry':0,'rx':0};
    socket.emit("config",{});
});

socket.on('deviceid',function(id){deviceid=id;});
socket.on("scene", function(scene){
    let el=document.getElementById("mixer");
    if( el ){
        while (el.firstChild) {el.removeChild(el.firstChild);}
        let elheader=document.createElement("h2");
        elheader.setAttribute("class","scene");
        el.append(scene,elheader);
        let elgainstore=document.createElement("p");
        elgainstore.setAttribute("class","gainstore");
        el.appendChild(elgainstore);
    }
});

socket.on("vertexpos", function(vertexid, name, x, y, z, path){
    if( Reflect.has(inchannelpos,vertexid) ){
	inchannelpos[vertexid].name = name;
	inchannelpos[vertexid].x = x;
	inchannelpos[vertexid].y = y;
	inchannelpos[vertexid].z = z;
	inchannelpos[vertexid].path = path;
    }else{
	inchannelpos[vertexid] = {'name':name,
				  'x':x, 'y':y, 'z':z,
				  'rz':0,'ry':0,'rx':0,
				  'path' : path};
    }
    update_objmix_sounds();
});

socket.on("vertexposrot", function(vertexid, name, x, y, z, rz, ry, rx, path){
    if( Reflect.has(inchannelpos,vertexid) ){
	var need_update = false;
	if( (x!=inchannelpos[vertexid].x) ||
	    (y!=inchannelpos[vertexid].y) ||
	    (z!=inchannelpos[vertexid].z) ||
	    (rx!=inchannelpos[vertexid].rx) ||
	    (ry!=inchannelpos[vertexid].ry) ||
	    (rz!=inchannelpos[vertexid].rz) )
	    need_update = true;
	inchannelpos[vertexid].x = x;
	inchannelpos[vertexid].y = y;
	inchannelpos[vertexid].z = z;
	inchannelpos[vertexid].rx = rx;
	inchannelpos[vertexid].ry = ry;
	inchannelpos[vertexid].rz = rz;
	if( need_update )
	    objmix_draw();
    }else{
	if( name === 'main' ){
	    var need_update = false;
	    if( (x!=recpos.x) ||
		(y!=recpos.y) ||
		(z!=recpos.z) ||
		(rx!=recpos.rx) ||
		(ry!=recpos.ry) ||
		(rz!=recpos.rz) )
		need_update = true;
	    recpos.x = x;
	    recpos.y = y;
	    recpos.z = z;
	    recpos.rx = rx;
	    recpos.ry = ry;
	    recpos.rz = rz;
	    if( need_update )
		objmix_draw();
	}
    }
    //inchannelpos[vertexid] = {'name':name,'x':x, 'y':y, 'z': z, 'path' : path};
    //update_objmix_sounds();
});
socket.on("newfader", function(faderno,val){
    // remove effect bus from mixer:
    if( val.startsWith('bus.') ){
        return;
    }
    fader="/touchosc/fader"+faderno;
    levelid="/touchosc/level"+faderno;
    muteid="/touchosc/mute"+faderno;
    let el_mixer=document.getElementById("mixer");
    if( el_mixer ){
        orig_val = val;
        let classname = "mixerstrip";
        val = val.replace('.'+deviceid,'');
        val = val.replace(deviceid+'.','');
        val = val.replace('bus.','');
        if( val.startsWith("ego.")||(val == "monitor") ){
	    classname = classname + " mixerego";
            val = val.replace('ego.','');
        }
        if( (val == "main") || (val == "reverb") )
	    classname = classname + " mixerother";
        // create main mixer strip element:
        let el_mixerstrip = el_mixer.appendChild(document.createElement("div"));
        el_mixerstrip.setAttribute("class",classname);
        el_mixerstrip.setAttribute("id","mixerstrip_"+orig_val);
        let el_row1 = el_mixerstrip.appendChild(document.createElement("div"));
        el_row1.setAttribute("class","mixerrow");
        let el_row2 = el_mixerstrip.appendChild(document.createElement("div"));
        el_row2.setAttribute("class","mixerrow");
        let el_row3 = el_mixerstrip.appendChild(document.createElement("div"));
        el_row3.setAttribute("class","mixerrow");
        let el_lab = el_row1.appendChild(document.createElement("label"));
        el_lab.setAttribute("for",fader);
        el_lab.setAttribute("class","mixerlabel");
        el_lab.append(val);
        let el_mutebuttondiv = el_row1.appendChild(document.createElement("div"));
        el_mutebuttondiv.setAttribute("class", "mutebuttondiv");
        let el_span = el_mutebuttondiv.appendChild(document.createElement("span"));
        el_span.setAttribute("class", "mutebuttonlabel");
        el_span.appendChild(document.createTextNode("mute "));
        let el_mutebutton = el_mutebuttondiv.appendChild(document.createElement("input"));
        el_mutebutton.setAttribute("class", "mutebutton");
        el_mutebutton.setAttribute("type", "checkbox");
        el_mutebutton.setAttribute("id", muteid);
        //el_mutebutton.onchange = upload_session_gains;
        let el_fader=el_row2.appendChild(document.createElement("input"));
        el_fader.setAttribute("class","fader");
        el_fader.setAttribute("type","range");
        el_fader.setAttribute("min","-30");
        el_fader.setAttribute("max","10");
        el_fader.setAttribute("value",val);
        el_fader.setAttribute("step","0.1");
        el_fader.setAttribute("id",fader);
        el_fader.onchange = upload_session_gains;
        let el_gaintext=el_row2.appendChild(document.createElement("input"));
        el_gaintext.setAttribute("type","number");
        el_gaintext.setAttribute("class","gaintxtfader");
        el_gaintext.setAttribute("min","-30");
        el_gaintext.setAttribute("max","10");
        el_gaintext.setAttribute("step","0.1");
        el_gaintext.setAttribute("id","txt"+fader);
        let el_meter = el_row3.appendChild(document.createElement("meter"));
        el_meter.setAttribute("class","level");
        el_meter.setAttribute("min","0");
        el_meter.setAttribute("max","94");
        el_meter.setAttribute("low","71");
        el_meter.setAttribute("high","84");
        el_meter.setAttribute("optimum","54");
        el_meter.setAttribute("id",levelid);
        let el_metertext = el_row3.appendChild(document.createElement("input"));
        el_metertext.setAttribute("type","text");
        el_metertext.setAttribute("readonly","true");
        el_metertext.setAttribute("class","gaintxtfader");
        el_metertext.setAttribute("id","txt"+levelid);
        //el_mixerstrip.appendChild(el_lab);
        //el_mixerstrip.appendChild(el_mutebuttondiv);
        //el_mixerstrip.appendChild(document.createElement("br"));
        //el_mixerstrip.appendChild(document.createElement("br"));
        //el_mixerstrip.appendChild(el_meter);
        //el_mixerstrip.appendChild(el_metertext);
    }
});
socket.on("updatefader", function(fader,val){
    let fad=document.getElementById(fader);
    if( fad!=null ){
	fad.value=val;
    }
    let fadt=document.getElementById("txt"+fader);
    if( fadt!=null ){
	fadt.value=val.toFixed(1);
    }
});

socket.on("updatemute", function(fader,val){
    let fad=document.getElementById(fader);
    if( fad!=null ){
	      fad.checked=(val == 1);
    }
});

socket.on('updatevar', function(id, val){
    let el=document.getElementById(id);
    if( el )
        el.setAttribute('value',val);
    let el2=document.getElementById(id+'.disp');
    if( el2 )
        el2.setAttribute('value',parseFloat(val.toPrecision(4)));
});

socket.on('oscvarlist', function(parents,varlist){
    let el=document.getElementById('plugpars');
    while (el.firstChild) {el.removeChild(el.firstChild);}
    for( var k=0;k<parents.length;++k){
        const p = parents[k];
        var d;
        var dp = null;
        if( p.parent && (p.parent.length>0) )
            dp = document.getElementById(p.parent);
        if( dp )
            d = dp.appendChild(document.createElement('div'));
        else
            d = el.appendChild(document.createElement('div'));
        d.setAttribute('id',p.id);
        d.setAttribute('class','parblock');
        d.appendChild(document.createTextNode(p.label));
    }
    for( const key in varlist ){
        const v = varlist[key];
        let p=document.getElementById(v.parent);
        if( p ){
            var dplug = p.appendChild(document.createElement('div'));
            dplug.setAttribute('class','parstrip');
            var dpluglab = dplug.appendChild(document.createElement('label'));
            //dpluglab.setAttribute('type','label');
            dpluglab.setAttribute('class','parstriplabel');
            dpluglab.appendChild(document.createTextNode(v.label));
            //dpluglab.setAttribute('value',v.label);
            dpluglab.setAttribute('title',v.comment);
            var inp = dplug.appendChild(document.createElement('input'));
            inp.setAttribute('class','parstripctl');
            inp.setAttribute('id',v.id);
            inp.setAttribute('title',v.comment);
            var inp2 = dplug.appendChild(document.createElement('input'));
            inp2.setAttribute('class','parstripdisp');
            inp2.setAttribute('id',v.id+'.disp');
            inp2.setAttribute('type','number');
            inp2.setAttribute('step','any');
            inp2.setAttribute('title',v.comment);
            var rg = v.range.split(',');
            if( rg.length == 2 ){
                var vmin = parseFloat(rg[0].replace('[','').replace(']',''));
                var vmax = parseFloat(rg[1].replace('[','').replace(']',''));
                const step = (vmax-vmin)/256;
                if( rg[0].startsWith(']') )
                    vmin += step;
                if( rg[1].endsWith('[') )
                    vmax -= step;
                inp.setAttribute('type','range');
                inp.setAttribute('min',vmin);
                inp.setAttribute('max',vmax);
                inp.setAttribute('step',step);
            }else{
                inp.setAttribute('type','number');
                inp.setAttribute('step','any');
            }
            inp.onchange = function(e){
                socket.emit("msg",{"path":v.path,"value":e.target.valueAsNumber});
                socket.emit("msg",{"path":"/uploadpluginsettings","value":null});
                let inp = document.getElementById(v.id+'.disp');
                if( inp )
                    inp.value = parseFloat(e.target.valueAsNumber.toPrecision(4));
            };
            inp2.onchange = function(e){
                socket.emit("msg",{"path":v.path,"value":e.target.valueAsNumber});
                socket.emit("msg",{"path":"/uploadpluginsettings","value":null});
                let inp = document.getElementById(v.id);
                if( inp )
                    inp.value = e.target.valueAsNumber;
            };
        }
    }
});

function upload_session_gains()
{
    socket.emit("msg",{"path":"/uploadsessiongains","value":null});
}

function str_pad_left(string,pad,length) {
    return (new Array(length+1).join(pad)+string).slice(-length);
}

function sec2minsec( t ){
    var minutes = Math.floor(t / 60);
    var seconds = Math.floor(t-60*minutes);
    var sec10 = Math.floor(10*(t-Math.floor(t)));
    return str_pad_left(minutes,'0',2) + ':' + str_pad_left(seconds,'0',2) + '.' + sec10;
}

function recerror( e ){
    let el=document.getElementById("recerr");
    if( el.childNodes.length > 0 )
	el.replaceChild(document.createTextNode(e),el.childNodes[0]);
    else
	el.appendChild(document.createTextNode(e));
}

function objmix_upload_posandgains(){
    socket.emit("msg",{"path":"/uploadobjmix","value":null});
}

socket.on("jackrecerr", function(e){recerror(e)} );

socket.on("jackrectime", function(t){
    let el=document.getElementById("rectime");
    el.setAttribute("value",sec2minsec(t));
    //el.set_attribute("value",t);
});

socket.on("jackrecportlist", function(t){
    let el=document.getElementById("portlist");
    if( el )
        while (el.firstChild) {el.removeChild(el.firstChild);}
});

socket.on('jackrecaddport', function(p){
    var labs = p;
    var classes = 'mixerstrip jackrecsrcport';
    var helps = '';
    if( p.startsWith('n2j_') )
        return;
    if( p.startsWith('system:capture') ){
        classes += ' mixerego';
        helps = 'Hardware input';
    }
    if( p.startsWith('bus.') ){
        classes += ' mixerego';
        helps = 'My input (with effects)';
        labs = labs.replace('bus.','').replace(':out.0','');
    }
    if( p == deviceid + '.metronome:out.0' ){
        return;
    }
    if( p == deviceid + '.metronome:out.1' ){
        classes += ' mixerother';
        helps = 'Metronome';
        labs = 'metronome';
    }
    if( p.startsWith('render.'+deviceid) ){
        classes += ' mixerother';
        helps = 'My headphone output';
        labs = labs.replace('render.'+deviceid+':','');
    }
    labs = labs.replace('.'+deviceid+':out','');
    let el=document.getElementById("portlist");
    if( el ){
        let div=el.appendChild(document.createElement('div'));
        div.setAttribute('class',classes);
        let inp=div.appendChild(document.createElement('input'));
        inp.setAttribute('title',helps);
        inp.setAttribute('type','checkbox');
        inp.setAttribute('value',p);
        inp.setAttribute('id',p);
        inp.setAttribute('class','jackport checkbox');
        let lab=div.appendChild(document.createElement('label'));
        lab.setAttribute('for',p);
        lab.setAttribute('title',helps);
        lab.appendChild(document.createTextNode(labs));
    }
});

socket.on("jackrecfilelist", function(t){
    let el=document.getElementById("filelist");
    if( el )
        while (el.firstChild) {el.removeChild(el.firstChild);}
});

socket.on('jackrecaddfile', function(p){
    let el=document.getElementById("filelist");
    if( !el )
        return;
    let div=el.appendChild(document.createElement('div'));
    let inp=div.appendChild(document.createElement('input'));
    inp.setAttribute('type','checkbox');
    inp.setAttribute('value',p);
    inp.setAttribute('class','filename');
    let lab=div.appendChild(document.createElement('a'));
    lab.setAttribute('href',p);
    lab.appendChild(document.createTextNode(p));
});

socket.on('jackrecstart', function(p){
    let el=document.getElementById("recindicator");
    el.style = 'display: inline-block;';
});

socket.on('jackrecstop', function(p){
    let el=document.getElementById("recindicator");
    el.style = 'display: none;';
});

socket.on('objmixredraw', function(p){
    objmix_draw();
});

let form = document.getElementById("mixer");

if( form )
    form.oninput = handleChange;

function handleChange(e) {
    if( e.target.id.substr(0,3)=="txt" ){
	      socket.emit("msg", { path: e.target.id.substr(3), value: e.target.valueAsNumber } );
	      let fad=document.getElementById(e.target.id.substr(3));
	      if( fad!=null ){
	          fad.value=e.target.valueAsNumber;
	      }
    }else{
        if(e.target.type == "checkbox" ){
            if(e.target.checked)
	              socket.emit("msg", { path: e.target.id, value: 1 } );
            else
	              socket.emit("msg", { path: e.target.id, value: 0 } );
        }else{
	          socket.emit("msg", { path: e.target.id, value: e.target.valueAsNumber } );
	          let fadt=document.getElementById(e.target.id);
	          if( fadt!=null ){
	              fadt.value=e.target.valueAsNumber.toFixed(1);
	          }
        }
    }
}

function jackrec_start() {
    socket.emit("msg", {path: '/jackrec/clear',value: null});
    let el=document.getElementById("portlist");
    if( !el )
        return;
    let ports=el.getElementsByClassName('jackport');
    for( var k=0;k<ports.length;k++){
	if( ports[k].checked ){
	    socket.emit("msg", {path: '/jackrec/addport',value: ports[k].getAttribute('value')});
	}
    }
    recerror('');
    socket.emit("msg", {path: '/jackrec/start', value: null} );
}

function jackrec_delete() {
    let el=document.getElementById("filelist");
    if( !el )
        return;
    let ports=el.getElementsByClassName('filename');
    for( var k=0;k<ports.length;k++){
	if( ports[k].checked ){
	    socket.emit("msg", {path: '/jackrec/rmfile',value: ports[k].getAttribute('value')});
	}
    }
    socket.emit('msg', {path: '/jackrec/listfiles', value: null});
    document.getElementById("selectallfiles").checked = false;
}

function jackrec_stop() {
    recerror('');
    socket.emit("msg", {path: '/jackrec/stop', value: null} );
    socket.emit('msg', {path: '/jackrec/listfiles', value: null});
}

function jackrec_selectallfiles() {
    let ischecked=document.getElementById("selectallfiles").checked;
    let el=document.getElementById("filelist");
    if( !el )
        return;
    let ports=el.getElementsByClassName('filename');
    for( var k=0;k<ports.length;k++){
	ports[k].checked = ischecked;
    }
}

function showtab( name ) {
    names = ['mixer','plugpars','jackrec','objmix'];
    for( var k=0;k<names.length;k++){
        var el = document.getElementById(names[k]);
        var elinp = document.getElementById('tabact'+names[k]);
        if( names[k] == name ){
            el.classList.add("tabshow");
            el.classList.remove("tabhide");
            elinp.classList.add("tabact");
        }else{
            el.classList.remove("tabshow");
            el.classList.add("tabhide");
            elinp.classList.remove("tabact");
        }
    }
    objmix_draw();
}

window.addEventListener('resize', function(event) {
    objmix_draw();
}, true);
