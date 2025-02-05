open! Core;
open! Async;
open! Import;

let main = (~dbpath, ~undo, ~video_ids) =>
  Video_db.with_file_and_txn(dbpath, ~f=db =>
    Deferred.Or_error.List.iter(video_ids, ~f=spec =>
      Video_db.mark_watched(
        db,
        spec,
        if (undo) {
          `Unwatched;
        } else {
          `Watched;
        },
      )
    )
  );

let command =
  Command.async_or_error(
    ~summary="Mark video(s) as watched",
    {
      let%map_open.Command () = return()
      and dbpath = Params.dbpath
      and undo = flag("-undo", no_arg, ~doc=" mark as unwatched instead")
      and video_ids = Params.nonempty_videos;
      () => main(~dbpath, ~undo, ~video_ids);
    },
  );
