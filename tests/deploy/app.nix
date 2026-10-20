# The application as a package, as the module expects it: the files
# app.urp names, under tests/deploy/app.
{ urweb }:
urweb.buildApp {
  pname = "app";
  root = ./app;
}
