urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// tests/strFold.ur:9:11-9:23
function _u2(v_0){let v_1 = v_0, v_2 = (0), v_3 = 0, v_4 = true; while (v_3 < v_1.length) {let v_5 = v_1.codePointAt(v_3); v_3 += v_5 > 0xFFFF ? 2 : 1; let v_6 = String.fromCodePoint(v_5), v_7 = v_2; let v_8; let v_9; let v_10 = isDigit(v_6); if (v_10 == true) {v_9 = (v_7 + (1));} else if (v_10 == false) {v_9 = v_7;} else { er("Match failure in compiled Ur code"); } v_8 = {_1:v_9,_2:true}; v_2 = v_8._1; if (!v_8._2) {v_4 = false; break;}} return {_1:v_2,_2:v_4}._1;}
urfuncs[2] = {c:"c",v:_u2};
// tests/strFold.ur:12:8-12:20
function _u3(v_0){let v_1 = v_0, v_2 = {_At:(0),_Sum:(0)}, v_3 = 0, v_4 = true; while (v_3 < v_1.length) {let v_5 = v_1.codePointAt(v_3); v_3 += v_5 > 0xFFFF ? 2 : 1; let v_6 = String.fromCodePoint(v_5), v_7 = v_2; let v_8; let v_9 = isDigit(v_6); if (v_9 == true) {v_8 = {_1:{_At:(v_7._At + (1)),_Sum:((v_7._Sum + ord(v_6)) - (48))},_2:true};} else if (v_9 == false) {v_8 = {_1:v_7,_2:false};} else { er("Match failure in compiled Ur code"); } v_2 = v_8._1; if (!v_8._2) {v_4 = false; break;}} let v_10 = {_1:v_2,_2:v_4}; if (true) {let v_11 = v_10._1; let v_12 = v_10._2; if (v_12 == true) {return v_11._Sum;} else if (v_12 == false) {return (-v_11._At);} else { er("Match failure in compiled Ur code"); }} else { er("Match failure in compiled Ur code"); }}
urfuncs[3] = {c:"c",v:_u3};
// tests/strFold.ur:22:10-22:22
function _u4(v_0){let v_1 = v_0, v_2 = {_Letters:(0),_Others:(0)}, v_3 = 0, v_4 = true; while (v_3 < v_1.length) {let v_5 = v_1.codePointAt(v_3); v_3 += v_5 > 0xFFFF ? 2 : 1; let v_6 = String.fromCodePoint(v_5), v_7 = v_2; let v_8; let v_9; let v_10 = isAlpha(v_6); if (v_10 == true) {v_9 = {_Letters:(v_7._Letters + (1)),_Others:v_7._Others};} else if (v_10 == false) {v_9 = {_Letters:v_7._Letters,_Others:(v_7._Others + (1))};} else { er("Match failure in compiled Ur code"); } v_8 = {_1:v_9,_2:true}; v_2 = v_8._1; if (!v_8._2) {v_4 = false; break;}} return {_1:v_2,_2:v_4}._1;}
urfuncs[4] = {c:"c",v:_u4};
// tests/strFold.ur:31:9-31:21
function _u5(v_0){let v_1 = v_0, v_2 = null, v_3 = 0, v_4 = true; while (v_3 < v_1.length) {let v_5 = v_1.codePointAt(v_3); v_3 += v_5 > 0xFFFF ? 2 : 1; let v_6 = String.fromCodePoint(v_5), v_7 = v_2; let v_8; v_8 = {_1:v_6,_2:(!(v_6 == "!"))}; v_2 = v_8._1; if (!v_8._2) {v_4 = false; break;}} return {_1:v_2,_2:v_4};}
urfuncs[5] = {c:"c",v:_u5};
// tests/strFold.ur:28:9-28:20
function _u7(a_0,a_1){while (true) {let v_0 = a_0, v_1 = a_1; let v_2 = (v_0 == "("); if (v_2 == true) {a_0 = ")"; a_1 = (v_1 + (1)); continue;} else if (v_2 == false) {return v_1;} else { er("Match failure in compiled Ur code"); }}}
function _c7(v_0){return function(v_1){return _u7(v_0,v_1);}}
urfuncs[7] = {c:"c",v:_c7};
// tests/strFold.ur:29:12-29:24
function _u6(v_0){let v_1 = v_0, v_2 = (0), v_3 = 0, v_4 = true; while (v_3 < v_1.length) {let v_5 = v_1.codePointAt(v_3); v_3 += v_5 > 0xFFFF ? 2 : 1; let v_6 = String.fromCodePoint(v_5), v_7 = v_2; let v_8; v_8 = {_1:_u7(v_6,v_7),_2:true}; v_2 = v_8._1; if (!v_8._2) {v_4 = false; break;}} return {_1:v_2,_2:v_4}._1;}
urfuncs[6] = {c:"c",v:_u6};
// tests/strFold.ur:45:19-46:165
function _u1(v_0,v_1){return sb(ss(v_0),function(v_2){let v_35 = ts(_u2(v_2)); let v_3; let v_4 = _u3(v_2); if (v_4 == null) {v_3 = "";} else if (v_4 != null) {let v_5 = v_4; v_3 = ts(v_5);} else { er("Match failure in compiled Ur code"); } let v_34 = eh(v_3); let v_33 = ts(_u4(v_2)._Others); let v_6; let v_7 = _u5(v_2)._1; if (v_7 == null) {v_6 = "";} else if (v_7 != null) {let v_8 = v_7; v_6 = ts(v_8);} else { er("Match failure in compiled Ur code"); } let v_32 = eh(v_6); let v_31 = bs(_u5(v_2)._2); let v_30 = ts(_u6(v_2)); let v_9 = v_2, v_10 = (0), v_11 = 0, v_12 = true; while (v_11 < v_9.length) {let v_13 = v_9.codePointAt(v_11); v_11 += v_13 > 0xFFFF ? 2 : 1; let v_14 = String.fromCodePoint(v_13), v_15 = v_10; let v_16; let v_17; let v_18 = isDigit(v_14); if (v_18 == true) {v_17 = (v_15 + slen(v_2));} else if (v_18 == false) {v_17 = v_15;} else { er("Match failure in compiled Ur code"); } v_16 = {_1:v_17,_2:true}; v_10 = v_16._1; if (!v_16._2) {v_12 = false; break;}} let v_29 = ts({_1:v_10,_2:v_12}._1); let v_20 = v_2, v_21 = function(v_19){return v_19;}, v_22 = 0, v_23 = true; while (v_22 < v_20.length) {let v_24 = v_20.codePointAt(v_22); v_22 += v_24 > 0xFFFF ? 2 : 1; let v_25 = String.fromCodePoint(v_24), v_26 = v_21; let v_27; v_27 = {_1:function(v_28){return ((ap(v_26,v_28) + ord(v_25)) - (48));},_2:true}; v_21 = v_27._1; if (!v_27._2) {v_23 = false; break;}} return sr(cat(v_35,cat(" ",cat(v_34,cat(" ",cat(v_33,cat(" ",cat(v_32,cat(" ",cat(v_31,cat(" ",cat(v_30,cat(" ",cat(v_29,cat(" ",ts(ap({_1:v_21,_2:v_23}._1,(0))))))))))))))))));});}
function _c1(v_0){return function(v_1){return _u1(v_0,v_1);}}
urfuncs[1] = {c:"c",v:_c1};

time_format = "%c";
