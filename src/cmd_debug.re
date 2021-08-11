open! Core;
open! Async;
open! Import;

module Get_video = {
  module What_to_show = {
    type t =
      | Video_info
      | Json({extra_parts: list(string)});

    let param = {
      open Command.Let_syntax;
      let%map_open () = return()
      and json = flag("-json", no_arg, ~doc=" Display raw JSON API response")
      and parts =
        flag(
          "-part",
          listed(string),
          ~doc=
            "PART include PART in the JSON response (see https://developers.google.com/youtube/v3/docs/videos/list).  Can be passed multiple times.",
        );

      switch (parts) {
      | [] =>
        if (json) {
          Json({extra_parts: []});
        } else {
          Video_info;
        }
      | [_, ..._] when !json =>
        failwith("[-part] can only be used with [-json]")
      | [_, ..._] as extra_parts => Json({extra_parts: extra_parts})
      };
    };
  };

  let main = (~api, ~video_ids, ~what_to_show) => {
    let video_ids = Pipe.of_list(video_ids);
    switch ((what_to_show: What_to_show.t)) {
    | Video_info =>
      Youtube_api.get_video_info(api, video_ids)
      |> Pipe.iter_without_pushback(~f=video_info =>
           print_s([%sexp (video_info: Or_error.t(Video_info.t))])
         )
      |> Deferred.ok
    | Json({extra_parts}) =>
      Youtube_api.get_video_json(
        api,
        video_ids,
        ~parts=["snippet", ...extra_parts],
      )
      |> Pipe.iter_without_pushback(~f=json =>
           print_endline(Json.to_string_pretty(ok_exn(json)))
         )
      |> Deferred.ok
    };
  };

  let command =
    Youtube_api.command(
      ~summary="Debug YouTube API calls",
      {
        let%map_open.Command () = return()
        and video_ids = Params.nonempty_videos
        and what_to_show = What_to_show.param;
        api => main(~api, ~video_ids, ~what_to_show);
      },
    );
};

let command =
  Command.group(
    ~summary="Debugging tools",
    [("db", Cmd_debug_db.command), ("get-video", Get_video.command)],
  );
