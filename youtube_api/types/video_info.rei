open! Core_kernel;
open! Import;

/* TODO: Add video duration, thumbnail link, description? */
/* FIXME: This module is an anti-pattern.  It forces sub-optimal queries when not all
   fields may be necessary. */

[@deriving (fields, sexp_of)]
type t = {
  channel_id: string,
  channel_title: string,
  video_id: Video_id.t,
  video_title: string,
};
