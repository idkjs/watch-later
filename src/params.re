open! Core;
open! Async;
open! Import;

let dbpath = {
  let default_db_path = Watch_later_directories.default_db_path;
  Command.Param.(
    flag(
      "-dbpath",
      optional_with_default(default_db_path, Filename.arg_type),
      ~doc=
        "FILE path to database file (default is $XDG_DATA_HOME/watch-later/watch-later.db)",
    )
  );
};

let video = {
  let%map_open.Command () = return()
  and anon = anon(maybe("VIDEO" %: Video_id.Plain_or_in_url.arg_type))
  and escaped =
    flag(
      "--",
      escape,
      ~doc="VIDEO escape a video whose ID may start with [-]",
    );

  switch (
    Option.to_list(anon)
    @ List.map(
        Option.value(escaped, ~default=[]),
        ~f=Video_id.Plain_or_in_url.of_string,
      )
  ) {
  | [] => raise_s([%message "expected exactly one video"])
  | [spec] => spec
  | [_, ..._] as specs =>
    raise_s(
      [%message "expected exactly one video"(specs: list(Video_id.t))],
    )
  };
};

let videos = {
  let%map_open.Command () = return()
  and anons = anon(sequence("VIDEO" %: Video_id.Plain_or_in_url.arg_type))
  and escaped =
    flag(
      "--",
      escape,
      ~doc="VIDEO escape videos whose IDs may start with [-]",
    );

  anons
  @ List.map(
      Option.value(escaped, ~default=[]),
      ~f=Video_id.Plain_or_in_url.of_string,
    );
};

let nonempty_videos =
  switch%map.Command (videos) {
  | [] => raise_s([%message "expected at least one video"])
  | [_, ..._] as specs => specs
  };

let nonempty_videos_or_playlist = {
  open Command.Param;
  let nonempty_videos =
    switch%map.Command (videos) {
    | [] => None
    | [_, ..._] as videos => Some(`Videos(videos))
    };

  let playlist_flag_name = "-playlist";
  let playlist =
    switch%map.Command (
      flag(
        playlist_flag_name,
        optional(Playlist_id.Plain_or_in_url.arg_type),
        ~doc=
          "PLAYLIST specify videos in PLAYLIST rather than individual command-line arguments",
      )
    ) {
    | None => None
    | Some(playlist_id) => Some(`Playlist(playlist_id))
    };

  let remove_from_playlist =
    flag(
      "-remove-from-playlist",
      no_arg,
      ~doc=
        " if reading videos from a playlist, remove videos from playlist when done",
    );

  let%map.Command which =
    choose_one([nonempty_videos, playlist], ~if_nothing_chosen=Raise)
  and remove_from_playlist = remove_from_playlist;
  switch (which, remove_from_playlist) {
  | (`Videos(_), true) =>
    raise_s(
      [%message [%string "VIDEO may not be used with %{playlist_flag_name}"]],
    )
  | (`Videos(_) as which, _) => which
  | (`Playlist(playlist_id), remove_from_playlist) =>
    `Playlist((playlist_id, remove_from_playlist))
  };
};
