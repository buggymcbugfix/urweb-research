# nixos/urweb.nix in a NixOS virtual machine, against a golden:
#
#   nix-build tests/deploy/test.nix       # fails with the diff from expected/vm
#   tests/deploy/update.sh --vm           # runs the machine and rewrites expected/vm
#
# Four instances of the application under tests/deploy/app: `app` in a
# group of its own with its database in its state directory and two
# secrets as credentials, `shop` and `erp` sharing one database
# elsewhere through a group, whose other member reads and writes it
# too, `erp` with one of the secrets, and `pg` on Postgres, under /pg/
# on app's virtual host.  Without KVM the machine is emulated, which
# takes minutes.
#
# The script fails only when a unit does not come up.  Everything else
# it looks at, the pages, the owners and modes, what the sandboxes
# allow, it records into expected/vm, as urt keeps snapshots: a change
# shows as a diff to read, not as an assertion to rewrite.
{ pkgs ? import ../../nixpkgs.nix }:
let
  urweb = pkgs.pkgsLinux.callPackage ../../derivation.nix { };
  app = pkgs.pkgsLinux.callPackage ./app.nix { inherit urweb; };
  pgApp = pkgs.pkgsLinux.callPackage ./app.nix { inherit urweb; dbms = "postgres"; prefix = "/pg/"; };
  # Built only: a database set up otherwise, and the sql file app.urp names.
  noSchemaApp = pkgs.pkgsLinux.callPackage ./app.nix { inherit urweb; schema = false; };
  shared = {
    package = app;
    database = { path = "/var/db/ww.db"; group = "ww"; };
  };
  vm = pkgs.testers.runNixOSTest {
    name = "urweb-deploy";
    requiredFeatures.kvm = false;

    nodes.machine = { config, lib, pkgs, ... }: {
      imports = [ ../../nixos/urweb.nix ];

      services.urweb.apps.app = {
        package = app;
        nginx.virtualHost = "app.example.org";
        quiet = false; # the journal shows its lines
        environment.TEST_GREETING = "hello";
        credentials = {
          TEST_SECRET_1 = "/etc/secrets/secret1";
          TEST_SECRET_2 = "/etc/secrets/secret2";
        };
      };
      services.urweb.apps.shop = shared // {
        nginx.virtualHost = "shop.example.org";
      };
      services.urweb.apps.erp = shared // {
        nginx.virtualHost = "erp.example.org";
        credentials = {
          TEST_SECRET_1 = "/etc/secrets/secret1";
        };
        network = true;
      };
      # Its database's directory is not there.
      services.urweb.apps.lost = {
        package = app;
        database.path = "/var/missing/lost.db";
      };
      services.urweb.apps.pg = {
        package = pgApp;
        nginx.virtualHost = "app.example.org";
        environment.URWEB_PQ_CON = "dbname=urweb-pg";
      };

      # pg's database, its tables created as its user, as a deployment would.
      services.postgresql = {
        enable = true;
        ensureDatabases = [ "urweb-pg" ];
        ensureUsers = [ { name = "urweb-pg"; ensureDBOwnership = true; } ];
      };
      # initdb on the first boot of an emulated machine can take longer than
      # the 120 s systemd allows PostgreSQL to start.
      systemd.services.postgresql.serviceConfig.TimeoutSec = lib.mkForce 600;
      systemd.services.urweb-pg-schema = {
        requiredBy = [ "urweb-pg.service" ];
        before = [ "urweb-pg.service" ];
        requires = [ "postgresql.target" ];
        after = [ "postgresql.target" ];
        serviceConfig = { Type = "oneshot"; RemainAfterExit = true; User = "urweb-pg"; };
        script = "${config.services.postgresql.package}/bin/psql -d urweb-pg -v ON_ERROR_STOP=1 -f ${pgApp}/share/app/schema.sql";
      };

      systemd.tmpfiles.rules = [
        # The shared database's directory: the group's.
        "d /var/db 2770 root ww -"
        # Root-owned secrets outside the store: one with a quote, a space and a backslash, one with a newline at the end.
        "d /etc/secrets 0700 root root -"
        "f /etc/secrets/secret1 0400 root root - hun\\x20ter\\x222\\x5c"
        "f /etc/secrets/secret2 0400 root root - hello,\\x20world!\\n"
      ];
      users.users.other = {
        isNormalUser = true;
        extraGroups = [ "ww" ];
      };
      environment.systemPackages = [ pkgs.curl pkgs.sqlite ];
      virtualisation.cores = 2;
    };

    testScript = ''
      import re
      units = ["urweb-app", "urweb-shop", "urweb-erp", "urweb-pg"]

      observed = []
      def record(title, text):
          observed.append(f"### {title}\n{text.rstrip()}\n")
      # What a command on the machine prints and returns.  A failure is a
      # result too: it is recorded, and the golden says whether it is the
      # expected one.
      def observe(title, cmd):
          status, out = machine.execute(cmd)
          record(title, f"{out.rstrip()}\nexit {status}")
      def get(title, host, path):
          observe(f"{title}: {host}{path}", f"curl -sfL -H 'Host: {host}' http://localhost{path}")
      def stat(title, *paths):
          observe(title, "stat -c '%n %U %G %a' " + " ".join(paths))

      for u in units:
          machine.wait_for_unit(f"{u}.socket")
          machine.wait_for_unit(f"{u}.service")
      machine.wait_for_unit("nginx.service")
      machine.wait_for_open_port(80)

      # The pages, on the databases preStart created.  The environment and
      # the credentials reach app, a secret as the file's bytes, a trailing
      # newline stripped; erp has the one it was given, shop none.
      get("app's page", "app.example.org", "/main")
      get("app's page, bumped", "app.example.org", "/bump")
      get("erp's page", "erp.example.org", "/main")
      get("shop's page", "shop.example.org", "/main")

      # The secrets: root's files, in no unit file and no unit's environment, unreadable to another user.
      observe("the secrets' files", "cat -A /etc/secrets/secret1 /etc/secrets/secret2; stat -c '%n %U %G %a' /etc/secrets /etc/secrets/*")
      observe("no secret in a unit file", "grep -r 'hun ter' /etc/systemd/system/")
      for u in units:
          observe(f"{u}'s unit environment", f"systemctl show -p Environment {u}.service")
      observe("another user reads a secret", "runuser -u other -- cat /etc/secrets/secret1")

      # The sockets: root's, nginx's group, nobody else's.
      stat("the sockets", *[f"/run/{u}.sock" for u in units])

      # The program's lines reach the journal.
      observe("app's journal", "journalctl -o cat -u urweb-app.service | grep 'Served URI /main' | head -1")

      # Each program's state is its group's: app's own, ww for shop and erp.
      observe("the state directories", "find /var/db /var/lib/urweb-app /var/lib/urweb-shop /var/lib/urweb-erp -printf '%p %u %g %m\\n' | sort")

      # One database for shop and erp, in the group's directory, group-writable with its WAL files.
      get("shop's page", "shop.example.org", "/main")
      get("shop's page, bumped", "shop.example.org", "/bump")
      get("erp's page", "erp.example.org", "/main")
      get("erp's page, bumped", "erp.example.org", "/bump")
      stat("the shared database", "/var/db/ww.db", "/var/db/ww.db-wal")

      # Another member of the group reads and writes it with shop-db, no root, and the programs go on.
      observe("another user counts the visits with shop-db", "runuser -u other -- shop-db 'select count(*) from uw_Visits'")
      observe("another user inserts a visit with erp-db", "runuser -u other -- erp-db \"insert into uw_Visits values (1000, '2026-10-19 00:00:00')\"")
      get("shop's page", "shop.example.org", "/main")
      get("erp's page, bumped", "erp.example.org", "/bump")
      observe("another user counts the visits with shop-db", "runuser -u other -- shop-db 'select count(*) from uw_Visits'")
      observe("another user reads shop's key", "runuser -u other -- cat /var/lib/urweb-shop/sig")

      # Outside app's group, nobody reaches its state.
      observe("another user lists app's state", "runuser -u other -- ls /var/lib/urweb-app/")
      observe("another user opens app's database", "runuser -u other -- app-db 'select 1'")

      # pg, on Postgres under /pg/ beside app on its virtual host.
      get("pg's page", "app.example.org", "/pg/main")
      get("pg's page, bumped", "app.example.org", "/pg/bump")
      get("app's page, still its own", "app.example.org", "/main")

      # A database whose directory is missing: the start fails, saying so.
      machine.wait_until_succeeds("journalctl -u urweb-lost.service | grep -q '/var/missing does not exist'")
      observe("lost's journal", "journalctl -o cat -u urweb-lost.service | grep 'does not exist' | head -1")

      # A package without a schema, from a .urp that names a sql file.
      import os
      record("the package without a schema", "bin/app executable: %s\nshare: %s" % (
          os.access("${noSchemaApp}/bin/app", os.X_OK), os.path.exists("${noSchemaApp}/share")))

      # network: a loopback of its own without, the host's with.
      observe("the private networks", "systemctl show -p PrivateNetwork " + " ".join(f"{u}.service" for u in units))

      # Nothing the sandbox took away was needed: no program was killed or restarted.
      observe("the restarts", "systemctl show -p NRestarts " + " ".join(f"{u}.service" for u in units))
      for u in units:
          observe(f"{u} killed", f"journalctl -u {u}.service | grep -e SIGSYS -e 'dumped core' -e 'code=killed'")

      # The exposure, and nginx's beside it.
      for u in units + ["nginx"]:
          observe(f"the exposure of {u}", f"systemd-analyze security --no-pager {u}.service")

      text = re.sub(r"/nix/store/[a-z0-9]{32}-", "/nix/store/HASH-", "\n".join(observed))
      # systemd-analyze pads its columns, to the end of the line.
      text = re.sub(r"[ \t]+$", "", text, flags=re.M)
      (driver.out_dir / "observed").write_text(text)
    '';
  };
in
pkgs.runCommand "urweb-deploy" { passthru = { inherit vm; }; } (
  if builtins.pathExists ./expected/vm then ''
    diff -u ${./expected/vm} ${vm}/observed
    touch $out
  '' else ''
    echo "no tests/deploy/expected/vm to compare with: tests/deploy/update.sh --vm records one" >&2
    exit 1
  ''
)
