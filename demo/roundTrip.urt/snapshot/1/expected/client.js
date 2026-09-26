urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// demo/buffer.ur:10:12-10:17
function _u2(v_0){if (v_0 == null) {return "";} else if (v_0 != null) {let v_1 = v_0._1; let v_2 = v_0._2; return cat(eh(v_1),cat("\074br />\074script type=\"text/javascript\">dyn(\"span\", execD(",cat(cs({c:"wc",env:cons(v_2,cons(v_1,cons(v_0,null))),body:{c:"a",f:{c:"n",n:1},x:{c:"v",n:0}}}),"))\074/script>")));} else { er("Match failure in compiled Ur code"); }}
urfuncs[2] = {c:"c",v:_u2};
// demo/buffer.ur:15:12-15:18
function _u1(v_0){return sb(ss(v_0),function(v_1){return sr(_u2(v_1));});}
urfuncs[1] = {c:"c",v:_u1};
// demo/roundTrip.ur:9:0-30:7
urfuncs[4] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:rv,a:cons({c:"v",n:3},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return {_1:uu(t[i++]),_2:parseInt(t[i++]),_3:parseFloat(t[i++])}}},cons({c:"K"},null)))},e2:{c:"=",e1:{c:"f",f:sg,a:cons({c:".",r:{c:"v",n:3},f:"Tail"},null)},e2:{c:"=",e1:{c:"f",f:sc,a:cons({c:"c",v:null},null)},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:"v",n:1},cons({c:"r",l:cons({n:"1",v:{c:"f",f:cat,a:cons({c:"c",v:"("},cons({c:"f",f:cat,a:cons({c:".",r:{c:"v",n:2},f:"1"},cons({c:"f",f:cat,a:cons({c:"c",v:", "},cons({c:"f",f:cat,a:cons({c:"f",f:ts,a:cons({c:".",r:{c:"v",n:2},f:"2"},null)},cons({c:"f",f:cat,a:cons({c:"c",v:", "},cons({c:"f",f:cat,a:cons({c:"a",f:{c:"c",v:ts},x:{c:".",r:{c:"v",n:2},f:"3"}},cons({c:"c",v:")"},null))},null))},null))},null))},null))},null))}},cons({n:"2",v:{c:"v",n:0}},null))},null))},e2:{c:"=",e1:{c:"f",f:sv,a:cons({c:".",r:{c:"v",n:6},f:"Tail"},cons({c:"v",n:1},null))},e2:{c:"a",f:{c:"a",f:{c:"a",f:{c:"a",f:{c:"n",n:4},x:{c:"v",n:8}},x:{c:"v",n:7}},x:{c:"c",v:null}},x:{c:"c",v:null}}}}}}}}}}}'};
// demo/roundTrip.ur:9:0-30:7
urfuncs[5] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:sl,a:cons({c:"c",v:2000},cons({c:"K"},null))},e2:{c:"=",e1:{c:"f",f:rc,a:cons({c:"c",v:"/"},cons({c:"f",f:cat,a:cons({c:"c",v:"RoundTrip/writeBack/"},cons({c:"f",f:cat,a:cons({c:"f",f:uf,a:cons({c:"v",n:4},null)},cons({c:"f",f:cat,a:cons({c:"c",v:"/"},cons({c:"f",f:cat,a:cons({c:"f",f:ts,a:cons({c:"v",n:3},null)},cons({c:"f",f:cat,a:cons({c:"c",v:"/"},cons({c:"f",f:ts,a:cons({c:"v",n:2},null)},null))},null))},null))},null))},null))},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return (i++,null)}},cons({c:"K"},cons({c:"c",v:false},null)))))},e2:{c:"a",f:{c:"a",f:{c:"a",f:{c:"a",f:{c:"n",n:5},x:{c:"f",f:cat,a:cons({c:"v",n:5},cons({c:"c",v:"!"},null))}},x:{c:"f",f:plus,a:cons({c:"v",n:4},cons({c:"c",v:1},null))}},x:{c:"f",f:plus,a:cons({c:"v",n:3},cons({c:"c",v:1.23},null))}},x:{c:"c",v:null}}}}}}}}'};
// demo/roundTrip.ur:27:24-28:3
urfuncs[3] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:sp,a:cons({c:"a",f:{c:"a",f:{c:"a",f:{c:"n",n:4},x:{c:"v",n:2}},x:{c:"v",n:1}},x:{c:"c",v:null}},null)},e2:{c:"a",f:{c:"a",f:{c:"a",f:{c:"a",f:{c:"n",n:5},x:{c:"c",v:""}},x:{c:"c",v:0}},x:{c:"c",v:0}},x:{c:"c",v:null}}}}}}'};
// demo/buffer.ur:19:15-19:29
function _u6(v_0,v_1){return _u1(v_0._Head);}
function _c6(v_0){return function(v_1){return _u6(v_0,v_1);}}
urfuncs[6] = {c:"c",v:_c6};

time_format = "%c";
