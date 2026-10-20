# Ur/Web FastCGI programs as NixOS services behind nginx.
#
#   imports = [ "${urweb}/nixos/urweb.nix" ];
#   services.urweb.apps.shop = {
#     package = urweb.buildApp { pname = "shop"; src = ./.; };
#     nginx.virtualHost = "shop.example.org";
#   };
#
# Each application gets a socket unit and a service, urweb-<name>: systemd
# creates /run/urweb-<name>.sock as root before the program starts, gives
# it nginx's group, keeps it open across restarts of the program, and
# hands the listening descriptor to the program as descriptor 0, as
# spawn-fcgi does.  The program never touches the socket file.  Its
# signing key, and by default its SQLite database, are in
# /var/lib/urweb-<name>.  The service is sandboxed as far as systemd
# allows: the program accepts on an inherited descriptor, reads its store
# path, writes its state directory and the journal, and needs nothing
# else; what else it needs is said with database, readWritePaths and
# network.
#
# Access to what the program writes is by group.  It runs as the static
# user urweb-<name> in the group database.group, its own group
# urweb-<name> by default; its state directory is the group's, 2770 with
# the group inherited, and it creates its files group-writable (umask
# 0007), so SQLite's database and its WAL files are open to the group
# and closed to everyone else.  An account in the group works on the
# database without root, with <name>-db, sqlite3 on that database, which
# the module installs.  Applications sharing a database name one group
# and, with database.path, a directory outside their state directories
# that the deployment provides with that group.  The key stays the
# program's own, since the runtime creates it 0700.
{ config, lib, pkgs, utils, ... }:
let
  inherit (lib) mkOption mkIf mkDefault types mapAttrs' mapAttrsToList nameValuePair filterAttrs optional optionals optionalString literalExpression hasPrefix;
  cfg = config.services.urweb;
  unit = name: "urweb-${name}";
  socket = name: "/run/urweb-${name}.sock";
  dir = name: "/var/lib/urweb-${name}";
  # A path in a unit setting that takes a list of them, such as
  # ReadWritePaths, which splits at spaces and expands %.
  unitPath = p: "\"${lib.escape [ "\\" "\"" ] (lib.replaceStrings [ "%" ] [ "%%" ] p)}\"";
