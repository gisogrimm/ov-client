var deviceid = '';

socket.on("connect", function() {
    socket.emit("config",{});
});
socket.on('deviceid',function(id){deviceid=id;});
socket.on("scene", function(scene){
    let el=document.getElementById("mixer");
    while (el.firstChild) {el.removeChild(el.firstChild);}
    let elheader=document.createElement("h2");
    elheader.setAttribute("class","scene");
    el.append(scene,elheader);
    let elgainstore=document.createElement("p");
    elgainstore.setAttribute("class","gainstore");
    el.appendChild(elgainstore);
});
socket.on("newfader", function(faderno,val){
    fader="/touchosc/fader"+faderno;
    levelid="/touchosc/level"+faderno;
    let el_div = document.createElement("div");
    let el_mixer=document.getElementById("mixer");
    let classname = "mixerstrip";
    val = val.replace('.'+deviceid,'');
    if( val.startsWith("ego.")||(val == "monitor") )
	classname = classname + " mixerego";
    if( (val == "main") || (val == "reverb") )
	classname = classname + " mixerother";
    el_div.setAttribute("class",classname);
    let el_lab=document.createElement("label");
    el_lab.setAttribute("for",fader);
    el_lab.append(val);
    let el_fader=document.createElement("input");
    el_fader.setAttribute("class","fader");
    el_fader.setAttribute("type","range");
    el_fader.setAttribute("min","-20");
    el_fader.setAttribute("max","10");
    el_fader.setAttribute("value",val);
    el_fader.setAttribute("step","0.1");
    el_fader.setAttribute("id",fader);
    el_fader.onchange = upload_session_gains;
    let el_gaintext=document.createElement("input");
    el_gaintext.setAttribute("type","number");
    el_gaintext.setAttribute("class","gaintxtfader");
    el_gaintext.setAttribute("min","-20");
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
    el_div.appendChild(document.createElement("br"));
    el_div.appendChild(el_fader);
    el_div.appendChild(el_gaintext);
    el_div.appendChild(document.createElement("br"));
    el_div.appendChild(el_meter);
    el_div.appendChild(el_metertext);
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

socket.on("jackrecerr", function(e){recerror(e)} );

socket.on("jackrectime", function(t){
    let el=document.getElementById("rectime");
    el.setAttribute("value",sec2minsec(t));
    //el.set_attribute("value",t);
});

socket.on("jackrecportlist", function(t){
    let el=document.getElementById("portlist");
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
});

socket.on("jackrecfilelist", function(t){
    let el=document.getElementById("filelist");
    while (el.firstChild) {el.removeChild(el.firstChild);}
});

socket.on('jackrecaddfile', function(p){
    let el=document.getElementById("filelist");
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

let form = document.getElementById("mixer");

form.oninput = handleChange;

function handleChange(e) {
    if( e.target.id.substr(0,3)=="txt" ){
	socket.emit("msg", { path: e.target.id.substr(3), value: e.target.valueAsNumber } );
	let fad=document.getElementById(e.target.id.substr(3));
	if( fad!=null ){
	    fad.value=e.target.valueAsNumber;
	}
    }else{
	socket.emit("msg", { path: e.target.id, value: e.target.valueAsNumber } );
	let fadt=document.getElementById(e.target.id);
	if( fadt!=null ){
	    fadt.value=e.target.valueAsNumber.toFixed(1);
	}
    }
}

function jackrec_start() {
    socket.emit("msg", {path: '/jackrec/clear',value: null});
    let el=document.getElementById("portlist");
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
    let ports=el.getElementsByClassName('filename');
    for( var k=0;k<ports.length;k++){
	ports[k].checked = ischecked;
    }
}
