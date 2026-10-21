fun show (o : option string) = case o of None => "unset" | Some s => s

table visits : { Id : int, At : time }
  PRIMARY KEY Id

sequence ids

fun main () =
    n <- oneRowE1 (SELECT COUNT( * ) AS N FROM visits);
    greeting <- getenv (blessEnvVar "TEST_GREETING");
    secret1 <- getenv (blessEnvVar "TEST_SECRET_1");
    secret2 <- getenv (blessEnvVar "TEST_SECRET_2");
    return <xml><body>
      <p>{[n]} visits</p>
      <p>greeting: {[greeting]}; secret1: {[secret1]}; secret2: {[secret2]}</p>
      <a link={bump ()}>bump</a>
    </body></xml>

and bump () =
    id <- nextval ids;
    t <- now;
    dml (INSERT INTO visits (Id, At) VALUES ({[id]}, {[t]}));
    redirect (url (main ()))