in
{
  options.services.urweb.apps = mkOption {
    default = { };
    description = "Ur/Web FastCGI programs, each a systemd service behind nginx.";
    type = types.attrsOf (types.submodule ({ name, config, ... }:
      let
        # How the package was built, when urweb.buildApp built it.
        built = config.package.urweb or { };
        pname = built.name or name;
      in
      {
        options = {
          package = mkOption {
            type = types.package;
            description = "The application, as urweb.buildApp builds it: bin/<name> with -protocol fastcgi, share/<name>/schema.sql from -sql, and passthru.urweb saying so.  A package built otherwise says it with exe, schema and database.type.";
          };
          exe = mkOption {
            type = types.str;
            default = "${config.package}/bin/${pname}";
            defaultText = literalExpression ''"''${package}/bin/<name>"'';
            description = "The program.";
          };
          schema = mkOption {
            type = types.nullOr types.path;
            default =
              if config.database.type == "sqlite" && (built.schema or true)
              then "${config.package}/share/${pname}/schema.sql"
              else null;
            defaultText = literalExpression ''"''${package}/share/<name>/schema.sql" for SQLite, null for Postgres or a package built with schema = false'';
            description = "The SQL that creates the SQLite database on the first start; null for a database set up otherwise, by migrations say.";
          };
          threads = mkOption {
            type = types.ints.positive;
            default = 8;
            description = "Worker threads: how many requests are served at once.";
          };
          quiet = mkOption {
            type = types.bool;
            default = true;
            description = "-q: no line per request in the journal.";
          };
          environment = mkOption {
            type = types.attrsOf types.str;
            default = { };
            description = "Environment of the program.  It ends up in the unit file, readable by everyone: secrets go in environmentFiles.";
          };
          environmentFiles = mkOption {
            type = types.listOf types.str;
            default = [ ];
            description = "Files of NAME=value lines read into the program's environment at start, outside the store and root's: for secrets.";
          };
          database.type = mkOption {
            type = types.enum [ "sqlite" "postgres" ];
            default = built.dbms or "sqlite";
            defaultText = literalExpression ''package.urweb.dbms or "sqlite"'';
            description = "The -dbms the program was built with.  With Postgres the program is given AF_UNIX for /run/postgresql, URWEB_PQ_CON goes in environment, and the database is the deployment's.";
          };
          database.path = mkOption {
            type = types.str;
            default = "${dir name}/${name}.db";
            defaultText = literalExpression ''"/var/lib/urweb-<name>/<name>.db"'';
            description = "The SQLite database.  Outside the state directory, its directory is made writable for the program and has to exist, with the group.";
          };
          database.group = mkOption {
            type = types.str;
            default = unit name;
            defaultText = literalExpression ''"urweb-<name>"'';
            description = "The program's group, whose members read and write its database and whatever else it writes: its own by default, one group for applications sharing a database.";
          };
          database.sqlitePackage = mkOption {
            type = types.package;
            default = pkgs.sqlite;
            defaultText = literalExpression "pkgs.sqlite";
            description = "The sqlite3 that <name>-db runs on the database.";
          };
          readWritePaths = mkOption {
            type = types.listOf types.str;
            default = [ ];
            description = "Directories the program writes besides its state directory and the database's.";
          };
          network = mkOption {
            type = types.bool;
            default = false;
            description = "Whether the program may use the network: a database over TCP, mail, requests of its own.  Without, it has a loopback of its own and no sockets of its own at all.";
          };
          nginx.virtualHost = mkOption {
            type = types.nullOr types.str;
            default = null;
            description = "An nginx virtual host to serve the application on.";
          };
          nginx.location = mkOption {
            type = types.str;
            default = built.prefix or "/";
            defaultText = literalExpression ''package.urweb.prefix or "/"'';
            description = "Where on the virtual host: the prefix the application was built with.";
          };
          nginx.keepalive = mkOption {
            type = types.ints.unsigned;
            default = 32;
            description = "Idle connections to the program nginx keeps per worker process: at least threads, and at least the requests in flight at once for keep-alive to pay; 0 for a connection per request.";
          };
        };
      }));
  };

  config = mkIf (cfg.apps != { }) {
    assertions = lib.concatLists (mapAttrsToList (name: app:
      let
        at = "services.urweb.apps.${name}";
        protocol = app.package.urweb.protocol or "fastcgi";
        # The other applications served at the same place.
        sharing = lib.attrNames (filterAttrs (other: o:
          other != name && app.nginx.virtualHost != null
          && o.nginx.virtualHost == app.nginx.virtualHost && o.nginx.location == app.nginx.location) cfg.apps);
      in
      [
        {
          assertion = protocol == "fastcgi";
          message = "${at}: the package was built with -protocol ${protocol}; the module runs FastCGI programs. Build it with urweb.buildApp, or with -protocol fastcgi.";
        }
        {
          assertion = app.nginx.keepalive == 0 || app.nginx.keepalive >= app.threads;
          message = "${at}: nginx.keepalive = ${toString app.nginx.keepalive}, but threads = ${toString app.threads}. nginx closes idle connections beyond keepalive each time a request ends, so under load it would close connections the program could still serve on, which costs more than keeping them. Set nginx.keepalive to at least ${toString app.threads}, or to 0 for a connection per request.";
        }
        {
          assertion = sharing == [ ];
          message = "${at}: ${lib.concatStringsSep ", " (map (o: "services.urweb.apps.${o}") sharing)} is served at the same nginx.location ${app.nginx.location} of ${toString app.nginx.virtualHost}. Give each application a location of its own.";
        }
      ]) cfg.apps);

    systemd.sockets = mapAttrs' (name: app: nameValuePair (unit name) {
      description = "Socket of the Ur/Web application ${name}";
      wantedBy = [ "sockets.target" ];
      socketConfig = {
        ListenStream = socket name;
        SocketGroup = config.services.nginx.group; # nginx's workers connect through the group
        SocketMode = "0660"; # nobody else does
      };
    }) cfg.apps;

    systemd.services = mapAttrs' (name: app:
      let
        sqlite = app.database.type == "sqlite";
        localPostgres = app.database.type == "postgres" && config.services.postgresql.enable;
        db = app.database.path;
        # The database's directory, when it is not the state directory.
        dbDir = optional (sqlite && !hasPrefix "${dir name}/" db) (dirOf db);
      in
      nameValuePair (unit name) {
        description = "Ur/Web application ${name}";
        requires = [ "${unit name}.socket" ];
        # A Postgres program fails to start without its server, so it
        # waits for one on this machine.
        wants = optional localPostgres "postgresql.target";
        after = [ "${unit name}.socket" ] ++ optional localPostgres "postgresql.target";
        wantedBy = [ "multi-user.target" ]; # at boot, not on the first request
        environment = {
          URWEB_SIGFILE_PATH = "${dir name}/sig"; # created on the first start
        } // lib.optionalAttrs sqlite {
          URWEB_SQLITE_DB_PATH = db;
        } // app.environment;
        # Under a lock on the database's directory: applications sharing a
        # database start together, and only one may create it.  flock
        # would create a missing directory as a file, so it is checked
        # first.
        preStart = optionalString (sqlite && app.schema != null) ''
          dir=${lib.escapeShellArg (dirOf db)}
          if [ ! -d "$dir" ]; then
            echo "$dir does not exist. It is the directory of database.path, which the deployment makes, with the group ${app.database.group}." >&2
            exit 1
          fi
          exec ${pkgs.util-linux}/bin/flock "$dir" ${pkgs.writeShellScript "${unit name}-create-database" ''
            set -eu
            db=${lib.escapeShellArg db}
            # Empty is how sqlite3 leaves a database it opened that did
            # not exist, as <name>-db does before the first start.
            if [ -s "$db" ]; then
              exit 0
            fi
            # Made aside and renamed into place, so that a schema that
            # fails, or a start cut short, leaves no database behind.
            tmp=$(mktemp "$db.XXXXXX")
            trap 'rm -f "$tmp"' EXIT
            ${lib.getExe' app.database.sqlitePackage "sqlite3"} -bail "$tmp" < ${lib.escapeShellArg "${app.schema}"}
            # SQLite gives its WAL files the database's mode.
            chmod 660 "$tmp"
            mv "$tmp" "$db"
          ''}
        '';
        serviceConfig = {
          ExecStart = utils.escapeSystemdExecArgs ([ app.exe "-t" (toString app.threads) ] ++ optional app.quiet "-q");
          EnvironmentFile = app.environmentFiles;
          StandardInput = "socket"; # the listening socket as descriptor 0
          StandardOutput = "journal"; # with StandardInput=socket both would default to the socket
          StandardError = "journal";
          Restart = "on-failure";
          User = unit name;
          Group = app.database.group;
          UMask = "0007"; # what the program creates is the group's too
          StateDirectory = unit name;
          StateDirectoryMode = "2770"; # the group's, inherited by new files
          ReadWritePaths = map unitPath (dbDir ++ app.readWritePaths);

          # The sandbox.  The program needs none of what these take away.
          NoNewPrivileges = true;
          CapabilityBoundingSet = "";
          ProtectSystem = "strict"; # nothing writable but the state directory and ReadWritePaths
          ProtectHome = true;
          PrivateTmp = true;
          PrivateDevices = true;
          PrivateUsers = true;
          PrivateNetwork = !app.network; # the socket comes in as a descriptor
          # What may call socket(): nothing in a SQLite program, since its
          # listening socket is inherited and accept() is not socket();
          # libpq for /run/postgresql; the network, and glibc's lookups
          # through nscd with it.
          RestrictAddressFamilies =
            if app.network then [ "AF_UNIX" "AF_INET" "AF_INET6" ]
            else if sqlite then "none"
            else [ "AF_UNIX" ];
          ProtectKernelTunables = true;
          ProtectKernelModules = true;
          ProtectKernelLogs = true;
          ProtectControlGroups = true;
          ProtectClock = true;
          ProtectHostname = true;
          ProtectProc = "invisible";
          ProcSubset = "pid";
          RestrictNamespaces = true;
          RestrictRealtime = true;
          RestrictSUIDSGID = true;
          LockPersonality = true;
          MemoryDenyWriteExecute = true;
          RemoveIPC = true;
          SystemCallArchitectures = "native";
          SystemCallFilter = [ "@system-service" "~@privileged" "~@resources" ];
          SocketBindDeny = "any";
        } // lib.optionalAttrs (!app.network) {
          IPAddressDeny = "any";
        };
      }) cfg.apps;

    users.users = mapAttrs' (name: app: nameValuePair (unit name) {
      isSystemUser = true;
      group = app.database.group;
    }) cfg.apps;
    users.groups = mapAttrs' (name: app: nameValuePair app.database.group { }) cfg.apps;

    # <name>-db: sqlite3 on the application's database, for the group.
    # umask 007: SQLite gives a WAL file the database's mode through the
    # umask, and one the operator creates 0640 locks the program out.
    environment.systemPackages = mapAttrsToList (name: app:
      pkgs.writeShellScriptBin "${name}-db" ''
        umask 007
        exec ${lib.getExe' app.database.sqlitePackage "sqlite3"} -cmd "PRAGMA foreign_keys = ON" ${lib.escapeShellArg app.database.path} "$@"
      '') (filterAttrs (_: app: app.database.type == "sqlite") cfg.apps);

    services.nginx = {
      enable = mkDefault true;
      upstreams = mapAttrs' (name: app: nameValuePair (unit name) {
        servers."unix:${socket name}" = { };
        extraConfig = optionalString (app.nginx.keepalive > 0) "keepalive ${toString app.nginx.keepalive};";
      }) cfg.apps;
      # Merged, so that applications on one virtual host each get their location.
      virtualHosts = lib.mkMerge (mapAttrsToList (name: app: {
        ${app.nginx.virtualHost}.locations.${app.nginx.location}.extraConfig = ''
          fastcgi_pass ${unit name};
          fastcgi_keep_conn ${if app.nginx.keepalive > 0 then "on" else "off"};
        '';
      }) (filterAttrs (_: app: app.nginx.virtualHost != null) cfg.apps));
    };
  };
}
