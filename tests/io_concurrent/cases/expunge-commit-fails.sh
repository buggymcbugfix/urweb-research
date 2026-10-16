# The clientLeaves task registers a transactional whose commit fails before
# the database's COMMIT, every time, so every expunge of A is rolled back:
# A's row in chans stays, and so should A, tried again at every pass.  A
# is made and left alone for 12 s, two passes and more; then the kinds of
# line logged, a poll of A's, and the rows of chans.
seed "INSERT INTO uw_Concurrent_flags (uw_Name) VALUES ('failLeaveCommit');"
start
client A
sleep 12
log_kinds
if curl_msgs A | tidy | grep -q '^R$'; then
  echo "a poll of A: refused"
else
  echo "a poll of A: answered, A is still there"
fi
echo "rows of chans: $(sqlite3 "$db" "SELECT COUNT(*) FROM uw_Concurrent_chans;")"
