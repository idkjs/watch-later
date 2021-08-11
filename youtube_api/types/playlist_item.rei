/** An entry in a YouTube playlist, which usually contains a video.

    @see <https://developers.google.com/youtube/v3/docs/playlistItems> */;

open! Core_kernel;
open! Import;
module Id: String_id.S;

[@deriving sexp_of]
type t;

include Of_jsonable.S with type t := t;

/** Identifies an item within a playlist; different from the video ID. */

let id: t => Id.t;

let video_id: t => Video_id.t;
let video_info: t => Or_error.t(Video_info.t);
