open! Core;
open! Async;
open! Import;

let main = dbpath =>
  Video_db.with_file_and_txn(
    dbpath,
    ~f=db => {
      let%bind stats = Video_db.video_stats(db);
      print_s([%sexp (stats: Stats.t)]);
      return();
    },
  );

let command =
  Command.async_or_error(
    ~summary="Show stats about the YouTube queue",
    {
      let%map_open.Command () = return()
      and dbpath = Params.dbpath;
      () => main(dbpath);
    },
  );
