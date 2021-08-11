open! Core_kernel;
open! Import;

module Id =
  String_id.Make(
    {
      let module_name = "Watch_later.Playlist_item.Id";
    },
    {},
  );

[@deriving (fields, sexp_of)]
type t = {
  id: Id.t,
  video_id: Video_id.t,
  video_info: Lazy.t(Video_info.t),
};

let of_json = {
  open Of_json.Let_syntax;
  let%map id = "id" @. string >>| Id.of_string
  and video_id = "snippet" @. "resourceId" @. "videoId" @. Video_id.of_json
  and video_info =
    lazy_(
      "snippet"
      @. {
        let%map channel_id = "videoOwnerChannelId" @. string
        and channel_title = "videoOwnerChannelTitle" @. string
        and video_id = "resourceId" @. "videoId" @. Video_id.of_json
        and video_title = "title" @. string;
        ({channel_id, channel_title, video_id, video_title}: Video_info.t);
      },
    );

  {id, video_id, video_info};
};

let video_info = t => Or_error.try_with(() => force(t.video_info));
