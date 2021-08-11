open! Core;
open! Async;
open! Import;

let main = (~dbpath, ~ignore_missing, ~video_ids) =>
  Video_db.with_file_and_txn(dbpath, ~f=db =>
    Deferred.Or_error.List.iter(video_ids, ~f=video_id =>
      switch%bind (Video_db.strict_remove(db, video_id)) {
      | `Ok => return()
      | `Missing =>
        if (ignore_missing) {
          return();
        } else {
          Deferred.Or_error.error_s(
            [%message
              "Attempted to remove video not in database"(
                video_id: Video_id.t,
              )
            ],
          );
        }
      }
    )
  );

let command =
  Command.async_or_error(
    ~summary="Remove videos from queue",
    {
      let%map_open.Command () = return()
      and dbpath = Params.dbpath
      and ignore_missing =
        flag(
          "-ignore-missing",
          no_arg,
          ~doc=" Silence errors about removing videos not in database",
          ~aliases=["-f"],
        )
      and video_ids = Params.nonempty_videos;
      () => main(~dbpath, ~ignore_missing, ~video_ids);
    },
  );
