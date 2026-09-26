urlRules = cons({allow:true,prefix:true,pattern:"#"},null);

// tests/js/counter/counter.ur:4:6-4:77
function _u1(v_0,v_1){let v_2 = uw_mouseEvent(); let v_3 = sg(v_0); return sv(v_0,(v_3 + (1)));}
function _c1(v_0){return function(v_1){return _u1(v_0,v_1);}}
urfuncs[1] = {c:"c",v:_c1};
// tests/js/counter/counter.ur:5:19-5:64
function _u2(v_0,v_1){return sb(ss(v_0),function(v_2){return sr(cat("Count: ",ts(v_2)));});}
function _c2(v_0){return function(v_1){return _u2(v_0,v_1);}}
urfuncs[2] = {c:"c",v:_c2};

time_format = "%c";
