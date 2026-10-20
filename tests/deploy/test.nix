# nixos/urweb.nix in a NixOS virtual machine, checked:
#
#   nix-build tests/deploy/test.nix
#
# Four instances of the application under tests/deploy/app: `app` in a
# group of its own with its database in its state directory, `shop` and
# `erp` sharing one database elsewhere through a group, whose other
# member reads and writes it too, and `pg` on Postgres, under /pg/ on
# app's virtual host.  Without KVM the machine is emulated, which takes
# minutes.
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
in
pkgs.testers.runNixOSTest {
  name = "urweb-deploy";
  requiredFeatures.kvm = false;

  nodes.machine = { config, lib, pkgs, ... }: {
    imports = [ ../../nixos/urweb.nix ];

    services.urweb.apps.app = {
      package = app;
      nginx.virtualHost = "app.example.org";
      quiet = false; # the journal shows its lines
      environment.TEST_GREETING = "hello";
      environmentFiles = [ "/etc/test-secret.env" ];
    };
    services.urweb.apps.shop = shared // {
      nginx.virtualHost = "shop.example.org";
    };
    services.urweb.apps.erp = shared // {
      nginx.virtualHost = "erp.example.org";
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
      # A secret outside the store, root's.
      "f /etc/test-secret.env 0400 root root - TEST_SECRET=hunter2"
    ];
    users.users.other = {
      isNormalUser = true;
      extraGroups = [ "ww" ];
    };
    environment.systemPackages = [ pkgs.curl pkgs.sqlite ];
    virtualisation.cores = 2;
  };

  testScript = ''
    units = ["urweb-app", "urweb-shop", "urweb-erp", "urweb-pg"]

    def get(host, path):
        return machine.succeed(f"curl -sfL -H 'Host: {host}' http://localhost{path}")

    for u in units:
        machine.wait_for_unit(f"{u}.socket")
        machine.wait_for_unit(f"{u}.service")
    machine.wait_for_unit("nginx.service")
    machine.wait_for_open_port(80)

    # The pages, on the databases preStart created; the environment and the secret reach the program.
    page = get("app.example.org", "/main")
    assert "0 visits" in page and "greeting: hello; secret: hunter2" in page, page
    assert "1 visits" in get("app.example.org", "/bump")
    assert "greeting: unset; secret: unset" in get("shop.example.org", "/main")

    # The sockets: root's, nginx's group, nobody else's.
    for u in units:
        assert machine.succeed(f"stat -c '%U %G %a' /run/{u}.sock").strip() == "root nginx 660"

    # The program's lines reach the journal.
    machine.succeed("journalctl -u urweb-app.service | grep -q 'Served URI /main'")

    # Each program's state is its group's: app's own, ww for shop and erp.
    print(machine.succeed("ls -la /var/db /var/lib/urweb-app /var/lib/urweb-shop /var/lib/urweb-erp"))
    assert machine.succeed("stat -c '%U %G %a' /var/lib/urweb-app").strip() == "urweb-app urweb-app 2770"
    assert machine.succeed("stat -c '%U %G %a' /var/lib/urweb-app/app.db").strip() == "urweb-app urweb-app 660"
    assert machine.succeed("stat -c '%U %G %a' /var/lib/urweb-app/sig").strip() == "urweb-app urweb-app 700"
    for u in ["urweb-shop", "urweb-erp"]:
        assert machine.succeed(f"stat -c '%U %G %a' /var/lib/{u}").strip() == f"{u} ww 2770"
        assert machine.succeed(f"stat -c '%U %G %a' /var/lib/{u}/sig").strip() == f"{u} ww 700"

    # One database for shop and erp, in the group's directory, group-writable with its WAL files.
    assert "0 visits" in get("shop.example.org", "/main")
    assert "1 visits" in get("shop.example.org", "/bump")
    assert "1 visits" in get("erp.example.org", "/main")
    assert "2 visits" in get("erp.example.org", "/bump")
    assert machine.succeed("stat -c '%G %a' /var/db/ww.db").strip() == "ww 660"
    assert machine.succeed("stat -c '%G %a' /var/db/ww.db-wal").strip() == "ww 660"

    # Another member of the group reads and writes it with shop-db, no root, and the programs go on.
    assert machine.succeed("runuser -u other -- shop-db 'select count(*) from uw_Visits'").strip() == "2"
    machine.succeed("runuser -u other -- erp-db \"insert into uw_Visits values (1000, '2026-10-19 00:00:00')\"")
    assert "3 visits" in get("shop.example.org", "/main")
    assert "4 visits" in get("erp.example.org", "/bump")
    assert machine.succeed("runuser -u other -- shop-db 'select count(*) from uw_Visits'").strip() == "4"
    machine.fail("runuser -u other -- cat /var/lib/urweb-shop/sig")

    # Outside app's group, nobody reaches its state.
    machine.fail("runuser -u other -- ls /var/lib/urweb-app/")
    machine.fail("runuser -u other -- app-db 'select 1'")

    # pg, on Postgres under /pg/ beside app on its virtual host.
    assert "0 visits" in get("app.example.org", "/pg/main")
    assert "1 visits" in get("app.example.org", "/pg/bump")
    assert "1 visits" in get("app.example.org", "/main") # app still serves its own

    # A database whose directory is missing: the start fails, saying so.
    machine.wait_until_succeeds("journalctl -u urweb-lost.service | grep -q '/var/missing does not exist'")

    # A package without a schema, from a .urp that names a sql file.
    import os
    assert os.access("${noSchemaApp}/bin/app", os.X_OK)
    assert not os.path.exists("${noSchemaApp}/share")

    # network: a loopback of its own without, the host's with.
    assert machine.succeed("systemctl show -p PrivateNetwork --value urweb-shop.service").strip() == "yes"
    assert machine.succeed("systemctl show -p PrivateNetwork --value urweb-erp.service").strip() == "no"

    # Nothing the sandbox took away was needed: no program was killed or restarted.
    for u in units:
        assert machine.succeed(f"systemctl show -p NRestarts --value {u}.service").strip() == "0"
        machine.fail(f"journalctl -u {u}.service | grep -e SIGSYS -e 'dumped core' -e 'code=killed'")

    # The exposure, and nginx's beside it.
    for u in units + ["nginx"]:
        print(machine.succeed(f"systemd-analyze security --no-pager {u}.service"))
  '';
}
