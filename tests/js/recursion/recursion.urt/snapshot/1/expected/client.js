urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// tests/js/recursion/recursion.ur:1:10-1:19
function _u2(v_0){let v_1 = (v_0 <= (0)); if (v_1 == true) {return (0);} else if (v_1 == false) {return (v_0 + _u2((v_0 - (1))));} else { er("Match failure in compiled Ur code"); }}
urfuncs[2] = {c:"c",v:_u2};
// tests/js/recursion/recursion.ur:6:6-6:79
function _u1(v_0,v_1){let v_2 = uw_mouseEvent(); let v_3 = _u2((sg(v_0) + (3))); return sv(v_0,v_3);}
function _c1(v_0){return function(v_1){return _u1(v_0,v_1);}}
urfuncs[1] = {c:"c",v:_c1};
// tests/js/recursion/recursion.ur:7:19-7:57
function _u3(v_0,v_1){return sb(ss(v_0),function(v_2){return sr(ts(v_2));});}
function _c3(v_0){return function(v_1){return _u3(v_0,v_1);}}
urfuncs[3] = {c:"c",v:_c3};

time_format = "%c";
