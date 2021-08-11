open! Core;
open! Async;
open! Import;

/* FIXME: This type is not exposed, but [Video_db] depends on its field order. */
type t;

let empty: t;
let unwatched: t;
let param: Command.Param.t(t);
let is_empty: t => bool;
let t: Caqti_type.t(t);
