open! Core;
open! Async;
open! Import;

module Print_path = {
  let command =
    Command.async_or_error(
      ~summary="Print file path to database",
      {
        let%map_open.Command () = return()
        and dbpath = Params.dbpath;
        () => {
          print_endline(dbpath);
          return();
        };
      },
    );
};

module Query = {
  let make_safe_path = file =>
    if (Filename.is_implicit(file)) {
      "./" ++ file;
    } else {
      file;
    };

  let command =
    Command.basic(
      ~summary="Run an arbitrary SQL query on stdin against the database",
      {
        let%map_open.Command () = return()
        and dbpath = Params.dbpath
        and args =
          flag(
            "--",
            escape,
            ~doc="ARG command-line arguments to pass to sqlite3",
          )
          >>| Option.value(~default=[]);

        () =>
          /* Caqti doesn't support ['a Caqti_type.t]s that read unknown numbers of fields,
             so we just shell out to the sqlite3 command-line shell instead. */
          never_returns(
            Core.Unix.exec(
              ~prog="sqlite3",
              ~argv=
                ["sqlite3", "-batch", "-bail", "-init", "/dev/null"]
                @ args
                @ [make_safe_path(dbpath)],
              (),
            ),
          );
      },
    );
};

let command =
  Command.group(
    ~summary="Database debugging commands",
    [("path", Print_path.command), ("query", Query.command)],
  );
