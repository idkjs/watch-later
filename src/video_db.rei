open! Core;
open! Async;
open! Import;

type t;

let with_file_and_txn:
  (string, ~f: t => Deferred.t(Or_error.t('a))) =>
  Deferred.t(Or_error.t('a));

let video_stats: t => Deferred.t(Or_error.t(Stats.t));
let add_video:
  (t, Video_info.t, ~overwrite: bool) => Deferred.t(Or_error.t(unit));
let get:
  (t, Video_id.t) => Deferred.t(Or_error.t(option((Video_info.t, bool))));
let mem: (t, Video_id.t) => Deferred.t(Or_error.t(bool));

let mark_watched:
  (t, Video_id.t, [ | `Watched | `Unwatched]) => Deferred.t(Or_error.t(unit));

let get_random_video: (t, Filter.t) => Deferred.t(Or_error.t(Video_id.t));
let get_videos: (t, Filter.t) => Pipe.Reader.t((Video_info.t, bool));
let strict_remove:
  (t, Video_id.t) => Deferred.t(Or_error.t([ | `Ok | `Missing]));
