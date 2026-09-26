urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// demo/listEdit.ur:6:4-9:2
function _u1(v_0,v_1){return sb(ss(v_0),function(v_2){return sr(eh(v_2));});}
function _c1(v_0){return function(v_1){return _u1(v_0,v_1);}}
urfuncs[1] = {c:"c",v:_c1};
// demo/listEdit.ur:18:12-19:3
function _u2(v_0,v_1,v_2){let v_3 = uw_mouseEvent(); let v_4 = sg(v_1); return sv(v_0,v_4);}
function _c2(v_0){return function(v_1){return function(v_2){return _u2(v_0,v_1,v_2);}}}
urfuncs[2] = {c:"c",v:_c2};
// demo/listEdit.ur:13:10-13:12
function _u4(v_0){if (v_0 == null) {return sr("");} else if (v_0 != null) {let v_1 = v_0._Data; let v_2 = v_0._NewData; let v_3 = v_0._Tail; return sr(cat("\n\074script type=\"text/javascript\">dyn(\"span\", execD(",cat(cs({c:"wc",env:cons(v_3,cons(v_2,cons(v_1,cons(v_0,null)))),body:{c:"a",f:{c:"a",f:{c:"n",n:1},x:{c:"v",n:2}},x:{c:"c",v:null}}}),cat("))\074/script>\n\074button onclick='uw_event=event;exec(",cat(cs({c:"wc",env:cons(v_3,cons(v_2,cons(v_1,cons(v_0,null)))),body:{c:"a",f:{c:"a",f:{c:"a",f:{c:"n",n:2},x:{c:"v",n:2}},x:{c:"v",n:1}},x:{c:"c",v:null}}}),cat(")'>Change to:\074/button>\n \074script type=\"text/javascript\">var d=inp(exec(",cat(cs({c:"wc",env:cons(v_3,cons(v_2,cons(v_1,cons(v_0,null)))),body:{c:"v",n:1}}),cat("));\074/script>\074br />\n\074script type=\"text/javascript\">dyn(\"span\", execD(",cat(cs({c:"wc",env:cons(v_3,cons(v_2,cons(v_1,cons(v_0,null)))),body:{c:"a",f:{c:"n",n:3},x:{c:"v",n:0}}}),"))\074/script>\n")))))))));} else { er("Match failure in compiled Ur code"); }}
urfuncs[4] = {c:"c",v:_u4};
// demo/listEdit.ur:9:9-9:12
function _u3(v_0){return sb(ss(v_0),function(v_1){return _u4(v_1);});}
urfuncs[3] = {c:"c",v:_u3};
// demo/listEdit.ur:44:40-44:86
function _u5(v_0,v_1,v_2){let v_3 = uw_mouseEvent(); let v_4 = sg(v_1); let v_5 = sc(v_4); let v_6 = sc(""); let v_7 = sg(v_0); let v_8 = sc(null); let v_9 = sv(v_7,{_Data:v_5,_NewData:v_6,_Tail:v_8}); return sv(v_0,v_8);}
function _c5(v_0){return function(v_1){return function(v_2){return _u5(v_0,v_1,v_2);}}}
urfuncs[5] = {c:"c",v:_c5};

time_format = "%c";
