open! Core;
open! Async;
open! Import;

/* TODO: Add validate and deriving fields for Stats.t and Video_info.t */

[@deriving sexp_of]
type t = {
  total_videos: int,
  watched_videos: int,
  unwatched_videos: int,
};
