open! Core;
open! Async;
open! Import;

let main = (~dbpath, ~id, ~which_videos: Which_videos.t) => {
  let print = ((video_info: Video_info.t, watched)) =>
    if (id) {
      printf("%{Video_id}\n"^, video_info.video_id);
    } else {
      print_s([%message (video_info: Video_info.t)(watched: bool)]);
    };

  Video_db.with_file_and_txn(dbpath, ~f=db =>
    switch (which_videos) {
    | These(ids) =>
      Deferred.Or_error.List.iter(
        ids,
        ~f=video_id => {
          let%bind info =
            switch%bind (Video_db.get(db, video_id)) {
            | Some(info) => return(info)
            | None =>
              Deferred.Or_error.error_s(
                [%message "Video not found"(video_id: Video_id.t)],
              )
            };

          print(info);
          return();
        },
      )
    | Filter(filter) =>
      let%bind () =
        Video_db.get_videos(db, filter)
        |> Pipe.iter_without_pushback(~f=print)
        |> Deferred.ok;

      return();
    }
  );
};

let command =
  Command.async_or_error(
    ~summary="List videos according to filter.",
    {
      let%map_open.Command () = return()
      and dbpath = Params.dbpath
      and which_videos = Which_videos.param(~default=Filter(Filter.empty))
      and id =
        flag(
          "-id",
          no_arg,
          ~doc=
            " If passed, print just the video ID rather than all the video info",
        );

      () => {
        /* FIXME: Move this to bin/main.ml.  Ditto for Async log output setup. */
        Writer.behave_nicely_in_pipeline();
        main(~dbpath, ~id, ~which_videos);
      };
    },
  );
