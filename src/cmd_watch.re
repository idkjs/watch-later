open! Core;
open! Async;
open! Import;

let browse_video = video_id =>
  Browse.url(
    Uri.of_string(sprintf("https://youtu.be/%{Video_id}"^, video_id)),
  );

let main = (~dbpath, ~mark_watched, ~which_videos: Which_videos.t) =>
  Video_db.with_file_and_txn(
    dbpath,
    ~f=db => {
      let%bind which_videos =
        switch (which_videos) {
        | These(ids) => return(ids)
        | Filter(filter) =>
          let%map video_id = Video_db.get_random_video(db, filter);
          [video_id];
        };

      Deferred.Or_error.List.iter(
        which_videos,
        ~f=video_id => {
          let%bind () = browse_video(video_id);
          if (mark_watched) {
            Video_db.mark_watched(db, video_id, `Watched);
          } else {
            return();
          };
        },
      );
    },
  );

let command =
  Command.async_or_error(
    ~summary="Open a video in $BROWSER and mark it watched.",
    ~readme=
      () =>
        {|
If video IDs are specified, process each video in sequence.

If a filter is specified, select one video matching the filter at random.

If neither video IDs nor filter is specified, defaults to selecting a random unwatched
video.
|},
    {
      let%map_open.Command () = return()
      and dbpath = Params.dbpath
      and mark_watched =
        flag_optional_with_default_doc(
          "-mark-watched",
          bool,
          [%sexp_of: bool],
          ~default=true,
          ~doc="(true|false) mark video as watched (default true)",
        )
      and which_videos =
        Which_videos.param(~default=Filter(Filter.unwatched));
      () => main(~dbpath, ~mark_watched, ~which_videos);
    },
  );
