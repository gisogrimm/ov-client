var deviceid = '';
var inchannelpos = [];
var objmix_sel = -1;
var objmix_drag = false;

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
    return 0.22*Math.min(w,h);
}

function pos2scr( pos )
{
    const canvas = document.getElementById("objmixer");
    const scale = objmix_getscale(canvas.width, canvas.height);
    return {x:(0.5*canvas.width-scale*pos[1]),y:0.9*canvas.height-scale*pos[0]};
}

function scr2pos( pos )
{
    const canvas = document.getElementById("objmixer");
    const scale = objmix_getscale(canvas.width, canvas.height);
    return {y:-(pos.x-0.5*canvas.width)/scale,
            x:(-pos.y+0.9*canvas.height)/scale};
}

function on_canvas_click( e )
{
    const canvas = document.getElementById("objmixer");
    var rect = canvas.getBoundingClientRect();
    var ksel = -1;
    for( var k=0; k<inchannelpos.length;k++){
        const vertex = inchannelpos[k];
        var pos = pos2scr([vertex.x,vertex.y,vertex.z]);
        pos.x = pos.x - e.clientX + rect.left;
        pos.y = pos.y - e.clientY + rect.top;
        const d = Math.sqrt(pos.x*pos.x + pos.y*pos.y);
        if( d < 24 )
            ksel = k;
    }
    objmix_sel = ksel;
    if( ksel >= 0 ){
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
        socket.emit("msg",{path:'/'+deviceid+'/ego/'+inchannelpos[objmix_sel].name+'/pos',
                           value:[inchannelpos[objmix_sel].x,inchannelpos[objmix_sel].y,inchannelpos[objmix_sel].z]});
        e.preventDefault();
    }
}

function objmix_draw()
{
    const canvas = document.getElementById("objmixer");
    if( !canvas )
        return;
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
    ctx.arc(p0.x,p0.y,Math.abs(p1.x-p0.x),0,Math.PI,true);
    ctx.stroke();
    ctx.beginPath();
    ctx.arc(p0.x,p0.y,0.25*Math.abs(p1.x-p0.x),0,Math.PI,true);
    ctx.stroke();
    ctx.restore();
    ctx.save();
    ctx.font = "24px sans";
    //canvas.width = canvas.width;
    for( var k=0; k<inchannelpos.length;k++){
        const vertex = inchannelpos[k];
        const pos = pos2scr([vertex.x,vertex.y,vertex.z]);
        const colrgb = HSVtoRGB(k/inchannelpos.length, 0.85, 0.8 );
        ctx.fillStyle = `rgb(${colrgb.r},${colrgb.g},${colrgb.b})`;
        ctx.beginPath();
        ctx.arc(pos.x, pos.y, 20, 0, Math.PI * 2, true); // Outer circle
        ctx.fill();
        ctx.fillText(vertex.name, pos.x+24, pos.y-5);
    }
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
    inchannelpos = [];
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
socket.on("vertexpos", function(name, x, y, z){
    var needclear = false;
    for( var k=0; k<inchannelpos.length;k++){
        if( inchannelpos[k].name == name )
            needclear = true;
    }
    if( needclear )
        inchannelpos = [];
    inchannelpos.push({'name':name,'x':x, 'y':y, 'z': z});
    update_objmix_sounds();
});
socket.on("newfader", function(faderno,val){
    // remove effect bus from mixer:
    if( val.startsWith('bus.') )
        return;
    fader="/touchosc/fader"+faderno;
    levelid="/touchosc/level"+faderno;
    muteid="/touchosc/mute"+faderno;
    let el_div = document.createElement("div");
    let el_mixer=document.getElementById("mixer");
    if( el_mixer ){
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
        el_div.setAttribute("class",classname);
        let el_lab=document.createElement("label");
        el_lab.setAttribute("for",fader);
        el_lab.setAttribute("class","mixerlabel");
        el_lab.append(val);
        let el_mutebuttondiv = document.createElement("div");
        el_mutebuttondiv.setAttribute("class", "mutebuttondiv");
        let el_span = el_mutebuttondiv.appendChild(document.createElement("span"));
        el_span.setAttribute("class", "mutebuttonlabel");
        el_span.appendChild(document.createTextNode("mute "));
        let el_mutebutton = document.createElement("input");
        el_mutebuttondiv.appendChild(el_mutebutton);
        el_mutebutton.setAttribute("class", "mutebutton");
        el_mutebutton.setAttribute("type", "checkbox");
        el_mutebutton.setAttribute("id", muteid);
        //el_mutebutton.onchange = upload_session_gains;
        let el_fader=document.createElement("input");
        el_fader.setAttribute("class","fader");
        el_fader.setAttribute("type","range");
        el_fader.setAttribute("min","-30");
        el_fader.setAttribute("max","10");
        el_fader.setAttribute("value",val);
        el_fader.setAttribute("step","0.1");
        el_fader.setAttribute("id",fader);
        el_fader.onchange = upload_session_gains;
        let el_gaintext=document.createElement("input");
        el_gaintext.setAttribute("type","number");
        el_gaintext.setAttribute("class","gaintxtfader");
        el_gaintext.setAttribute("min","-30");
        el_gaintext.setAttribute("max","10");
        el_gaintext.setAttribute("step","0.1");
        el_gaintext.setAttribute("id","txt"+fader);
        let el_meter=document.createElement("meter");
        el_meter.setAttribute("class","level");
        el_meter.setAttribute("min","0");
        el_meter.setAttribute("max","94");
        el_meter.setAttribute("low","71");
        el_meter.setAttribute("high","84");
        el_meter.setAttribute("optimum","54");
        el_meter.setAttribute("id",levelid);
        let el_metertext=document.createElement("input");
        el_metertext.setAttribute("type","text");
        el_metertext.setAttribute("readonly","true");
        el_metertext.setAttribute("class","gaintxtfader");
        el_metertext.setAttribute("id","txt"+levelid);
        el_mixer.appendChild(el_div);
        el_div.appendChild(el_lab);
        el_div.appendChild(el_mutebuttondiv);
        el_div.appendChild(document.createElement("br"));
        el_div.appendChild(el_fader);
        el_div.appendChild(el_gaintext);
        el_div.appendChild(document.createElement("br"));
        el_div.appendChild(el_meter);
        el_div.appendChild(el_metertext);
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
