urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// tests/js/datatype/datatype.ur:48:10-49:3
urfuncs[1] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:uw_mouseEvent,a:null},e2:{c:"f",f:sv,a:cons({c:"v",n:2},cons({c:"1",n:3,v:{c:"c",v:2}},null))}}}}'};
function _n1(t,i){var x=t[i++];var r=x=="Box"?{n:4,v:{_H:parseInt(t[i++]),_W:parseInt(t[i++])}}:x=="Circle"?{n:3,v:parseInt(t[i++])}:x=="Dot"?2:pf("tests/js/datatype/datatype.ur:51:45-51:47");return {_1:i,_2:r}}

// tests/js/datatype/datatype.ur:49:10-54:3
urfuncs[2] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:uw_mouseEvent,a:null},e2:{c:"m",e:{c:"f",f:rc,a:cons({c:"c",v:"/"},cons({c:"c",v:"Datatype/shapes/_"},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return uul(function(){return t[i++];},function(){return (tmp=_n1(t,i),i=tmp._1,tmp._2)})}},cons({c:"K"},cons({c:"c",v:false},null)))))},p:cons({p:{c:"s",n:false,p:{c:"r",l:cons({n:"1",p:{/*hoho*/c:"v"}},cons({n:"2",p:{/*hoho*/c:"v"}},null))}},b:{c:"f",f:sv,a:cons({c:"v",n:4},cons({c:"v",n:1},null))}},cons({p:{c:"c",v:null},b:{c:"f",f:sv,a:cons({c:"v",n:2},cons({c:"c",v:2},null))}},null))}}}}'};
function _n2(t,i){var x=t[i++];var r=x=="Blue"?8:x=="Green"?7:x=="Red"?6:pf("tests/js/datatype/datatype.ur:55:39-56:81");return {_1:i,_2:r}}

function _n3(t,i){var x=t[i++];var r=x=="Just"?parseInt(t[i++]):x=="Nothing"?null:pf("tests/js/datatype/datatype.ur:55:39-56:81");return {_1:i,_2:r}}

function _n4(t,i){var x=t[i++];var r=x=="Node"?{_L:(tmp=_n4(t,i),i=tmp._1,tmp._2),_R:(tmp=_n4(t,i),i=tmp._1,tmp._2),_V:parseInt(t[i++])}:x=="Leaf"?null:pf("tests/js/datatype/datatype.ur:55:39-56:81");return {_1:i,_2:r}}

function _n6(t,i){var x=t[i++];var r=x=="O"?{n:19,v:(tmp=_n5(t,i),i=tmp._1,tmp._2)}:pf("tests/js/datatype/datatype.ur:55:39-56:81");return {_1:i,_2:r}}

function _n5(t,i){var x=t[i++];var r=x=="S"?(tmp=_n6(t,i),i=tmp._1,tmp._2):x=="Z"?null:pf("tests/js/datatype/datatype.ur:55:39-56:81");return {_1:i,_2:r}}

