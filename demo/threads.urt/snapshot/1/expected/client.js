urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// demo/buffer.ur:10:12-10:17
function _u2(v_0){if (v_0 == null) {return "";} else if (v_0 != null) {let v_1 = v_0._1; let v_2 = v_0._2; return cat(eh(v_1),cat("\074br />\074script type=\"text/javascript\">dyn(\"span\", execD(",cat(cs({c:"wc",env:cons(v_2,cons(v_1,cons(v_0,null))),body:{c:"a",f:{c:"n",n:1},x:{c:"v",n:0}}}),"))\074/script>")));} else { er("Match failure in compiled Ur code"); }}
urfuncs[2] = {c:"c",v:_u2};
// demo/buffer.ur:15:12-15:18
function _u1(v_0){return sb(ss(v_0),function(v_1){return sr(_u2(v_1));});}
urfuncs[1] = {c:"c",v:_u1};
// demo/threads.ur:1:~4-17:7
urfuncs[5] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"l",b:{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:sg,a:cons({c:".",r:{c:"v",n:4},f:"Tail"},null)},e2:{c:"=",e1:{c:"f",f:sc,a:cons({c:"c",v:null},null)},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:"v",n:1},cons({c:"r",l:cons({n:"1",v:{c:"f",f:cat,a:cons({c:"v",n:5},cons({c:"f",f:cat,a:cons({c:"c",v:": Message #"},cons({c:"f",f:ts,a:cons({c:"v",n:3},null)},null))},null))}},cons({n:"2",v:{c:"v",n:0}},null))},null))},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:".",r:{c:"v",n:7},f:"Tail"},cons({c:"v",n:1},null))},e2:{c:"=",e1:{c:"f",f:sl,a:cons({c:"v",n:6},cons({c:"K"},null))},e2:{c:"a",f:{c:"a",f:{c:"a",f:{c:"a",f:{c:"a",f:{c:"n",n:5},x:{c:"v",n:9}},x:{c:"v",n:8}},x:{c:"v",n:7}},x:{c:"f",f:plus,a:cons({c:"v",n:6},cons({c:"c",v:1},null))}},x:{c:"c",v:null}}}}}}}}}}}}'};
// demo/threads.ur:4:8-12:15
function _u4(v_0,v_1,v_2){return ap(ap(ap(nf(5),v_0),v_1),v_2);}
function _c4(v_0){return function(v_1){return function(v_2){return _u4(v_0,v_1,v_2);}}}
urfuncs[4] = {c:"c",v:_c4};
// demo/threads.ur:14:24-15:3
function _u3(v_0,v_1){let v_2 = sp(ap(_u4(v_0,"A",(5000)),(0))); return sp(ap(_u4(v_0,"B",(3000)),(100)));}
function _c3(v_0){return function(v_1){return _u3(v_0,v_1);}}
urfuncs[3] = {c:"c",v:_c3};
// demo/buffer.ur:19:15-19:29
function _u6(v_0,v_1){return _u1(v_0._Head);}
function _c6(v_0){return function(v_1){return _u6(v_0,v_1);}}
urfuncs[6] = {c:"c",v:_c6};

time_format = "%c";
