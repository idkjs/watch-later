open! Core;
open! Async;
open! Import;

/* FIXME: This function is now quite convoluted.  Try to simplify. */
let main = (~api, ~dbpath, ~mark_watched, ~overwrite, ~videos_or_playlist) => {
  let%bind playlist_items_to_delete =
    Video_db.with_file_and_txn(
      dbpath,
      ~f=db => {
        let process_video_info =
          switch (mark_watched) {
          | None => (
              video_info => Video_db.add_video(db, video_info, ~overwrite)
            )
          | Some(state) => (
              video_info => {
                let%bind () = Video_db.add_video(db, video_info, ~overwrite);
                let%bind () =
                  Video_db.mark_watched(db, video_info.video_id, state);
                return();
              }
            )
          };

        let process_video_infos = video_infos =>
          switch%map.Deferred (
            video_infos
            |> Pipe.filter_map'(~f=video_info =>
                 Deferred.map(
                   ~f=Result.error,
                   {
                     let%bind video_info = Deferred.return(video_info);
                     process_video_info(video_info);
                   },
                 )
               )
            |> Pipe.to_list
          ) {
          | [] => Ok()
          | [_, ..._] as errors => Error(Error.of_list(errors))
          };

        let%bind playlist_items_to_delete =
          switch (videos_or_playlist) {
          | `Videos(video_ids) =>
            let video_ids_to_lookup = video_ids |> Pipe.of_list;
            let video_ids_to_lookup =
              if (overwrite) {
                video_ids_to_lookup;
              } else {
                video_ids_to_lookup
                |> Pipe.filter_map'(~f=video_id =>
                     if%map.Deferred (Video_db.mem(db, video_id)
                                      |> Deferred.Or_error.ok_exn) {
                       None;
                     } else {
                       Some(video_id);
                     }
                   );
              };

            let%bind () =
              video_ids_to_lookup
              |> Youtube_api.get_video_info(api)
              |> process_video_infos;

            return([]);
          | `Playlist(playlist_id, delete_after_processing) =>
            let pipe = Youtube_api.get_playlist_items(api, playlist_id);
            let (video_infos, playlist_items_to_delete) = {
              let (playlist_items, playlist_items_to_delete) =
                if (delete_after_processing) {
                  let (r1, r2) =
                    Pipe.fork(pipe, ~pushback_uses=`Both_consumers);
                  (
                    r1,
                    r2
                    |> Pipe.to_list
                    |> Deferred.map(~f=Or_error.combine_errors),
                  );
                } else {
                  (pipe, return([]));
                };

              (
                Pipe.map(
                  playlist_items,
                  ~f=Or_error.bind(~f=Playlist_item.video_info),
                ),
                playlist_items_to_delete,
              );
            };

            let%bind () = process_video_infos(video_infos);
            playlist_items_to_delete;
          };

        return(playlist_items_to_delete);
      },
    );

  Deferred.Or_error.List.iter(playlist_items_to_delete, ~f=item =>
    Youtube_api.delete_playlist_item(api, Playlist_item.id(item))
  );
};

let command =
  Youtube_api.command(
    ~summary="Add video(s) to queue",
    {
      let%map_open.Command () = return()
      and dbpath = Params.dbpath
      and mark_watched =
        flag(
          "-mark-watched",
          optional(bool),
          ~doc="(true|false) mark video as watched (default do nothing)",
        )
        >>| Option.map(
              ~f=
                fun
                | true => `Watched
                | false => `Unwatched,
            )
      and overwrite =
        flag(
          "-overwrite",
          no_arg,
          ~doc=" overwrite existing entries (default skip)",
          ~aliases=["-f"],
        )
      and videos_or_playlist = Params.nonempty_videos_or_playlist;
      api =>
        main(~api, ~dbpath, ~mark_watched, ~overwrite, ~videos_or_playlist);
    },
  );
