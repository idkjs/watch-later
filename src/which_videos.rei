open! Core;
open! Async;
open! Import;

type t =
  | These(list(Video_id.t))
  | Filter(Filter.t);

let param: (~default: t) => Command.Param.t(t);
