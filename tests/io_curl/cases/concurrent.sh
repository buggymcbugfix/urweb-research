# Two io RPCs wait on the provider at once while a third thread serves a
# page: the wait holds no lock a page needs.  Then both are answered.  The
# log is not shown: whether one claim finds the database busy and is run
# again is a matter of timing.
seed "INSERT INTO uw_Curl_io_orders VALUES (4, 70, 'slow', 'new', '');" \
     "INSERT INTO uw_Curl_io_orders VALUES (5, 80, 'slow', 'new', '');"
start -t 3
curl -s -m 30 -X POST -H 'Content-type: text/plain' -H "UrWeb-Sig: $sig" "http://127.0.0.1:$port/Curl_io/pay/4" | tr -d '\r' > "$out/concurrent.4" &
p4=$!
curl -s -m 30 -X POST -H 'Content-type: text/plain' -H "UrWeb-Sig: $sig" "http://127.0.0.1:$port/Curl_io/pay/5" | tr -d '\r' > "$out/concurrent.5" &
p5=$!
sleep 0.3
if curl -s -m 0.5 "http://127.0.0.1:$port/Curl_io/state" > /dev/null; then
  echo "~ the state page answered while the two payments waited"
else
  echo "~ the state page did not answer within half a second"
fi
wait $p4 $p5
echo "> POST /Curl_io/pay/4"; cat "$out/concurrent.4"; echo
echo "> POST /Curl_io/pay/5"; cat "$out/concurrent.5"; echo
sleep 3
charges
rows
