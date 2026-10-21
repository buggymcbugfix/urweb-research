# The application as a package, as the module expects it: the files
# app.urp names, under tests/deploy/app.
{ urweb, dbms ? "sqlite", prefix ? "/", schema ? true }:
urweb.buildApp {
  pname = "app";
  root = ./app;
  inherit dbms prefix schema;
}
