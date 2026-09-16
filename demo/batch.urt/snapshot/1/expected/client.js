urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// demo/batch.ur:28:69-29:3
urfuncs[1] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:uw_mouseEvent,a:null},e2:{c:"f",f:rc,a:cons({c:"c",v:"/"},cons({c:"f",f:cat,a:cons({c:"c",v:"Batch/del/"},cons({c:"f",f:ts,a:cons({c:"v",n:2},null)},null))},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return (i++,null)}},cons({c:"K"},cons({c:"c",v:false},null)))))}}}}'};
// demo/batch.ur:23:8-34:4
function _u3(v_0,v_1){if (v_1 == null) {return "";} else if (v_1 != null) {let v_2 = v_1._1._1; let v_3 = v_1._1._2; let v_4 = v_1._2; let v_7 = ts(v_2); let v_6 = eh(v_3); let v_5; if (v_0 == true) {v_5 = cat("\074td>\074button onclick='uw_event=event;exec(",cat(cs({c:"wc",env:cons(v_4,cons(v_3,cons(v_2,cons(v_1,cons(v_0,null))))),body:{c:"a",f:{c:"a",f:{c:"n",n:1},x:{c:"v",n:2}},x:{c:"c",v:null}}}),")'>Delete\074/button>\n\074/td>"));} else if (v_0 == false) {v_5 = "";} else { er("Match failure in compiled Ur code"); } return cat("\n\074tr>\074td>",cat(v_7,cat("\074/td> \074td>",cat(v_6,cat("\074/td> ",cat(v_5,cat(" \074/tr>\n",cat(_u3(v_0,v_4),"\n"))))))));} else { er("Match failure in compiled Ur code"); }}
function _c3(v_0){return function(v_1){return _u3(v_0,v_1);}}
urfuncs[3] = {c:"c",v:_c3};
// demo/batch.ur:35:26-38:26
function _u2(v_0,v_1,v_2){return sb(ss(v_1),function(v_3){return sr(cat("\074table>\n\074tr> \074th>Id\074/th> \074th>A\074/th> \074/tr>\n",cat(_u3(v_0,v_3),"\n\074/table>")));});}
function _c2(v_0){return function(v_1){return function(v_2){return _u2(v_0,v_1,v_2);}}}
urfuncs[2] = {c:"c",v:_c2};
function _n1697(t,i){var x=t[i++];var r=x=="Cons"?{_1:{_1:parseInt(t[i++]),_2:uu(t[i++])},_2:(tmp=_n1697(t,i),i=tmp._1,tmp._2)}:x=="Nil"?null:pf("demo/batch.ur:67:50-67:84");return {_1:i,_2:r}}

// demo/batch.ur:67:14-67:91
urfuncs[4] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:uw_mouseEvent,a:null},e2:{c:"=",e1:{c:"f",f:rc,a:cons({c:"c",v:"/"},cons({c:"c",v:"Batch/allRows/_"},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return (tmp=_n1697(t,i),i=tmp._1,tmp._2)}},cons({c:"K"},cons({c:"c",v:false},null)))))},e2:{c:"f",f:sv,a:cons({c:"v",n:3},cons({c:"v",n:0},null))}}}}}'};
// demo/batch.ur:75:31-75:82
function _u5(v_0,v_1,v_2,v_3){let v_4 = uw_mouseEvent(); let v_5 = sg(v_1); let v_6 = sg(v_2); let v_7 = sg(v_0); return sv(v_0,{_1:{_1:pi(v_5),_2:v_6},_2:v_7});}
function _c5(v_0){return function(v_1){return function(v_2){return function(v_3){return _u5(v_0,v_1,v_2,v_3);}}}}
urfuncs[5] = {c:"c",v:_c5};
// demo/batch.ur:16:16-16:19
function _u7(a_0){let acc_ = ""; while (true) {let v_0 = a_0; if (v_0 == null) {return cat(acc_,"Nil");} else if (v_0 != null) {let v_1 = v_0; acc_ = cat(acc_,"Cons/"); acc_ = cat(acc_,ts(v_1._1._1)); acc_ = cat(acc_,"/"); acc_ = cat(acc_,uf(v_1._1._2)); acc_ = cat(acc_,"/"); a_0 = v_1._2; continue;} else { er("Match failure in compiled Ur code"); }}}
urfuncs[7] = {c:"c",v:_u7};
// demo/batch.ur:80:14-81:3
urfuncs[6] = {c:"t",f:'{c:"l",b:{c:"l",b:{c:"=",e1:{c:"f",f:uw_mouseEvent,a:null},e2:{c:"=",e1:{c:"f",f:rc,a:cons({c:"c",v:"/"},cons({c:"f",f:cat,a:cons({c:"c",v:"Batch/doBatch/"},cons({c:"a",f:{c:"n",n:7},x:{c:"f",f:sg,a:cons({c:"v",n:2},null)}},null))},cons({c:"c",v:function(s){var t=s.split("/");var i=0;return (i++,null)}},cons({c:"K"},cons({c:"c",v:false},null)))))},e2:{c:"f",f:sv,a:cons({c:"v",n:3},cons({c:"c",v:null},null))}}}}}'};

time_format = "%c";
