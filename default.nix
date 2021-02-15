{ lib, nix-gitignore, installShellFiles, buildDunePackage, async
, async_interactive, async_ssl, caqti-async, caqti-driver-sqlite3, cohttp-async
, core, cryptokit, ppx_log, uri, webbrowser, yojson }:

buildDunePackage {
  pname = "watch-later";
  version = "0.1.0";
  useDune2 = true;
  src = nix-gitignore.gitignoreFilterSource lib.cleanSourceFilter [ ] ./.;
  propagatedBuildInputs = [
    async
    async_interactive
    async_ssl
    caqti-async
    caqti-driver-sqlite3
    cohttp-async
    core
    cryptokit
    ppx_log
    uri
    webbrowser
    yojson
  ];
  meta = { homepage = "https://github.com/bcc32/watch-later"; };

  nativeBuildInputs = [ installShellFiles ];
  postInstall = ''
    installShellCompletion share/completions/watch-later.bash
  '';
}
