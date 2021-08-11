open! Core;
open! Async;
open! Import;

type t =
  | These(list(Video_id.t))
  | Filter(Filter.t);

let param = (~default) => {
  open Command.Let_syntax;
  let filter = {
    let%map filter = Filter.param;
    if (Filter.is_empty(filter)) {
      None;
    } else {
      Some(Filter(filter));
    };
  };

  let video_ids =
    switch%map (Params.videos) {
    | [] => None
    | [_, ..._] as nonempty_videos => Some(These(nonempty_videos))
    };

  Command.Param.choose_one(
    [filter, video_ids],
    ~if_nothing_chosen=Default_to(default),
  );
};
