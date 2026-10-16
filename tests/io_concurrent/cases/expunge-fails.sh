# The clientLeaves task fails the first time it runs (BOUNDED_RETRY), and
# so A's expunge with it: the database is left as it was, and A should be
# tried again at the pruner's next pass, and then expunged.  A is made and
# left alone, and a poll of A's is refused once it has been expunged.
seed "INSERT INTO uw_Concurrent_flags (uw_Name) VALUES ('failLeave');"
start
client A
wait_for "a client left"
if curl_msgs A | tidy | grep -q '^R$'; then
  echo "a poll of A: refused"
else
  echo "a poll of A: answered, A is still there"
fi
