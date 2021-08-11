open! Core;
open! Async;
open! Import;

type t;

let command:
  (
    ~extract_exn: bool=?,
    ~summary: string,
    ~readme: unit => string=?,
    Command.Param.t(t => Deferred.t(Or_error.t(unit)))
  ) =>
  Command.t;

let get_video_info:
  (t, Pipe.Reader.t(Video_id.t)) => Pipe.Reader.t(Or_error.t(Video_info.t));

/** See https://developers.google.com/youtube/v3/docs/videos/list for the
    documentation of [parts]. */

let get_video_json:
  (t, Pipe.Reader.t(Video_id.t), ~parts: list(string)) =>
  Pipe.Reader.t(Or_error.t(Json.t));

let get_playlist_items:
  (t, Playlist_id.t) => Pipe.Reader.t(Or_error.t(Playlist_item.t));
let delete_playlist_item:
  (t, Playlist_item.Id.t) => Deferred.t(Or_error.t(unit));

let append_video_to_playlist:
  (t, Playlist_id.t, Video_id.t) => Deferred.t(Or_error.t(unit));