// tests/js/datatype/datatype.ur:54:10-57:3
urfuncs[3] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"l",b:{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:uw_mouseEvent,a:null},e2:{c:"=",e1:{c:"f",f:rc,a:cons({c:"c",v:"/"},cons({c:"c",v:"Datatype/rest/_"},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return {_1:(tmp=_n2(t,i),i=tmp._1,tmp._2),_2:(tmp=_n3(t,i),i=tmp._1,tmp._2),_3:(tmp=_n4(t,i),i=tmp._1,tmp._2),_4:(tmp=_n5(t,i),i=tmp._1,tmp._2)}}},cons({c:"K"},cons({c:"c",v:false},null)))))},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:"v",n:6},cons({c:".",r:{c:"v",n:0},f:"1"},null))},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:"v",n:6},cons({c:".",r:{c:"v",n:1},f:"2"},null))},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:"v",n:6},cons({c:".",r:{c:"v",n:2},f:"3"},null))},e2:{c:"f",f:sv,a:cons({c:"v",n:6},cons({c:".",r:{c:"v",n:3},f:"4"},null))}}}}}}}}}}}'};
// tests/js/datatype/datatype.ur:57:19-57:72
urfuncs[4] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"f",f:sb,a:cons({c:"f",f:ss,a:cons({c:"v",n:1},null)},cons({c:"l",b:{c:"f",f:sr,a:cons({c:"f",f:cat,a:cons({c:"c",v:"Area: "},cons({c:"f",f:ts,a:cons({c:"m",e:{c:"v",n:0},p:cons({p:{c:"c",v:2},b:{c:"c",v:0}},cons({p:{c:"1",n:3,p:{/*hoho*/c:"v"}},b:{c:"f",f:times,a:cons({c:"c",v:3},cons({c:"f",f:times,a:cons({c:"v",n:0},cons({c:"v",n:0},null))},null))}},cons({p:{c:"1",n:4,p:{/*hoho*/c:"v"}},b:{c:"f",f:times,a:cons({c:".",r:{c:"v",n:0},f:"W"},cons({c:".",r:{c:"v",n:0},f:"H"},null))}},null)))},null)},null))},null)}},null))}}}'};
// tests/js/datatype/datatype.ur:58:19-58:118
urfuncs[5] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"f",f:sb,a:cons({c:"f",f:ss,a:cons({c:"v",n:1},null)},cons({c:"l",b:{c:"f",f:sr,a:cons({c:"f",f:eh,a:cons({c:"m",e:{c:"v",n:0},p:cons({p:{c:"c",v:6},b:{c:"c",v:"red"}},cons({p:{c:"c",v:7},b:{c:"c",v:"green"}},cons({p:{c:"c",v:8},b:{c:"c",v:"blue"}},null)))},null)},null)}},null))}}}'};
// tests/js/datatype/datatype.ur:59:19-59:96
urfuncs[6] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"f",f:sb,a:cons({c:"f",f:ss,a:cons({c:"v",n:1},null)},cons({c:"l",b:{c:"f",f:sr,a:cons({c:"f",f:ts,a:cons({c:"m",e:{c:"v",n:0},p:cons({p:{c:"c",v:null},b:{c:"c",v:0}},cons({p:{c:"s",n:false,p:{/*hoho*/c:"v"}},b:{c:"v",n:0}},null))},null)},null)}},null))}}}'};
// tests/js/datatype/datatype.ur:25:8-25:9
urfuncs[8] = {c:"t",f:'{c:"l",b:{c:"m",e:{c:"v",n:0},p:cons({p:{c:"c",v:null},b:{c:"c",v:0}},cons({p:{c:"s",n:false,p:{/*hoho*/c:"v"}},b:{c:"f",f:plus,a:cons({c:"f",f:plus,a:cons({c:"a",f:{c:"n",n:8},x:{c:".",r:{c:"v",n:0},f:"L"}},cons({c:".",r:{c:"v",n:0},f:"V"},null))},cons({c:"a",f:{c:"n",n:8},x:{c:".",r:{c:"v",n:0},f:"R"}},null))}},null))}}'};
// tests/js/datatype/datatype.ur:60:19-60:65
urfuncs[7] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"f",f:sb,a:cons({c:"f",f:ss,a:cons({c:"v",n:1},null)},cons({c:"l",b:{c:"f",f:sr,a:cons({c:"f",f:ts,a:cons({c:"a",f:{c:"n",n:8},x:{c:"v",n:0}},null)},null)}},null))}}}'};
// tests/js/datatype/datatype.ur:31:7-31:8
urfuncs[11] = {c:"t",f:'{c:"l",b:{c:"m",e:{c:"v",n:0},p:cons({p:{c:"1",n:19,p:{/*hoho*/c:"v"}},b:{c:"f",f:plus,a:cons({c:"c",v:1},cons({c:"a",f:{c:"n",n:10},x:{c:"v",n:0}},null))}},null)}}'};
// tests/js/datatype/datatype.ur:30:7-30:8
urfuncs[10] = {c:"t",f:'{c:"l",b:{c:"m",e:{c:"v",n:0},p:cons({p:{c:"c",v:null},b:{c:"c",v:0}},cons({p:{c:"s",n:false,p:{/*hoho*/c:"v"}},b:{c:"f",f:plus,a:cons({c:"c",v:1},cons({c:"a",f:{c:"n",n:11},x:{c:"v",n:0}},null))}},null))}}'};
// tests/js/datatype/datatype.ur:61:19-61:64
urfuncs[9] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"f",f:sb,a:cons({c:"f",f:ss,a:cons({c:"v",n:1},null)},cons({c:"l",b:{c:"f",f:sr,a:cons({c:"f",f:ts,a:cons({c:"a",f:{c:"n",n:10},x:{c:"v",n:0}},null)},null)}},null))}}}'};

time_format = "%c";
