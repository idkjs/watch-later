open! Core;
open! Async;
open! Import;

[@deriving sexp_of]
type t = {
  total_videos: int,
  watched_videos: int,
  unwatched_videos: int,
};
